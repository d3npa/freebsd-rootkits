#include <sys/param.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/kernel.h>

static void greeting() {
	char *message = "Hello, world!";
	uprintf("%s (addr: %p)\n", message, message);
}

// define load function
static int
load(struct module *module, int cmd, void *args)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		greeting();
		break;

	case MOD_UNLOAD:
		uprintf("Goodbye, cruel world...\n");
		break;

	default:
		error = EOPNOTSUPP;
		break;

	}

	return error;
}

static moduledata_t hello_mod = {
	"hello",
	load,
	NULL
};

DECLARE_MODULE(hello, hello_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
