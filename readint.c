#include <stdio.h>
#include <stdlib.h>
#include <endian.h>
#include "readint.h"

uint8_t read_u8(FILE *fp)
{
    uint8_t val;
    if (!fread(&val, sizeof val, 1, fp)) {
        perror("Reading Error - failed to read a u8");
        exit(1);
    }
    return val;
}

uint16_t read_u16(FILE *fp)
{
    uint16_t val;
    if (!fread(&val, sizeof val, 1, fp)) {
        perror("Reading Error - failed to read a u16");
        exit(1);
    }
    return le16toh(val);
}

uint32_t read_u32(FILE *fp)
{
    uint32_t val;
    if (!fread(&val, sizeof val, 1, fp)) {
        perror("Reading Error - failed to read a u32");
        exit(1);
    }
    return le32toh(val);
}

uint64_t read_u64(FILE *fp)
{
    uint64_t val;
    if (!fread(&val, sizeof val, 1, fp)) {
        perror("Reading Error - failed to read a u64");
        exit(1);
    }
    return le64toh(val);
}
