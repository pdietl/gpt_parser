#ifndef CRC32_H__
#define CRC32_H__

#include <stddef.h>
#include <stdint.h>

uint32_t crc32_for_byte(uint32_t r);
void crc32(const void *data, size_t n_bytes, uint32_t* crc);

#endif
