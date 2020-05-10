#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>

#include <bsm/audit_kevents.h>

struct kmalloc_args {
	void *ptr;
	size_t size;
};

static int kmalloc_handler(struct thread *td, void *arg)
{
	struct kmalloc_args *uap;
	uap = (struct kmalloc_args *)arg;

	uprintf("Pointer: %p\nSize: %zu\n", uap->ptr, uap->size);

	return 0;
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
