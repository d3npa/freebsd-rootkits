#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/module.h>
#include <sys/types.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/protosw.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/ip_icmp.h>

#define TRIGGER "Shiny."
extern struct protosw inetsw[];

int icmp_input_hook(struct mbuf **, int *, int); // the compiler really wants this prototype

int icmp_input_hook(struct mbuf **mp, int *offp, int proto) {
	struct mbuf *mb = *mp;
	int hlen = *offp;

	// adjust m_data and m_len before the mtod cast
	mb->m_data += hlen;
	mb->m_len -= hlen;

	// mtod casts things for us
	// TODO: pick a better variable name
	struct icmp *icmpdata = mtod(mb, struct icmp*);

	// restore original mb values
	mb->m_data -= hlen;
	mb->m_len += hlen;

	// compare ICMP data with trigger
	char buffer[6+1];
	strncpy(buffer, icmpdata->icmp_data, 6);
	buffer[sizeof(buffer) - 1] = 0;

	if (buffer[0] != 0) {
		printf("Payload of our ICMP packet: %s\n", buffer);
		// super risky printf....
	}

	if (strncmp(icmpdata->icmp_data, TRIGGER, 6) == 0) {
		printf("Our shiny packet!!\n");
	}

	return icmp_input(mp, offp, proto);
}

static int load(struct module *module, int cmd, void *arg) {
	int error = 0;

	switch (cmd) {
		case MOD_LOAD:
			// replace icmp_input pointer in inetsw
			inetsw[ip_protox[IPPROTO_ICMP]].pr_input = icmp_input_hook;
			uprintf("Loaded ICMP hook\n");
			break;
		case MOD_UNLOAD:
			// restore original icmp_input
			inetsw[ip_protox[IPPROTO_ICMP]].pr_input = icmp_input;
			uprintf("Unloaded ICMP hook\n");
			break;
		default:
			error = EOPNOTSUPP;
	}

	return error;
}

static moduledata_t mod_data = { "icmp_hook", load, NULL };
DECLARE_MODULE(icmp_hook, mod_data, SI_SUB_DRIVERS, SI_ORDER_MIDDLE);
