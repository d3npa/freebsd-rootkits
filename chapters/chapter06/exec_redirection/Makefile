SRCS=execve_hook.c
KMOD=execve_hook

.include <bsd.kmod.mk>

all:
	cc -o /tmp/evil evil.c
	cc -o /tmp/good good.c
