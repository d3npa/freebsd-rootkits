#!/bin/sh
make
kldload ./explore_kern.ko
perl -e 'syscall(210);'
kldunload ./explore_kern.ko
