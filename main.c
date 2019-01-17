#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <string.h>
#include <byteswap.h>
#include "gpt.h"
#include "readint.h"
#include "fort.h"
#include "crc32.h"

#define GUID_NUM_STR_BYTES 37
#define NO 0

void show_help(const char *name)
{
    printf( "%s <GPT-formated thing>\n"
            "    Prints all GPT knowledge that can be dervied from the argument\n",
            name);
}

void read_gpt_header(FILE *fp, struct gpt_header *hdr, int block_size)
{
    uint32_t temp;
    long offset, start_offset;
    size_t i;
    uint8_t t;

    start_offset = ftell(fp);

    hdr->header_crc_ok = NO;
    hdr->part_array_crc_ok = NO;

    // Clear out the string
    memset(hdr->signature, 0, sizeof hdr->signature);

    if (fread(hdr->signature, 1, 8, fp) != 8) {
        perror("Error trying to read the GPT header signature");
        exit(1);
    }

    hdr->revision = read_u32(fp);
    hdr->header_size_bytes = read_u32(fp);
    hdr->crc32_of_header = read_u32(fp);

    temp = read_u32(fp);
    if (temp != 0) {
        fprintf(stderr, "Error corrupt GPT header - read non-zero 32 bits at offset 20\n");
        exit(1);
    }

    hdr->lba_of_current = read_u64(fp);
    hdr->lba_of_backup = read_u64(fp);
    hdr->first_usable_lba = read_u64(fp);
    hdr->last_usable_lba = read_u64(fp);

    // Group numbering     1      2      3      4          5
    //   UUID binary is xxxx  =  xx  =  xx  =  xx  =  xxxxxx
    //          
    
    // where each x is a byte. Each individual groupings of x's are delimited
    // by '=' signs. What's weird is that integers in groups 1, 2, and 3
    // are little endian, BUT groups 4 and 5 are big endian!

    // What's worse, the uuid library wants them all to be big endian.
    // Here's an example:

    // GUID:                    3B7CDB12-7FE4-4625-9C00-B3AE6DC7246D
        
    // The binary encoding:     12 db 7c 3b e4 7f 25 46 9c 00 b3 ae 6d c7 24 6d

    if (fread(hdr->disk_guid, 1, 16, fp) != 16) {
        perror("Error reading all 16 bytes of Disk GUID");
        exit(1);
    }

    // This will be painful to convert lol

    *((uint32_t *)(hdr->disk_guid)) = bswap_32(*((uint32_t *)(hdr->disk_guid)));
    *((uint16_t *)(hdr->disk_guid + 4)) = bswap_16(*((uint16_t *)(hdr->disk_guid + 4)));
    *((uint16_t *)(hdr->disk_guid + 6)) = bswap_16(*((uint16_t *)(hdr->disk_guid + 6)));

    hdr->lba_start_of_part_entries = read_u64(fp);
    hdr->num_part_entries = read_u32(fp);
    hdr->single_part_entry_size = read_u32(fp);
    hdr->crc32_of_part_array = read_u32(fp);

    offset = ftell(fp);

    // The rest of the block must be filled with zeros
    for (i = 0; i < (block_size - (offset - start_offset)); ++i) {
        if (fread(&t, 1, 1, fp) != 1) {
            perror("Error reading the remainder of the GPT header LBA");
            exit(1);
        }
        if (t != 0) {
            fprintf(stderr, "Error - corrupt GPT header. We expected to read zeros until the end of the LBA\n");
            exit(1);
        }
    }
}

int get_block_size(const char *f)
{
    int fd, block_size;

    if ((fd = open(f, O_RDONLY)) == -1) {
        perror("Error opening file");
        fprintf(stderr, "Offending file: %s\n", f);
        exit(1);
    }

    if (ioctl(fd, BLKSSZGET, &block_size)) {
        perror("Error reading the sector size");
        exit(1);
    }

    close(fd);
    return block_size;
}

void check_header_crc(FILE *fp, struct gpt_header *hdr, int block_size)
{
    uint8_t full_header[hdr->header_size_bytes];
    uint32_t crc;

    // Move back to the beginning of the header
    if (fseek(fp, block_size, SEEK_SET)) {
        perror("Error skipping to Logical Block Address (LBA) 1");
        exit(1);
    }

    if (fread(full_header, 1, hdr->header_size_bytes, fp) != hdr->header_size_bytes) {
        perror("Error reading GPT header in full");
        exit(1);
    }

    // Now we need to zero-out the CRC we just read,
    // per the instructions for calculating the CRC

   *(((uint32_t *)(full_header)) + 4) = 0;

    crc32(full_header, sizeof full_header, &crc);
    hdr->header_crc_ok = (crc == hdr->crc32_of_header);
}

void check_part_array_crc(FILE *fp, struct gpt_header *hdr, int block_size)
{
    uint8_t full_part_array[hdr->num_part_entries * hdr->single_part_entry_size];
    uint32_t crc;

    // Move back to the beginning of the part array
    if (fseek(fp, block_size * hdr->lba_start_of_part_entries, SEEK_SET)) {
        perror("Error skipping to beginning of partition entries array");
        exit(1);
    }

    if (fread(full_part_array, hdr->single_part_entry_size, 
        hdr->num_part_entries, fp) != hdr->num_part_entries) {
        perror("Error reading partition entries array in full");
        exit(1);
    }

    crc32(full_part_array, sizeof full_part_array, &crc);
    hdr->part_array_crc_ok = (crc == hdr->crc32_of_part_array);
    printf("CHECK: %x\n", crc);
}

void display_gpt_header(struct gpt_header *hdr)
{
    char buffer[GUID_NUM_STR_BYTES] = {0};

    ft_table_t *table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_border_style(table, FT_SOLID_STYLE);
    ft_write_ln(table, "Primary GPT Header");
    ft_set_cell_span(table, 0, 0, 2);
    ft_set_cell_prop(table, FT_ANY_ROW, 1, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_RIGHT);
    ft_set_cell_prop(table, 0, 0, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    
    ft_printf_ln(table, "Signature|'%s'", hdr->signature);
    ft_printf_ln(table, "Revision|0x%08X", hdr->revision);
    ft_printf_ln(table, "Header Size|%uB", hdr->header_size_bytes);
    ft_set_cell_prop(table, 4, FT_ANY_COLUMN, FT_CPROP_CONT_BG_COLOR, 
        hdr->header_crc_ok ? FT_COLOR_GREEN : FT_COLOR_RED);

    ft_printf_ln(table, "Header CRC32|0x%X|%s", hdr->crc32_of_header,
        hdr->header_crc_ok ? "crc OK" : "crc BAD");
    
    ft_printf_ln(table, "Current LBA|%lu", hdr->lba_of_current);
    ft_printf_ln(table, "Backup LBA|%lu", hdr->lba_of_backup);
    ft_printf_ln(table, "First Usable LBA|%lu", hdr->first_usable_lba);
    ft_printf_ln(table, "Last Usable LBA|%lu", hdr->last_usable_lba);
    uuid_unparse(hdr->disk_guid, buffer);
    ft_printf_ln(table, "Disk GUID|%s", buffer);
    ft_printf_ln(table, "LBA Part Start|%lu", hdr->lba_start_of_part_entries);

    ft_printf_ln(table, "Num Part Entries|%u", hdr->num_part_entries);
    ft_printf_ln(table, "Part Entry Size|%uB", hdr->single_part_entry_size);
    ft_printf_ln(table, "CRC32 of Part Entries|0x%X", hdr->crc32_of_part_array);
    printf("%s\n", ft_to_string(table));
    ft_destroy_table(table);
}

int main(int argc, char **argv)
{
    FILE *fp;
    int block_size;
    struct gpt_header header;

    if (argc <= 1) {
        fprintf(stderr, "ERROR: expected a <GPT-formatted thing> as an argument\n");
        exit(1);
    }

    block_size = get_block_size(argv[1]);

    if (!(fp = fopen(argv[1], "rb"))) {
        perror("Error opening file");
        fprintf(stderr, "Offending file: %s\n", argv[1]);
        exit(1);
    }

    if (fseek(fp, block_size, SEEK_SET)) {
        perror("Error skipping to Logical Block Address (LBA) 1");
        exit(1);
    }

    read_gpt_header(fp, &header, block_size);
    check_header_crc(fp, &header, block_size);
    check_part_array_crc(fp, &header, block_size);
    printf("GPT header scanned successfully\n");
    display_gpt_header(&header);

    return 0;
}
