#ifndef __DECODE_UTILS_H__
#define __DECODE_UTILS_H__

#include <stddef.h>

#define BLOCK_SIZE 512
#define START_DIFF 0x00c8

int decode_block(unsigned short *diff, unsigned char *buf, size_t block_size);
void encode_block(unsigned short *diff, unsigned char *buf, size_t block_size);

#endif
