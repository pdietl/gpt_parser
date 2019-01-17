#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include "gpt.h"
#include "readint.h"
#include "fort.h"

void show_help(const char *name)
{
    printf( "%s <GPT-formated thing>\n"
            "    Prints all GPT knowledge that can be dervied from the argument\n",
            name);
}

void read_gpt_header(FILE *fp, struct gpt_header *hdr, int block_size)
{
    uint32_t temp;
    char buffer[16];
    long offset, start_offset;
    size_t i;
    uint8_t t;

    start_offset = ftell(fp);

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
    
    if (fread(buffer, sizeof *buffer, sizeof buffer, fp) != sizeof buffer) {
        perror("Error reading all 16 bytes of Disk GUID");
        exit(1);
    }

    uuid_parse(buffer, hdr->disk_guid);
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

void display_gpt_header(struct gpt_header *hdr)
{
    ft_table_t *table = ft_create_table();
    ft_set_cell_prop(table, 0, FT_ANY_COLUMN, FT_CPROP_ROW_TYPE, FT_ROW_HEADER);
    ft_set_border_style(table, FT_SOLID_STYLE);
    ft_write_ln(table, "Primary GPT Header");
    ft_set_cell_span(table, 0, 0, 2);
    ft_set_cell_prop(table, 0, 0, FT_CPROP_TEXT_ALIGN, FT_ALIGNED_CENTER);
    
    ft_printf_ln(table, "Signature|'%s'", hdr->signature);

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

    if (!(fp = fopen(argv[1], "r"))) {
        perror("Error opening file");
        fprintf(stderr, "Offending file: %s\n", argv[1]);
        exit(1);
    }

    if (fseek(fp, block_size, SEEK_SET)) {
        perror("Error skipping to Logical Block Address (LBA) 1");
        exit(1);
    }

    read_gpt_header(fp, &header, block_size);
    printf("GPT header scanned successfully\n");
    display_gpt_header(&header);

    return 0;
}
