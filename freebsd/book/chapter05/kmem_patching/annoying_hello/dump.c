#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SIZE 0x10b - 0xc0

int main()
{
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL} };
	char errbuf[_POSIX2_LINE_MAX];
	unsigned char code[SIZE];

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n", errbuf);
		exit(-1);
	}

	nl[0].n_name = "annoying_hello";
	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr,
			"\033[91mERROR: Symbol %s not found\033[0m\n", nl[0].n_name);
		exit(-1);
	}

	if (nl[0].n_value == 0) {
		fprintf(stderr, "Error not found\n");
		exit(-1);
	}

	fprintf(stderr, "Dumping from %p\n", (void *)nl[0].n_value);
	kvm_read(kd, nl[0].n_value, code, SIZE);

	write(1, code, SIZE);

	kvm_close(kd);
}
