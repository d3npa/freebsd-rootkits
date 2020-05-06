#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

int main(void)
{
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr, "\003[91mError: %s\033[0m", errbuf);
		exit(-1);
	}

	kvm_close(kd);

	return 0;
}
