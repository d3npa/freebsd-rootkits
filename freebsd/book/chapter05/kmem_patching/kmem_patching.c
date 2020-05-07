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
 *
 *  ; printfの呼出をuprintfに書き換えたいけど、逆アセンブリの結果には
 *  ; callの引数がNULLになっていて、実行時に解決されることが確認できた。
 *  ; したがって関数の呼出を書き換えるには、まず両方のアドレスを解決し、
 *  ; ライブカーネルからcall命令の引数も取得する必要があります。
 *  ; それらから、新しいcallの引数を生成し、使えるようになります。
 *
 */
int main(void)
{
	int i, off_i, off_call;
	char errbuf[_POSIX2_LINE_MAX];
	kvm_t *kd;
	struct nlist nl[] = { {NULL}, {NULL}, {NULL}, {NULL} };
	unsigned char annoying_hello_code[SIZE];

	if ((kd = kvm_open(NULL, NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n", errbuf);
		exit(-1);
	}

	nl[0].n_name = "annoying_hello";
	nl[1].n_name = "printf";
	nl[2].n_name = "uprintf";

	if (kvm_nlist(kd, nl) != 0) {
		fprintf(stderr,
			"\033[91mERROR: Symbol %s not found\033[0m\n", nl[0].n_name);
		exit(-1);
	}

	for (i = 0; i < 3; i++) {
		printf("\033[92m[+] Found symbol %s at %p\033[0m\n",
			nl[i].n_name, (void *)nl[i].n_value);
	}

	/* annoying_helloのマシンコードを読み込む */
	if (kvm_read(kd, nl[0].n_value, annoying_hello_code, SIZE) == -1) {
		fprintf(stderr,
			"\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		exit(-1);
	}

	printf("--\n");

	/* cmpとcall命令を探索する */
	off_i = -1;
	off_call = -1;
	for (i = 3; i < SIZE; i++) {
		if (memcmp(&annoying_hello_code[i-2], "\x83\x7d\xec", 3) == 0 && off_i == -1) {
			/* forループの条件 */
			off_i = i + 1;
			printf("\033[92m[+] Found cmp instruction at +0x%02x\033[0m\n", off_i);
		} else if (annoying_hello_code[i] == 0xe8 && off_call == -1) {
			/* printfの呼び出し */
			off_call = i;
			printf("\033[92m[+] Found call instruction at +0x%02x\033[0m\n", off_call);
		}
	}

	printf("--\n");

	/* forループの条件を書き換える */
	annoying_hello_code[off_i] = 0x2;
	printf("\033[92m[+] New instructions at %p: %02hhx %02hhx %02hhx %02hhx\033[0m\n",
			(void *) nl[0].n_value + off_i,
			annoying_hello_code[off_i - 3], annoying_hello_code[off_i - 2],
			annoying_hello_code[off_i - 1], annoying_hello_code[off_i]);

	/* uprintfの相対位置を計算し、call行きを書き換える */
	unsigned long addr_ret = nl[0].n_value + off_call + 5;
	unsigned long addr_uprintf = nl[2].n_value;
	signed int rel = addr_uprintf - addr_ret;
	memcpy(annoying_hello_code + off_call + 1, &rel, 4);

	printf("\033[92m[+] New instructions at %p: %02hhx %02hhx %02hhx %02hhx %02hhx\033[0m\n",
			(void *) addr_ret,
			annoying_hello_code[off_call], annoying_hello_code[off_call + 1],
			annoying_hello_code[off_call + 2], annoying_hello_code[off_call + 3],
			annoying_hello_code[off_call + 3]);

	printf("--\n");

	/* 新しいコードをカーネルに書き込む */
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
