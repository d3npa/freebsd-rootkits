#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>

/*
 *  ; 以下のcmp命令を変え、0xaじゃなくて0x1までループさせる（一回だけ）
 *  83 7d ec 0a             cmp    DWORD PTR [rbp-0x14],0xa
 */
int main(void)
{
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL} }; // 2つもいらないんじゃない？

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr, "\003[91mERROR: %s\033[0m\n", errbuf);
		exit(-1);
	}

	nl[0].n_name = "annoying_hello";

	if (kvm_nlist(kd, nl) == -1) {
		fprintf(stderr,
			"\003[91mERROR: Symbol %s not found\033[0m\n", nl[0].n_name);
		exit(-1);
	}

	printf("\033[92mFound symbol %s at %p\033[0m\n", nl[0].n_name, (void *)nl[0].n_value);

	kvm_close(kd);

	return 0;
}
