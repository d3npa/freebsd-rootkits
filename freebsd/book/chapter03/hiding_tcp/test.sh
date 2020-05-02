#!/bin/sh

make || exit
kldload ./hiding_tcp.ko || exit
echo -e '\033[92m'
perl -e 'syscall(210, 4444);'
echo -e '\033[0m'
kldunload ./hiding_tcp.ko
