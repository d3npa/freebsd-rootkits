#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>

#include <sys/ioctl.h>
#include <sys/memrange.h>

#define PATH_KMEM "/dev/kmem"
#define READ_SIZE 0x10

/*
 * Debug why I can't write to /dev/kmem
 * Situation: I can read but I cannot write
 * It is NOT:
 * 	A Permissions error (Perms: 7 = RWX)
 * 	A Memrange attribute error (Flags: 0)
 *
 * Error 14: Bad Address (only when writing)
 * TODO: restart lol
 * Reinstall FreeBSD?? Lmao
 * Find help
 */
int main()
{
	int kd;
	int errno;
	unsigned long offset;
	unsigned char buf[READ_SIZE];

	if ((kd = open(PATH_KMEM, O_RDWR)) == -1) {
		fprintf(stderr, "Failed to open %s\n", PATH_KMEM);
		return -1;
	}

	unsigned long addr = 0xffffffff80cae970;

	errno = 0;
	offset = lseek(kd, addr, SEEK_SET);
	printf("Read %zd bytes from 0x%lx -> %d\n", read(kd, buf, READ_SIZE),
		offset, errno);

	errno = 0;
	offset = lseek(kd, addr, SEEK_SET);
	printf("Wrote %zd bytes to 0x%lx -> %d\n", write(kd, buf, READ_SIZE),
		offset, errno);

	close(kd);
	return 0;
}
