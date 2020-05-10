#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

unsigned char kmalloc[] =
	"\x55"                            /* push   rbp                       */
	"\x48\x89\xe5"                    /* mov    rbp,rsp                   */
	"\x53"                            /* push   rbx                       */
	"\x50"                            /* push   rax                       */
	"\x48\x89\xf3"                    /* mov    rbx,rsi                   */
	"\x48\x8b\x3e"                    /* mov    rdi,QWORD PTR [rsi]       */
	"\x48\xc7\xc6\x00\x00\x00\x00"    /* mov    rsi,0x0                   */
	"\xba\x02\x01\x00\x00"            /* mov    edx,0x102                 */
	"\xe8\x00\x00\x00\x00"            /* call   6d <kmalloc_handler+0x1d> */
	"\x48\x89\x45\xf0"                /* mov    QWORD PTR [rbp-0x10],rax  */
	"\x48\x8b\x73\x08"                /* mov    rsi,QWORD PTR [rbx+0x8]   */
	"\x48\x8d\x7d\xf0"                /* lea    rdi,[rbp-0x10]            */
	"\xba\x08\x00\x00\x00"            /* mov    edx,0x8                   */
	"\xe8\x00\x00\x00\x00"            /* call   83 <kmalloc_handler+0x33> */
	"\x48\x83\xc4\x08"                /* add    rsp,0x8                   */
	"\x5b"                            /* pop    rbx                       */
	"\x5d"                            /* pop    rbp                       */
	"\xc3";                           /* ret                              */

#define CODE_SIZE sizeof(kmalloc)
#define OFF_SYM_M_WAIT  0x0c + 3
#define OFF_SYM_MALLOC  0x18 + 1
#define OFF_SYM_COPYOUT 0x28 + 1

int main(int argc, char **argv)
{
	int error;
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	struct nlist nl[] = { { NULL }, { NULL }, { NULL }, { NULL }, { NULL }};
	unsigned char backup[CODE_SIZE];

	/*
		open kernel
		find mkdir, and symbols needed for kmalloc
		patch kmalloc code
		swap mkdir code
		syscall
		fix mkdir
		close all
	*/

	/* カーネルメモリの記述子を取得する */
	if ((kd = kvm_openfiles(NULL, NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n",
					"Unable to open kmem device");
		exit(-1);
	};

	/* シンボル解決 */
	nl[0].n_name = "sys_mkdir";
	nl[1].n_name = "M_TEMP";
	nl[2].n_name = "malloc";
	nl[3].n_name = "copyout";

	error = kvm_nlist(kd, nl);
	for (int i = 0; i < sizeof(nl) / sizeof(*nl) - 1; i++) {
		fprintf(stderr, "\033[94mDEBUG: [%p] %s\033[0m\n",
					(void *) nl[i].n_value, nl[i].n_name);
	}
	if (error != 0) {
		if (error == -1)
			fprintf(stderr, "\033[91mERROR: %s\033[0m\n",
					"Unable to read kernel symbol table");
		else
			fprintf(stderr,
	"\033[91mERROR: %d symbol(s) could not be resolved\033[0m\n",
				error);
		kvm_close(kd);
		exit(-1);
	}

	/* sys_mkdirの読み込み・バックアップ */
	if ((kvm_read(kd, nl[0].n_value, backup, CODE_SIZE)) == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		kvm_close(kd);
		exit(-1);
	}

	write(1, backup, CODE_SIZE);

	kvm_close(kd);

	return 0;
}
