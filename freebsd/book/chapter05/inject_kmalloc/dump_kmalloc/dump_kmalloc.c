#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <unistd.h>

#define KMALLOC_SIZE 0x89 - 0x50


int main()
{
	kvm_t *kd;
	struct nlist nl[] = {{NULL}, {NULL}};
	char errbuf[_POSIX2_LINE_MAX];
	unsigned char code[KMALLOC_SIZE];

	kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, errbuf);

	nl[0].n_name = "kmalloc";
	kvm_nlist(kd, nl);
	fprintf(stderr, "DEBUG: [%p] %s\n", (void *)nl[0].n_value, nl[0].n_name);

	kvm_read(kd, nl[0].n_value, code, KMALLOC_SIZE);

	write(1, code, KMALLOC_SIZE);

	kvm_close(kd);

	return 0;
}
