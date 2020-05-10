#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/types.h>
#include <sys/malloc.h>

#include <bsm/audit_kevents.h>

struct kmalloc_args {
	size_t size;
	void *ptr;
};

static int kmalloc_handler(struct thread *td, void *arg)
{
	int error;
	void *ptr;
	struct kmalloc_args *uap;
	uap = (struct kmalloc_args *)arg;

	ptr = malloc(uap->size, M_TEMP, M_ZERO | M_WAITOK);
	error = copyout(&ptr, uap->ptr, sizeof(void *));

	return error;
}

static int offset = NO_SYSCALL;
static struct sysent kmalloc_sysent = {
	2,
	kmalloc_handler
};

static int load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		printf("Installed syscall 'kmalloc' at offset %d\n", offset);
		break;
	case MOD_UNLOAD:
		printf("Uninstalled syscall 'kmalloc'\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

SYSCALL_MODULE(kmalloc, &offset, &kmalloc_sysent, load, NULL);
