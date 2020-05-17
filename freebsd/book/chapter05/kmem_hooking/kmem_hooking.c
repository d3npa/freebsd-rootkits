#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <unistd.h>

#define KMALLOC_SIZE            0x90 - 0x50
#define OFFSET_M_WAIT           0x0c + 3
#define OFFSET_MALLOC           0x18 + 1
#define OFFSET_COPYOUT          0x2e + 1
#define OFFSET_HOOK_START       0x0f
#define OFFSET_HOOK_MSG         0x0f + 9
#define OFFSET_HOOK_UPRINTF     0x0f + 14

unsigned char kmalloc[] =
/* 0000000000000050 <kmalloc>: */
/* 50: */  "\x55"                          /* push   rbp                      */
/* 51: */  "\x48\x89\xe5"                  /* mov    rbp,rsp                  */
/* 54: */  "\x53"                          /* push   rbx                      */
/* 55: */  "\x50"                          /* push   rax                      */
/* 56: */  "\x48\x89\xf3"                  /* mov    rbx,rsi                  */
/* 59: */  "\x48\x8b\x3e"                  /* mov    rdi,QWORD PTR [rsi]      */
/* 5c: */  "\x48\xc7\xc6\x00\x00\x00\x00"  /* mov    rsi,0x0                  */
/* 63: */  "\xba\x02\x01\x00\x00"          /* mov    edx,0x102                */
/* 68: */  "\xe8\x00\x00\x00\x00"          /* call   6d <kmalloc+0x1d>        */
/* 6d: */  "\x48\x89\x45\xf0"              /* mov    QWORD PTR [rbp-0x10],rax */
/* 71: */  "\x48\x8b\x73\x08"              /* mov    rsi,QWORD PTR [rbx+0x8]  */
/* 75: */  "\x48\x8d\x7d\xf0"              /* lea    rdi,[rbp-0x10]           */
/* 79: */  "\xba\x08\x00\x00\x00"          /* mov    edx,0x8                  */
/* 7e: */  "\xe8\x00\x00\x00\x00"          /* call   83 <kmalloc+0x33>        */
/* 83: */  "\x48\x83\xc4\x08"              /* add    rsp,0x8                  */
/* 87: */  "\x5b"                          /* pop    rbx                      */
/* 88: */  "\x5d"                          /* pop    rbp                      */
/* 89: */  "\xc3";                         /* ret                             */
unsigned char backup[KMALLOC_SIZE];
unsigned char hook[] =
           "Hello, world!\n\0"
           "\x55"                          /* push   %ebp                     */
           "\x89\xe5"                      /* mov    %esp,%ebp                */
           "\x83\xec\x04"                  /* sub    $0x4,%esp                */
           "\xc7\x04\x24\x00\x00\x00\x00"  /* movl   $0x0,(%esp)              */
           "\xe8\xfc\xff\xff\xff"          /* call   uprintf                  */
           "\x31\xc0"                      /* xor    %eax,%eax                */
           "\x83\xc4\x04"                  /* add    $0x4,%esp                */
           "\x5d";                         /* pop    %ebp                     */

/*
 *
 * ffffffff80bd5ea0 <sys_mkdir>:
 * ffffffff80bd5ea0:   55                      push   rbp
 * ffffffff80bd5ea1:   48 89 e5                mov    rbp,rsp
 * ffffffff80bd5ea4:   48 8b 16                mov    rdx,QWORD PTR [rsi]
 * ffffffff80bd5ea7:   44 8b 46 08             mov    r8d,DWORD PTR [rsi+0x8]
 * ffffffff80bd5eab:   be 9c ff ff ff          mov    esi,0xffffff9c
 * ffffffff80bd5eb0:   31 c9                   xor    ecx,ecx
 * ffffffff80bd5eb2:   5d                      pop    rbp
 * ffffffff80bd5eb3:   eb 0b                   jmp    ffffffff80bd5ec0 <kern_mkdirat>
 * ffffffff80bd5eb5:   66 2e 0f 1f 84 00 00    nop    WORD PTR cs:[rax+rax+0x0]
 * ffffffff80bd5ebc:   00 00 00
 * ffffffff80bd5ebf:   90                      nop
 */


/*
 * 	Function implementation of inject_kmalloc.c
 * 	Also reads sys_mkdir into backup
 * 	WARNING: no checks - bad code :þ
 */
void kvm_kmalloc(kvm_t *kd, size_t size, void **addr) {
	/* Resolve symbols */
	struct nlist nl[] = {
		{ .n_name = "sys_mkdir" },
		{ .n_name = "M_TEMP"    },
		{ .n_name = "malloc"    },
		{ .n_name = "copyout"   },
		{ NULL }
	};
	kvm_nlist(kd, nl);

	/* Patch offsets in kmalloc */
	*(int *)&kmalloc[OFFSET_M_WAIT] = nl[1].n_value;
	*(int *)&kmalloc[OFFSET_MALLOC] = nl[2].n_value -
		(nl[0].n_value + OFFSET_MALLOC + sizeof(int));
	*(int *)&kmalloc[OFFSET_COPYOUT] = nl[3].n_value -
		(nl[0].n_value + OFFSET_COPYOUT + sizeof(int));

	/* Backup and replace sys_mkdir code */
	kvm_read(kd, nl[0].n_value, backup, KMALLOC_SIZE);
	kvm_write(kd, nl[0].n_value, kmalloc, KMALLOC_SIZE);

	/* Syscall mkdir to execute kmalloc */
	syscall(SYS_mkdir, (size_t) size, addr);

	/* Restore sys_mkdir */
	kvm_write(kd, nl[0].n_value, backup, KMALLOC_SIZE);
}

/*
 *	End goal:
 *	Hook syscall via kmem patching
 */
int main()
{
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	void *chunk;
	int i, call_offset;

	if ((kd = kvm_openfiles("/dev/kmem", NULL, NULL, O_RDWR, errbuf)) == NULL) {
		fprintf(stderr,
			"\033[91mError: Could not open device\033[0m\n");
	}

	/*
	 * 	Acquire chunk of kernel heap
	 * 	Also reads sys_mkdir into backup
	 */
	kvm_kmalloc(kd, 128, &chunk);
	printf("Addr of chunk: %p\n", chunk);

	/* Find offset of first call instruction */
	for (i = call_offset = 0;; i++) {
		unsigned char instruction = backup[i];
		if (instruction == 0xeb) {
			// Found
			call_offset = i;
			break;
		} else if (instruction == 0xc3) {
			// Return
			fprintf(stderr, "Error: did not find 0xeb byte!\n");
			return -1;
		}
	}

	// フックにある相対アドレスをパッチ
	*(int)&hook[OFFSET_HOOK_MSG] = chunk;
	// *(int)&hook[OFFSET_HOOK_UPRINTF] = uprinf

	// 確保すべきチャンク・サイズを把握したい


	kvm_close(kd);

	return 0;
}
