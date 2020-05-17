#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define KMALLOC_SIZE            0x90 - 0x50
#define OFFSET_M_WAIT           0x0c + 3
#define OFFSET_MALLOC           0x18 + 1
#define OFFSET_COPYOUT          0x2e + 1
#define OFFSET_HOOK_START       0x0f
#define OFFSET_HOOK_MSG         0x11 + 2
#define OFFSET_HOOK_UPRINTF     0x1b + 2
#define HOOK_SIZE		0x29 - 0x00
#define JUMP_SIZE		0x35 - 0x29

unsigned char kmalloc[] =
/* 0000000000000050 <kmalloc>: */
/* 50: */ "\x55"                         /* push   rbp                      */
/* 51: */ "\x48\x89\xe5"                 /* mov    rbp,rsp                  */
/* 54: */ "\x53"                         /* push   rbx                      */
/* 55: */ "\x50"                         /* push   rax                      */
/* 56: */ "\x48\x89\xf3"                 /* mov    rbx,rsi                  */
/* 59: */ "\x48\x8b\x3e"                 /* mov    rdi,QWORD PTR [rsi]      */
/* 5c: */ "\x48\xc7\xc6\x00\x00\x00\x00" /* mov    rsi,0x0                  */
/* 63: */ "\xba\x02\x01\x00\x00"         /* mov    edx,0x102                */
/* 68: */ "\xe8\x00\x00\x00\x00"         /* call   6d <kmalloc+0x1d>        */
/* 6d: */ "\x48\x89\x45\xf0"             /* mov    QWORD PTR [rbp-0x10],rax */
/* 71: */ "\x48\x8b\x73\x08"             /* mov    rsi,QWORD PTR [rbx+0x8]  */
/* 75: */ "\x48\x8d\x7d\xf0"             /* lea    rdi,[rbp-0x10]           */
/* 79: */ "\xba\x08\x00\x00\x00"         /* mov    edx,0x8                  */
/* 7e: */ "\xe8\x00\x00\x00\x00"         /* call   83 <kmalloc+0x33>        */
/* 83: */ "\x48\x83\xc4\x08"             /* add    rsp,0x8                  */
/* 87: */ "\x5b"                         /* pop    rbx                      */
/* 88: */ "\x5d"                         /* pop    rbp                      */
/* 89: */ "\xc3";                        /* ret                             */

unsigned char hook[] =
/* 0000000000000000 <msg>: */
/* 00: */  "Hello, world!\n\0"
/* 000000000000000f <hook>: */
/* 0f: */ "\x57"                         /* push   rdi                     */
/* 10: */ "\x50"                         /* push   rax                     */
/* 11: */ "\x48\xbf\0\0\0\0\0\0\0\0"     /* mov    rdi,0x0                 */
/* 1b: */ "\x48\xb8\0\0\0\0\0\0\0\0"     /* mov    rax,0x0                 */
/* 25: */ /*"\xff\xd0"*/ "\x90\x90"                     /* call   rax                     */
/* 27: */ "\x58"                         /* pop    rax                     */
/* 28: */ "\x5f";                        /* pop    rdi                     */

unsigned char jump[] =
/* 0000000000000029 <jump>: */
/* 29: */ "\x48\xb8\0\0\0\0\0\0\0\0"     /* mov    rax,0x0                 */
/* 33: */ "\xff\xe0";                    /* jmp    rax                     */

/*
 *	End goal:
 *	Hook syscall via kmem patching
 */
int main()
{
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	unsigned char backup[KMALLOC_SIZE];
	int i, jmp_offset;
	unsigned long chunk;
	struct nlist nl[6] = {
		{ .n_name = "sys_mkdir" },
		{ .n_name = "M_TEMP"    },
		{ .n_name = "malloc"    },
		{ .n_name = "copyout"   },
		{ .n_name = "uprintf"   },
		{ NULL }
	};

	kd = kvm_openfiles("/dev/kmem", NULL, NULL, O_RDWR, errbuf);
	if (kd == NULL) {
		fprintf(stderr,
			"\033[91mError: Could not open device\033[0m\n");
		return -1;
	}


	/* シンボル解決 */
	kvm_nlist(kd, nl);

	/* sys_mkdirを読み込む */
	kvm_read(kd, nl[0].n_value, backup, KMALLOC_SIZE);

	/*
	 * フック関数をカーネルに書き込むのに必要なチャンクサイズを把握する
	 */

	/* 最初のジャンプ命令のオフセットを取る */
	for (i = jmp_offset = 0;; i++) {
		unsigned char instruction = backup[i];
		if (instruction == 0xeb) {
			// Found
			jmp_offset = i;
			break;
		} else if (instruction == 0xc3) {
			// Return
			fprintf(stderr, "Error: did not find 0xeb byte!\n");
			return -1;
		}
	}

	int size = HOOK_SIZE + jmp_offset + JUMP_SIZE;
	fprintf(stderr, "[+] Need to allocate 0x%x bytes for hook\n", size);

	/*
	 * カーネル空間のチャンクを確保する
	 */

	/* kmallocのコード内さまざまな相対アドレスを解決する */
	*(unsigned int *)&kmalloc[OFFSET_M_WAIT] = nl[1].n_value;
	*(unsigned int *)&kmalloc[OFFSET_MALLOC] = nl[2].n_value -
		(nl[0].n_value + OFFSET_MALLOC + sizeof(unsigned int));
	*(unsigned int *)&kmalloc[OFFSET_COPYOUT] = nl[3].n_value -
		(nl[0].n_value + OFFSET_COPYOUT + sizeof(unsigned int));

	/* kmallocをsys_mkdirのところに投入し、実行後にsys_mkdirをリストア */
	kvm_write(kd, nl[0].n_value, kmalloc, KMALLOC_SIZE);
	syscall(SYS_mkdir, (size_t) size, &chunk);
	kvm_write(kd, nl[0].n_value, backup, KMALLOC_SIZE);

	fprintf(stderr, "[+] Allocated kernel chunk at %p\n", (void *)chunk);

	/*
	 * フック関数をチャンクに書き込み、sys_mkdirのところにjmp命令を投入する
	 */

	/* hookコード内の相対アドレスを計算する */
	*(unsigned long *)&hook[OFFSET_HOOK_MSG] = chunk;
	*(unsigned long *)&hook[OFFSET_HOOK_UPRINTF] = nl[4].n_value;
	 	// -
		// (chunk + OFFSET_HOOK_UPRINTF + sizeof(unsigned long));
	*(unsigned long *)&jump[2] = nl[0].n_value + jmp_offset;

	/*
	 * チャンクの構成
	 * - hook自体のコード
	 * - sys_mkdir上書きした命令
	 * - sys_mkdirに戻るjmp命令
	 */
	kvm_write(kd, chunk, hook, HOOK_SIZE);
	kvm_write(kd, chunk + HOOK_SIZE, backup, jmp_offset);
	kvm_write(kd, chunk + HOOK_SIZE + jmp_offset, jump, JUMP_SIZE);

	/* デバッグ - 計算した相対アドレスを自ら確認 */
	fprintf(stderr, "\033[90m[DEBUG] jmp addr in mkdir %p\033[0m\n",
		(void *)(nl[0].n_value + jmp_offset));
	fprintf(stderr, "\033[90m[DEBUG] hook will jmp to %p\033[0m\n",
		(void *)*(unsigned long *)&jump[2]);

	unsigned char dump[size];
	kvm_read(kd, chunk, dump, size);
	write(1, dump, size);

	/* jumpを再利用。hookの絶対アドレスを指す */
	*(unsigned long*)&jump[2] = chunk + OFFSET_HOOK_START;
	fprintf(stderr, "\033[90m[DEBUG] mkdir will jmp to %p\033[0m\n",
		(void *)*(unsigned long *)&jump[2]);

	// sys_mkdirをhookにジャンプさせる
	kvm_write(kd, nl[0].n_value, jump, JUMP_SIZE);

	kvm_close(kd);
	return 0;
}
