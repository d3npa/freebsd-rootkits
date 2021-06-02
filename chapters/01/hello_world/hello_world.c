#include <sys/param.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/kernel.h>

static void greeting() {
	// Print message and its address because it's cool!
	char *message = "Hello, world!";
	printf("[Addr: %p] %s\n", message, message);
}

// Define what happens when the module is loaded
static int
load(struct module *module, int cmd, void *args)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		greeting();
		break;

	case MOD_UNLOAD:
		printf("Goodbye, cruel world...\n");
		break;

	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static moduledata_t hello_mod = {
	"hello_world",
	load,
	NULL
};

DECLARE_MODULE(hello, hello_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
