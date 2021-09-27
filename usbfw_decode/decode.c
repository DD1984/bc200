#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <limits.h>
#include <libgen.h>

#include "decode_utils.h"

typedef enum {
	M_NONE,
	M_DEC,
	M_ENC,
} work_mode_t;

#define OUTFILE_PREF "BC200_Upgrade_"

void show_usage(void)
{
	printf(
		"\t-i   input file (binary format)\n"
		"\t-o   output file; \""OUTFILE_PREF"\" prefix will be added (only for encode)\n"
		"\t-d   decode\n"
		"\t-e   encode\n"
	);
}

int open_files(const char *infile, const char *outfile, int *fd_in, int *fd_out, const char *file_prefix)
{
	*fd_in = open(infile, O_RDONLY);
	if (*fd_in < 0) {
		printf("err open infile:[%s]\n", infile);
		return -1;
	}

	struct stat st;
	fstat(*fd_in, &st);

	char buf[PATH_MAX];
	strcpy(buf, outfile);

	if (file_prefix) {
		char *dir = strdup(outfile);
		char *name = strdup(outfile);

		if (dir && name)
			sprintf(buf, "%s/%s%s", dirname(dir), file_prefix, basename(name));

		if (dir)
			free(dir);
		if (name)
			free(name);
	}

	if (access(buf, F_OK) == 0)
		unlink(buf);

	*fd_out = open(buf, O_RDWR | O_CREAT | O_TRUNC, st.st_mode);
	if (*fd_out < 0) {
		printf("err open outfile:[%s]\n", buf);
		return -1;
	}

	return 0;
}

int decode(const char *infile, const char *outfile)
{
	int fd_in = -1;
	int fd_out = -1;
	int ret = -1;

	if (open_files(infile, outfile, &fd_in, &fd_out, NULL))
		goto out;

	ret = 0;

	unsigned char buf[BLOCK_SIZE];

	unsigned short diff = START_DIFF;

	size_t addr = 0;
	ssize_t block_size;
	while ((block_size = read(fd_in, buf, BLOCK_SIZE)) > 0) {

		ret = decode_block(&diff, buf, block_size);
		if (ret)
			printf("decode error - 0x%08zx: block_size: 0x%03zx, diff: 0x%02hhx[%02hhx]\n",
				addr, block_size - 4, diff >> 8, diff & 0xff);

		write(fd_out, buf, block_size - 4);

		addr += block_size;
	}

out:
	if (fd_in >= 0)
		close(fd_in);
	if (fd_out >= 0)
		close(fd_out);

	return ret;
}

int encode(const char *infile, const char *outfile)
{
	int fd_in = -1;
	int fd_out = -1;
	int ret = -1;

	if (open_files(infile, outfile, &fd_in, &fd_out, OUTFILE_PREF))
		goto out;

	ret = 0;

	unsigned char buf[BLOCK_SIZE];

	unsigned short diff = START_DIFF;

	size_t addr = 0;
	ssize_t block_size;
	while ((block_size = read(fd_in, buf, BLOCK_SIZE - 4)) > 0) {

		encode_block(&diff, buf, block_size);

		write(fd_out, buf, block_size + 4);
	}

out:
	if (fd_in >= 0)
		close(fd_in);
	if (fd_out >= 0)
		close(fd_out);

	return ret;
}


int main(int argc, char *argv[])
{
	work_mode_t mode = M_NONE;

	char *infile = NULL;
	char *outfile = NULL;

	int c;
	while ((c = getopt(argc, argv, "i:o:edh")) != -1) {
		switch (c) {
			case 'i':
				infile = optarg;
				break;
			case 'o':
				outfile = optarg;
				break;
			case 'e':
				mode = M_ENC;
				break;
			case 'd':
				mode = M_DEC;
				break;
			case 'h':
			case '?':
			default:
				show_usage();
				return 0;
		}
	}

	if ((!infile || !outfile) ||
		(mode == M_NONE)) {
		show_usage();
		return 0;
	}

	switch (mode) {
		case M_DEC:
			decode(infile, outfile);
			break;
		case M_ENC:
			encode(infile, outfile);
			break;
	}

	return 0;
}
