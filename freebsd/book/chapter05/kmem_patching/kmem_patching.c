#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 0xdb - 0x90 = 0x4b
 */
#define SIZE 0x4b

/*
 *  ; 以下のcmp命令を書き換え、0xaを0x1にするので一回だけループさせる
 *  a7:   83 7d ec 0a             cmp    DWORD PTR [rbp-0x14],0xa
 */
int main(void)
{
	int offset;
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL} }; // 2つもいらないんじゃない？
	unsigned char annoying_hello_code[SIZE];

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

	printf("\033[92m[+] Found symbol %s at %p\033[0m\n",
				nl[0].n_name, (void *)nl[0].n_value);

	if (kvm_read(kd, nl[0].n_value, annoying_hello_code, SIZE) == -1) {
		fprintf(stderr,
			"\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		exit(-1);
	}

	/* cmp命令を探索する */
	for (offset = 3; offset < SIZE; offset++) {
		if (memcmp(&annoying_hello_code[offset-3], "\x83\x7d\xec\x0a", 4) == 0) {
			printf("\033[92m[+] Found cmp instruction at 0x%x\033[0m\n", offset);
			break;
		}
	}

	if (offset >= SIZE) {
		fprintf(stderr, "\033[93m[-] Unable to locate target instruction. "
			"Perhaps it was already overwritten?\033[0m\n");
		exit(0);
	}

	/* コードをパッチしてカーネルに書き込む */
	annoying_hello_code[offset] = 0x1;
	if (kvm_write(kd, nl[0].n_value, annoying_hello_code, SIZE) == -1) {
		fprintf(stderr,
			"\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		exit(-1);
	}

	printf("\033[92m[+] Patched function %s at %p\033[0m\n",
				nl[0].n_name, (void *)nl[0].n_value);

	if (kvm_close(kd) == -1) {
		fprintf(stderr,
			"\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		exit(-1);
	}

	exit(0);
}
