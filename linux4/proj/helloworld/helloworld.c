#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static int __init helloworld_init(void)
{
	printk(KERN_INFO "Hello world!\n");
	return 0;
}

static void __exit helloworld_exit(void)
{
	printk(KERN_INFO "Goodbye, cruel world!\n");
}


module_init(helloworld_init);
module_exit(helloworld_exit);
