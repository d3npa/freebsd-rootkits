#!/bin/sh

cd /home/vagrant/book/chapter01/chardev
if [ ! -e cd_example.ko ]
then
	make
	kldload ./cd_example.ko
fi
cd -

echo 'Kernel hacking is hard!' > /dev/cd_example

make || exit
kldload ./cd_example_hook.ko || exit
echo -e '\033[92m'
cat /dev/cd_example
echo -e '\033[0m'
kldunload ./cd_example_hook.ko
