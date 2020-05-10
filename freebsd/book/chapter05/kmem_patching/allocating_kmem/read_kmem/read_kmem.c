#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#define PATH_KMEM "/dev/kmem"

/*
 * Purpose
 * Read data from /dev/kmem directly
 */
int main()
{
	int kd;
	int errno;
	unsigned char buf[0x10];

	if ((kd = open(PATH_KMEM, O_RDWR)) == -1) {
		fprintf(stderr, "Failed to open %s\n", PATH_KMEM);
		return -1;
	}

	unsigned long addr = 0xffffffff80cae970;
	unsigned long offset = lseek(kd, addr, SEEK_SET);
	fprintf(stderr, "Set cursor to 0x%lx\n", offset);

	errno = 0;
	printf("Reading\t%zd\n", read(kd, buf, 0x10));
	if (errno != 0) {
		fprintf(stderr, "Errno: %d\n", errno);
		errno = 0;
	}

	printf("Writing\t%zd\n", write(kd, buf, 0x10));
	if (errno != 0) {
		fprintf(stderr, "Errno: %d\n", errno);
		errno = 0;
	}

	close(kd);

	return 0;
}
