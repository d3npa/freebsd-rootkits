#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <sys/syscall.h>

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

#define PATH_KMEM "/dev/kmem"
#define CODE_SIZE sizeof(kmalloc)
#define OFF_SYM_M_TEMP  0x0c + 3
#define OFF_SYM_MALLOC  0x18 + 1
#define OFF_SYM_COPYOUT 0x2e + 1

int main(int argc, char **argv)
{
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	struct nlist nl[5] = {[4] = {NULL}};
	unsigned char backup[CODE_SIZE];
	int i, status;
	void *addr;

	fprintf(stderr, "\033[95mkmalloc via kmem patching\033[0m\n");

	/* kmemの記述子を取得する */
	kd = kvm_openfiles(PATH_KMEM, NULL, NULL, O_RDWR, errbuf);
	if (kd == NULL) {
		fprintf(stderr,
			"\033[91mERROR: Cannot open %s\033[0m\n", PATH_KMEM);
		exit(-1);
	}

	nl[0].n_name = "sys_mkdir";
	nl[1].n_name = "M_TEMP";
	nl[2].n_name = "malloc";
	nl[3].n_name = "copyout";

	/* シンボルの位置を取得する */
	status = kvm_nlist(kd, nl);

	/* デバッグ */
	for (i = 0; i < sizeof(nl) / sizeof(*nl) - 1; i++) {
		fprintf(stderr, "\033[94mDEBUG: [%p] %s\033[0m\n",
					(void *)nl[i].n_value, nl[i].n_name);
	}

	if (status != 0) {
		fprintf(stderr,	"\033[91mERROR: %s\033[0m\n",
					"Some symbols could not be resolved");
	}

	/* sys_mkdirのアセンブラ命令を読み込む */
	status = kvm_read(kd, nl[0].n_value, backup, CODE_SIZE);
	fprintf(stderr, "Read %d bytes from %p\n", status, (void *)nl[0].n_value);
	if (status == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n",
					"Unable to read from device");
	}

	/* kmallocにある相対ジャンプ(call命令)をパッチする */
	*(int *)&kmalloc[OFF_SYM_M_TEMP] = nl[1].n_value;
	*(int *)&kmalloc[OFF_SYM_MALLOC] =
		nl[2].n_value - (nl[0].n_value + OFF_SYM_MALLOC + sizeof(int));
	*(int *)&kmalloc[OFF_SYM_COPYOUT] =
		nl[3].n_value - (nl[0].n_value + OFF_SYM_COPYOUT + sizeof(int));

	status = kvm_write(kd, nl[0].n_value, kmalloc, CODE_SIZE);
	fprintf(stderr, "Wrote %d bytes to %p\n", status, (void *)nl[0].n_value);
	if (status == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n",
					"Unable to write to device");
	}

	/* sys_mkdirを呼び出す(kmallocが実行される) */
	syscall(SYS_mkdir, (size_t) 128, &addr);

	// write(1, kmalloc, CODE_SIZE);

	/* sys_mkdirのアセンブラ命令をリストアする */
	status = kvm_write(kd, nl[0].n_value, backup, CODE_SIZE);
	fprintf(stderr, "Restored %d bytes to %p\n", status, (void *)nl[0].n_value);
	if (status == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n",
					"Unable to write to device");
	}

	printf("\033[92mAddress of kernel chunk: %p\033[0m\n", addr);

	kvm_close(kd);

	return 0;
}
