#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h> // For copy_to_user, copy_from_user
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/atomic.h>
#include <linux/slab.h>    // For kmalloc/kfree (if needed, though static for simplicity here)
#include <linux/ioctl.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("Concurrency Mechanisms : Mutex, Spinlock, Atomic Ops");

#define DEVICE_NAME "concurrency"
#define CLASS_NAME  "concurrency_class"

// IOCTL definitions
#define CONCURRENCY_IOC_MAGIC 'k'
#define IOCTL_SPINLOCK_INCREMENT _IO(CONCURRENCY_IOC_MAGIC, 1)
#define IOCTL_ATOMIC_INCREMENT   _IO(CONCURRENCY_IOC_MAGIC, 2)
#define IOCTL_ATOMIC_DECREMENT   _IO(CONCURRENCY_IOC_MAGIC, 3)
#define IOCTL_GET_VALUES         _IOR(CONCURRENCY_IOC_MAGIC, 4, struct shared_data_values)

static int major_number;
static struct class* concurrency_class = NULL;
static struct device* concurrency_device = NULL;
static struct cdev my_cdev;

// Shared data
static struct mutex my_mutex;
static spinlock_t my_spinlock;
static atomic_t atomic_counter;

static char shared_buffer[256] = {0};
static int mutex_protected_counter = 0;
static int spinlock_protected_counter = 0;

struct shared_data_values {
    int mutex_val;
    int spinlock_val;
    int atomic_val;
    char buffer_val[256];
};

// File operations
static int concurrency_open(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Concurrency: Device opened\n");
    return 0;
}

static int concurrency_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Concurrency: Device closed\n");
    return 0;
}

// Read: Protected by Mutex
static ssize_t concurrency_read(struct file *filp, char __user *buf, size_t len, loff_t *off) {
    int ret;
    struct shared_data_values data;

    if (mutex_lock_interruptible(&my_mutex)) {
        printk(KERN_WARNING "Concurrency: Read: Failed to acquire mutex\n");
        return -ERESTARTSYS;
    }

    printk(KERN_INFO "Concurrency: Read: Mutex acquired\n");

    data.mutex_val = mutex_protected_counter;
    data.spinlock_val = spinlock_protected_counter; // Reading this without spinlock for simplicity in this IOCTL context
    data.atomic_val = atomic_read(&atomic_counter); // Reading this without spinlock for simplicity
    strncpy(data.buffer_val, shared_buffer, sizeof(data.buffer_val) - 1);
    data.buffer_val[sizeof(data.buffer_val) -1] = '\0';


    if (len > sizeof(struct shared_data_values)) {
        len = sizeof(struct shared_data_values);
    }

    ret = copy_to_user(buf, &data, len);
    if (ret != 0) {
        printk(KERN_ERR "Concurrency: Read: Failed to copy data to user space (%d bytes)\n", ret);
        mutex_unlock(&my_mutex);
        return -EFAULT;
    }

    printk(KERN_INFO "Concurrency: Read: %zu bytes sent to user. Mutex released.\n", len);
    mutex_unlock(&my_mutex);
    return len; // Number of bytes read
}

// Write: Protected by Mutex
static ssize_t concurrency_write(struct file *filp, const char __user *buf, size_t len, loff_t *off) {
    size_t data_len;

    if (mutex_lock_interruptible(&my_mutex)) {
        printk(KERN_WARNING "Concurrency: Write: Failed to acquire mutex\n");
        return -ERESTARTSYS;
    }
    printk(KERN_INFO "Concurrency: Write: Mutex acquired\n");

    data_len = len < (sizeof(shared_buffer) - 1) ? len : (sizeof(shared_buffer) - 1);

    if (copy_from_user(shared_buffer, buf, data_len) != 0) {
        printk(KERN_ERR "Concurrency: Write: Failed to copy data from user space\n");
        mutex_unlock(&my_mutex);
        return -EFAULT;
    }
    shared_buffer[data_len] = '\0'; // Null-terminate

    mutex_protected_counter++;

    printk(KERN_INFO "Concurrency: Write: Received %zu bytes. Mutex counter: %d. Mutex released.\n", data_len, mutex_protected_counter);
    mutex_unlock(&my_mutex);
    return data_len; // Number of bytes written
}

// IOCTL: For spinlock and atomic operations
static long concurrency_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    unsigned long flags;
    struct shared_data_values data_to_user;

    switch (cmd) {
        case IOCTL_SPINLOCK_INCREMENT:
            spin_lock_irqsave(&my_spinlock, flags);
            spinlock_protected_counter++;
            printk(KERN_INFO "Concurrency: IOCTL_SPINLOCK_INCREMENT: Counter is %d\n", spinlock_protected_counter);
            spin_unlock_irqrestore(&my_spinlock, flags);
            break;

        case IOCTL_ATOMIC_INCREMENT:
            atomic_inc(&atomic_counter);
            printk(KERN_INFO "Concurrency: IOCTL_ATOMIC_INCREMENT: Counter is %d\n", atomic_read(&atomic_counter));
            break;

        case IOCTL_ATOMIC_DECREMENT:
            atomic_dec(&atomic_counter);
            printk(KERN_INFO "Concurrency: IOCTL_ATOMIC_DECREMENT: Counter is %d\n", atomic_read(&atomic_counter));
            break;

        case IOCTL_GET_VALUES:
            // For spinlock_protected_counter, ideally, you'd acquire the spinlock
            // if you need a consistent read with other spinlock-protected operations.
            // Here, we read it directly for simplicity, which might be racy if writes are frequent.
            spin_lock_irqsave(&my_spinlock, flags);
            data_to_user.spinlock_val = spinlock_protected_counter;
            spin_unlock_irqrestore(&my_spinlock, flags);

            // Mutex protected data is read without the mutex here for simplicity in IOCTL.
            // A more robust implementation might acquire the mutex or have specific IOCTLs.
            if (mutex_lock_interruptible(&my_mutex)) return -ERESTARTSYS;
            data_to_user.mutex_val = mutex_protected_counter;
            strncpy(data_to_user.buffer_val, shared_buffer, sizeof(data_to_user.buffer_val) -1);
            data_to_user.buffer_val[sizeof(data_to_user.buffer_val)-1] = '\0';
            mutex_unlock(&my_mutex);
            
            data_to_user.atomic_val = atomic_read(&atomic_counter);

            if (copy_to_user((struct shared_data_values __user *)arg, &data_to_user, sizeof(struct shared_data_values))) {
                return -EFAULT;
            }
            break;

        default:
            printk(KERN_WARNING "Concurrency: IOCTL: Unknown command %u\n", cmd);
            return -EINVAL;
    }
    return 0;
}

static const struct file_operations fops = {
    .owner = THIS_MODULE,
    .open = concurrency_open,
    .release = concurrency_release,
    .read = concurrency_read,
    .write = concurrency_write,
    .unlocked_ioctl = concurrency_ioctl,
};

static int __init concurrency_init(void) {
    printk(KERN_INFO "Concurrency: Initializing module\n");

    // 1. Allocate major number
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Concurrency: Failed to register a major number\n");
        return major_number;
    }
    printk(KERN_INFO "Concurrency: Registered correctly with major number %d\n", major_number);

    // 2. Create device class
    concurrency_class = class_create(CLASS_NAME); // Corrected call
    if (IS_ERR(concurrency_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Concurrency: Failed to register device class\n");
        return PTR_ERR(concurrency_class);
    }
    printk(KERN_INFO "Concurrency: Device class registered correctly\n");

    // 3. Create device
    concurrency_device = device_create(concurrency_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(concurrency_device)) {
        class_destroy(concurrency_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ALERT "Concurrency: Failed to create the device\n");
        return PTR_ERR(concurrency_device);
    }
    printk(KERN_INFO "Concurrency: Device created correctly\n");

    // Initialize mutex, spinlock, and atomic variable
    mutex_init(&my_mutex);
    spin_lock_init(&my_spinlock);
    atomic_set(&atomic_counter, 0);

    printk(KERN_INFO "Concurrency: Mutex, Spinlock, and Atomic Counter initialized.\n");
    printk(KERN_INFO "Concurrency: Module loaded successfully.\n");
    printk(KERN_INFO "Concurrency: Create device node with: sudo mknod /dev/%s c %d 0\n", DEVICE_NAME, major_number);
    return 0;
}

static void __exit concurrency_exit(void) {
    printk(KERN_INFO "Concurrency: Exiting module\n");

    device_destroy(concurrency_class, MKDEV(major_number, 0));
    class_unregister(concurrency_class);
    class_destroy(concurrency_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    // Mutex is destroyed automatically if statically allocated and init/exit are module-scoped.
    // No explicit destroy for spinlock_t or atomic_t needed.

    printk(KERN_INFO "Concurrency: Module unloaded successfully.\n");
}

module_init(concurrency_init);
module_exit(concurrency_exit);
