#ifndef READINT_H__
#define READINT_H__

#include <stdio.h>
#include <stdint.h>

uint8_t     read_u8(FILE *fp);
uint16_t    read_u16(FILE *fp);
uint32_t    read_u32(FILE *fp);
uint64_t    read_u64(FILE *fp);

#endif
