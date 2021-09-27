#include "decode_utils.h"

unsigned short crc16_table[16] = {
	0x0000, 0xCC01, 0xD801, 0x1400, 0xF001, 0x3C00, 0x2800, 0xE401,
	0xA001, 0x6C00, 0x7800, 0xB401, 0x5000, 0x9C01, 0x8801, 0x4400
};

static unsigned short crc16(unsigned short init, unsigned char *buf, size_t len)
{
	unsigned short crc16 = init;

	for (size_t i = 0; i < len; i++) {
		/* compute checksum of lower four bits of *p */
		unsigned short r = crc16_table[crc16 & 0xf];
		crc16 = (crc16 >> 4) & 0x0fff;
		crc16 = crc16 ^ r ^ crc16_table[buf[i] & 0xf];

		/* now compute checksum of upper four bits of *p */
		r = crc16_table[crc16 & 0xf];
		crc16 = (crc16 >> 4) & 0x0fff;
		crc16 = crc16 ^ r ^ crc16_table[(buf[i] >> 4) & 0xf];
	}

	return crc16;
}

int decode_block(unsigned short *diff, unsigned char *buf, size_t block_size)
{
	for (unsigned int i = 0; i < block_size; i++) {
		unsigned int tmp = buf[i];
		tmp -= *diff;
		buf[i] = tmp; //only one byte
	}

	unsigned int read_cksum = *(unsigned int *)(buf + block_size - 4);
	unsigned short calc_cksum = crc16(*diff, buf, block_size - 4);

	*diff = read_cksum & 0xffff;

	if ((calc_cksum != (read_cksum & 0xffff)) || ((read_cksum >> 16) + (read_cksum & 0xffff) != 0xffff))
		return -1;

	return 0;
}

#ifdef ENCODE_SUPPORT
void encode_block(unsigned short *diff, unsigned char *buf, size_t block_size)
{
	unsigned short calc_cksum = crc16(*diff, buf, block_size);
	*(unsigned int *)(buf + block_size) = ((0xffff - calc_cksum) << 16) | calc_cksum;

	for (unsigned int i = 0; i < block_size + 4; i++) {
		unsigned int tmp = buf[i];
		tmp += *diff;
		buf[i] = tmp; //only one byte
	}

	*diff = calc_cksum;
}
#endif
