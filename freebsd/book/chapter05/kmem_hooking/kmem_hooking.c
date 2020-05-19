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
#define HOOK_SIZE               0x2b - 0x00
#define JUMP_SIZE               0x37 - 0x2b
#define OFFSET_HOOK_MSG         0x12 + 2
#define OFFSET_HOOK_UPRINTF     0x1c + 2

unsigned char kmalloc[] =
/* 0000000000000050 <kmalloc>:                                              */
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
/* 0000000000000000 <msg>:                                                  */
/* 00: */  "Hello, world!\n\0"
/* 000000000000000f <hook>:                                                 */
/* 0f: */  "\x50"                        /* push   rax                      */
/* 10: */  "\x56"                        /* push   rsi                      */
/* 11: */  "\x57"                        /* push   rdi                      */
/* 12: */  "\x48\xbf\0\0\0\0\0\0\0\0"    /* mov    rdi,0x0                  */
/* 1c: */  "\x48\xb8\0\0\0\0\0\0\0\0"    /* mov    rax,0x0                  */
/* 26: */  "\xff\xd0"                    /* call   rax                      */
/* 28: */  "\x5f"                        /* pop    rdi                      */
/* 29: */  "\x5e"                        /* pop    rsi                      */
/* 2a: */  "\x58";                       /* pop    rax                      */

unsigned char jump[] =
/* 000000000000002b <jump>:                                                 */
/* 2b: */  "\x48\xb8\0\0\0\0\0\0\0\0"    /* mov    rax,0x0                  */
/* 35: */  "\xff\xe0";                   /* jmp    rax                      */

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

	/* カーネルメモリデバイスを開く */
	kd = kvm_openfiles("/dev/kmem", NULL, NULL, O_RDWR, errbuf);
	if (kd == NULL) {
		fprintf(stderr,
			"\033[91mError: Could not open device\033[0m\n");
		return -1;
	}

	/* シンボル解決 */
	if (kvm_nlist(kd, nl) == -1) {
		fprintf(stderr, "\033[91mError: %s\033[0m\n", kvm_geterr(kd));
		return -1;
	}

	fprintf(stderr, "\033[92m[+] sys_mkdir is at %p\033[0m\n",
						(void *)nl[0].n_value);

	/* sys_mkdirの命令を読み込む */
	kvm_read(kd, nl[0].n_value, backup, KMALLOC_SIZE);

	/*
	 * sys_mkdirの中にはjmp命令がある。
	 * ヒープからsys_mkdirに戻るとき、実行が止めないように
	 * そのjmp命令に戻りたいので、オフセットを取る。
	 */
	for (i = jmp_offset = JUMP_SIZE;; i++) {
		unsigned char instruction = backup[i];
		if (instruction == 0xeb) {
			// jmp
			jmp_offset = i;
			break;
		} else if (instruction == 0xc3) {
			// ret
			fprintf(stderr, "\033[91mError: %s\033[0m\n",
				"Did not find 0xe8 instruction in sys_mkdir!");
			return -1;
		}
	}

	/* kmallocのシェルコードを、前の段階で解決したアドレスでパッチする */
	*(unsigned int *)&kmalloc[OFFSET_M_WAIT] = nl[1].n_value;
	*(unsigned int *)&kmalloc[OFFSET_MALLOC] = nl[2].n_value -
		(nl[0].n_value + OFFSET_MALLOC + sizeof(unsigned int));
	*(unsigned int *)&kmalloc[OFFSET_COPYOUT] = nl[3].n_value -
		(nl[0].n_value + OFFSET_COPYOUT + sizeof(unsigned int));

	/*
	 * kmallocのシェルコードをsys_mkdirのところに投入する。
	 * それから、mkdirシスコールを実行することでヒープ領域を確保する。
	 * 最後にsys_mkdirもとの状態に戻す。
	 */
	int size = HOOK_SIZE + jmp_offset + JUMP_SIZE;
	kvm_write(kd, nl[0].n_value, kmalloc, KMALLOC_SIZE);
	syscall(SYS_mkdir, (size_t) size, &chunk);
	kvm_write(kd, nl[0].n_value, backup, KMALLOC_SIZE);

	fprintf(stderr, "\033[92m[+] allocated 0x%x bytes at %p\033[0m\n",
							size, (void *)chunk);

	/* hookのシェルコードを、前の段階で解決したアドレスでパッチする */
	*(unsigned long *)&hook[OFFSET_HOOK_MSG] = chunk;
	*(unsigned long *)&hook[OFFSET_HOOK_UPRINTF] = nl[4].n_value;
	*(unsigned long *)&jump[2] = nl[0].n_value + jmp_offset;

	/* 確保したヒープ・チャンクにhookを書き込む */
	kvm_write(kd, chunk, hook, HOOK_SIZE);
	kvm_write(kd, chunk + HOOK_SIZE, backup, jmp_offset);
	kvm_write(kd, chunk + HOOK_SIZE + jmp_offset, jump, JUMP_SIZE);

	/* jumpを再利用し、sys_mkdirからhookにジャンプさせる */
	*(unsigned long*)&jump[2] = chunk + 0xf;
	kvm_write(kd, nl[0].n_value, jump, JUMP_SIZE);

	/* いろいろ表示したい */
	unsigned char dump[size];
	kvm_read(kd, chunk, dump, size);
	fprintf(stderr, "\033[92m[+] mkdir will jmp to %p\033[0m\n",
					(void *)*(unsigned long *)&jump[2]);
	fprintf(stderr, "\033[92m[+] hook will jmp back to %p\033[0m\n",
					(void *)nl[0].n_value + jmp_offset);
	fprintf(stderr, "\033[95m[+] dumping hook code:\033[0m\n");
	write(1, dump, size);

	kvm_close(kd);
	return 0;
}
