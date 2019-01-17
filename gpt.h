#ifndef GPT_H__
#define GPT_H__

#include <stdint.h>
#include <uuid/uuid.h>

#define GPT_HEADER_SIG_NUM 0x5452415020494645ULL
#define GPT_HEADER_SIG_STR "EFI PART"

struct gpt_header {
    char     signature[8];
    uint32_t revision;
    uint32_t header_size_bytes;
    uint32_t crc32_of_header;
    // 4 bytes of 0s here
    uint64_t lba_of_current;    // This header
    uint64_t lba_of_backup;     // The other one
    uint64_t first_usable_lba;  // For partitions
    uint64_t last_usable_lba;   // For partitions
    uuid_t   disk_guid;
    uint64_t lba_start_of_part_entries;
    uint32_t num_part_entries;
    uint32_t single_part_entry_size;
    uint32_t crc32_of_part_array;
    // padded with zeros for rest of block / sector
};

struct gpt_part_entry {
    uuid_t   part_type_guid;
    uuid_t   unique_uuid;
    uint64_t first_lba;
    uint64_t last_lba; // inclusive, usually odd
    uint64_t attr_flags;
    char     part_name[72];
};

enum part_attr {
    PART_ATTR_PLATFORM_REQUIRED = 0,
    PART_ATTR_IGNORE = 1,
    PART_ATTR_BIOS_BOOT = 2
};

#endif
