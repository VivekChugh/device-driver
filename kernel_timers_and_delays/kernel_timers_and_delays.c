#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/delay.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("Kernel Timers and Delays Example");
MODULE_VERSION("1.0");

static struct timer_list my_timer;

// Timer callback function
void timer_callback(struct timer_list *timer) {
    printk(KERN_INFO "Timer callback executed at jiffies: %lu\n", jiffies);
}

static int __init timer_delay_init(void) {
    printk(KERN_INFO "Initializing Kernel Timer and Delay Module\n");

    // Initialize the timer
    timer_setup(&my_timer, timer_callback, 0);

    // Set the timer to expire after 2 seconds (2 * HZ jiffies)
    mod_timer(&my_timer, jiffies + 2 * HZ);

    // Demonstrate short delays
    printk(KERN_INFO "Sleeping for 1 second using msleep\n");
    msleep(1000);

    printk(KERN_INFO "Sleeping for 500 milliseconds using mdelay\n");
    mdelay(500);

    printk(KERN_INFO "Kernel Timer and Delay Module Loaded\n");
    return 0;
}

static void __exit timer_delay_exit(void) {
    printk(KERN_INFO "Exiting Kernel Timer and Delay Module\n");

    // Delete the timer if it is still active
    del_timer(&my_timer);

    printk(KERN_INFO "Kernel Timer and Delay Module Unloaded\n");
}

module_init(timer_delay_init);
module_exit(timer_delay_exit);