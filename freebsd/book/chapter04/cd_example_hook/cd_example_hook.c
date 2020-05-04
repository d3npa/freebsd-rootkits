#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/conf.h>
#include <sys/queue.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/uio.h>

#include <fs/devfs/devfs_int.h>

#define _REPLACE "Kernel hacking is hard"
#define _WITH "Kernel hacking is fun"

d_read_t read_hook;
d_read_t *read;

void patch_uio(char *buffer, long lbuffer);

void patch_uio(char *buffer, long lbuffer)
{
	int lwith = strlen(_WITH);
	int lreplace = strlen(_REPLACE);
	int shift = lwith - lreplace;         /* ズレ */

	strcpy(buffer, _WITH);
	if (shift < 0) {
		/* _WITHが_REPLACEより短い場合
		   バッファーの最後までゼロを書き込む */
	   	memcpy(buffer + lwith, buffer + lreplace, lbuffer - lreplace);
		memset(buffer + lbuffer + shift, 0, -shift);
	} else {
		/* _WITHが_REPLACEより長い場合
		   バッファーを越えないようにコピーの長さを調整する */
		memcpy(buffer + lwith, buffer + lreplace, lbuffer - lwith);
	}
}

int read_hook(struct cdev *dev, struct uio *uio, int ioflag)
{
	int result = (*read)(dev, uio, ioflag);

	// 読み込みの直後、読み込まれたデータを取得
	char buffer[uio->uio_offset];
	copyin(((char *)uio->uio_iov->iov_base) - uio->uio_offset,
			buffer, uio->uio_offset);

	if (strncmp(buffer, _REPLACE, strlen(_REPLACE)) == 0) {
		patch_uio(buffer, uio->uio_offset);
		copyout(buffer, ((char *)uio->uio_iov->iov_base) - uio->uio_offset,
				uio->uio_offset);
	}

	return result;
}


static int load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	struct cdev_priv *cdp;

	switch(cmd) {
	case MOD_LOAD:
		mtx_lock(&devmtx);
		TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
			if (strncmp(cdp->cdp_c.si_name, "cd_example", 10) == 0) {
				read = cdp->cdp_c.si_devsw->d_read;
				cdp->cdp_c.si_devsw->d_read = read_hook;
				break;
			}
		}
		mtx_unlock(&devmtx);
		break;
	case MOD_UNLOAD:
		mtx_lock(&devmtx);
		TAILQ_FOREACH(cdp, &cdevp_list, cdp_list) {
			if (strncmp(cdp->cdp_c.si_name, "cd_example", 10) == 0) {
				cdp->cdp_c.si_devsw->d_read = read;
				break;
			}
		}
		mtx_unlock(&devmtx);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static moduledata_t cd_example_hook_data = {
	"cd_example_hook",
	load,
	NULL
};

DECLARE_MODULE(cd_example_hook, cd_example_hook_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
