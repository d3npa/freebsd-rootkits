#include <bsm/audit_kevents.h>

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>

static int annoying_hello(struct thread *td, void *arg)
{
	for (int i = 0; i < 10; i++)
		printf("Hello FreeBSD!\n");

	return 0;
}

static int offset = NO_SYSCALL;

static int load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		uprintf("Loaded annoying_hello at offset %d\n", offset);
		break;
	case MOD_UNLOAD:
		uprintf("Unloaded annoying_hello\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static struct sysent annoying_hello_sysent = {
	0,
	annoying_hello
};

SYSCALL_MODULE(annoying_hello, &offset, &annoying_hello_sysent, load, NULL);
