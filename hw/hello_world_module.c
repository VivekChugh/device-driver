#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("A simple Hello World Kernel Module");
MODULE_VERSION("1.0");

// Function executed when the module is loaded
static int __init hello_init(void) {
    printk(KERN_INFO "Hello, World! Kernel Module Loaded.\n");
    return 0;
}

// Function executed when the module is removed
static void __exit hello_exit(void) {
    printk(KERN_INFO "Goodbye, World! Kernel Module Unloaded.\n");
}

module_init(hello_init);
module_exit(hello_exit);