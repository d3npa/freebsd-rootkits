#include <stdio.h>
#include <sys/param.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <unistd.h>

int main()
{
	void *addr;
	int syscall_num;
	struct module_stat stat;

	stat.version = sizeof(stat);
	modstat(modfind("kmalloc"), &stat);
	syscall_num = stat.data.intval;

	/*
	 * 何故かどうしてもmodfindはモジュールを見つけれない (-1を返す)
	 */
	syscall_num = 210;

	printf("Calling syscall %d\n", syscall_num);
	syscall(syscall_num, (size_t) 128, &addr);
	printf("Address of allocated kernel memory: %p\n", addr);

	return 0;
}
