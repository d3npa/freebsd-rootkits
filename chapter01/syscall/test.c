#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		printf("Usage: %s <message>\n", argv[0]);
		return 1;
	}

	return syscall(210, argv[1]);
}
