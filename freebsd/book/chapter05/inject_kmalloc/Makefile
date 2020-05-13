IN = inject_kmalloc.c
OUT = inject_kmalloc
CFLAGS += -lkvm

all: inject_kmalloc

kmem_kmalloc:
	cc ${CFLAGS} -o ${OUT} ${IN}
