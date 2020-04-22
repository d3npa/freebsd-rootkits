#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/ip.h> // これまだ紐づけあるかも
#include <netinet/ip_var.h> // ip_protox はここ！！
#include <netinet/udp.h> 		// 中にstruct udphdrの定義ある
#include <netinet/udp_var.h> // udp_input プロトタイプ

// ip_protoxってどこで定義されるんだっけ

extern struct protosw inetsw[];
int udp_input_hook(struct mbuf **, int *, int);

int
udp_input_hook(struct mbuf **mp, int *offp, int proto)
{
	/*
		mbufの構成:
		[ struct ip ] [ struct udphdr ] [ data ]
		              ↑ *offp

		つまりdataを取得するには*offpにstruct udphdrの長さを足せばいいです。
	*/

	int iphlen = *offp;
	int uhlen = sizeof(struct udphdr);

	struct mbuf *mb = *mp;
	struct ip *ip = mtod(mb, struct ip *);
	struct udphdr *uh = (struct udphdr *) ((caddr_t) ip + iphlen);
	char *data = (char *) ((caddr_t) uh + uhlen);

	if (ntohs(uh->uh_dport) == 5000)
		printf("User Data: %s\n", data);

	return udp_input(mp, offp, proto);
}

static int
load (struct module *module, int cmd, void *arg)
{
	int error = 0;

	switch (cmd) {
		case MOD_LOAD:
			inetsw[ip_protox[IPPROTO_UDP]].pr_input = udp_input_hook;
			printf("UDP hook loaded!\n");
			break;
		case MOD_UNLOAD:
			inetsw[ip_protox[IPPROTO_UDP]].pr_input = udp_input;
			printf("UDP hook unloaded!\n");
			break;
		default:
			error = EOPNOTSUPP;
	}

	return error;
}

static moduledata_t mod_data = { "udp_hook", load, NULL };
DECLARE_MODULE(udp_hook, mod_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
