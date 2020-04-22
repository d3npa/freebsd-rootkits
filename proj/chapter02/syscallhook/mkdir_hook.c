#include <sys/param.h> 
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>
#include <sys/sysent.h>

static int 
mkdir_hook(struct thread *td, struct mkdir_args *args) 
{
	struct mkdir_args *uap = (struct mkdir_args *) args;

	char path[0x100];
	int error = copyinstr(uap->path, path, 0x100, NULL);
	if (error != 0)
		return error;

	uprintf("Creating new directory '%s' with permissions %o\n", path, uap->mode);
	return sys_mkdir(td, args);
}

static int
load(struct module *module, int cmd, void *args) 
{
	int error = 0;

	switch(cmd) {
		case MOD_LOAD:
			sysent[SYS_mkdir].sy_call = (sy_call_t *) mkdir_hook;
			break;
		case MOD_UNLOAD:
			sysent[SYS_mkdir].sy_call = (sy_call_t *) sys_mkdir;
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}

static moduledata_t mod = {
	"mkdir_hook",
	load,
	NULL
};

DECLARE_MODULE(mkdir_hook, mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
