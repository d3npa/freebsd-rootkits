#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <bsm/audit_kevents.h>

struct sc_example_args {
	char *str;
};

static int
sc_example(struct thread *td, void *syscall_args)
{
	struct sc_example_args *uap;
	uap = (struct sc_example_args *) syscall_args;

	char message[0x100];
	copyinstr(uap->str, message, sizeof(message), NULL);
	printf("%s\n", message);

	return 0;
}

static int offset = NO_SYSCALL;

static struct sysent sc_example_sysent = {
	1,
	sc_example
};

static int
load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
		case MOD_LOAD:
			printf("Loaded new syscall at offset %d\n", offset);
			break;
		case MOD_UNLOAD:
			printf("Unloaded syscall number %d\n", offset);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}

SYSCALL_MODULE(sc_example, &offset, &sc_example_sysent, load, NULL);
