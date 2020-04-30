#include <sys/types.h>
#include <sys/systm.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/conf.h>
#include <sys/uio.h>

#define BUFSIZE 0x100
char buffer[BUFSIZE];

d_open_t 	open;
d_close_t 	close;
d_read_t	read;

static struct cdevsw cd_example_cdevsw = {
	.d_version = 	D_VERSION,
	.d_name = 	"cd_example",
	.d_open = 	open,
	.d_close = 	close,
	.d_read = 	read
};

int 
open(struct cdev *dev, int oflags, int devtype, struct thread *td)
{
	return 0;
}

int
close(struct cdev *dev, int fflag, int devtype, struct thread *td)
{
	return 0;
}

int
read(struct cdev *dev, struct uio *uio, int ioflag)
{ 
	uiomove_frombuf(buffer, BUFSIZE, uio);
	return 0;
}

static struct make_dev_args args;
static struct cdev *dev; // sdevってどういういみ？

static int
load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch(cmd) {
		case MOD_LOAD:
			make_dev_args_init(&args);
			args.mda_devsw = &cd_example_cdevsw;
			args.mda_flags = MAKEDEV_CHECKNAME;
			args.mda_uid = args.mda_gid = 0;
			args.mda_mode = 0600;
			make_dev_s(&args, &dev, "cd_example");
			copystr("Hello, world!\n", buffer, BUFSIZE, NULL);
			break;
		case MOD_UNLOAD:
			destroy_dev(dev);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}

DEV_MODULE(cd_example, load, NULL);
