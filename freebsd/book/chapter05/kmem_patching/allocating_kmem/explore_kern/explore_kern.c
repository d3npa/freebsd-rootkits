#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/module.h>
#include <sys/sysent.h>
#include <sys/types.h>
#include <sys/malloc.h>
#include <sys/cdefs.h>

#include <bsm/audit_kevents.h>

#include <vm/vm.h>
#include <vm/pmap.h>
#include <vm/vm_kern.h>
#include <vm/vm_map.h>
#include <vm/vm_extern.h>

static int explore_kern(struct thread *td, void *arg)
{
	char c;
	unsigned long va = 0xffffffff80cae970;

	if (kernacc((void *) va, 1, VM_PROT_READ) == TRUE) {
		uprintf("Can read\n");
	}

	if (kernacc((void *) va, 1, VM_PROT_WRITE) == TRUE) {
		uprintf("Can write\n");
	}


	c = *(char *)va;
	uprintf("mem before write %c\n", *(char *)va);
	vm_map_lock_read(kernel_map);

	vm_map_entry_t entry;
	vm_map_entry_t tmp_entry;

	if (!vm_map_lookup_entry(kernel_map, va, &tmp_entry))
		return (FALSE);
	entry = tmp_entry;

	uprintf("Permissions on %p: ", (void *) va);
	if ((entry->protection & VM_PROT_READ) > 0)
		uprintf("r");
	if ((entry->protection & VM_PROT_WRITE) > 0)
		uprintf("w");
	if ((entry->protection & VM_PROT_EXECUTE) > 0)
		uprintf("x");
	uprintf("\n");

	/*
		I have rwx...
		why does uiomove not work then
		I need to investigate uiomove...

		(I tried to write directly but it panicked)
		Uiomove must have a check that prevents a panic
		I want to know why I can't write to this memory
	*/

	vm_map_unlock_read(kernel_map);
	return 0;
}

static int offset = NO_SYSCALL;
static struct sysent explore_kern_sysent = {
	0,
	explore_kern
};

static int load(struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		printf("Installed syscall 'explore_kern' at offset %d\n", offset);
		break;
	case MOD_UNLOAD:
		printf("Uninstalled syscall 'explore_kern'\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

SYSCALL_MODULE(explore_kern, &offset, &explore_kern_sysent, load, NULL);
