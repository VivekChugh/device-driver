#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DEVICE_NAME "char_device"
#define BUFFER_SIZE 1024

static int major_number;
static char device_buffer[BUFFER_SIZE];
static int open_count = 0;

// Open function
static int device_open(struct inode *inode, struct file *file) {
    open_count++;
    printk(KERN_INFO "Device opened %d time(s)\n", open_count);
    return 0;
}

// Read function
static ssize_t device_read(struct file *file, char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_read = min(count, (size_t)(BUFFER_SIZE - *offset));
    if (bytes_to_read == 0) {
        printk(KERN_INFO "Reached end of device\n");
        return 0;
    }

    if (copy_to_user(user_buffer, device_buffer + *offset, bytes_to_read)) {
        return -EFAULT;
    }

    *offset += bytes_to_read;
    printk(KERN_INFO "Read %zu bytes from device\n", bytes_to_read);
    return bytes_to_read;
}

// Write function
static ssize_t device_write(struct file *file, const char __user *user_buffer, size_t count, loff_t *offset) {
    size_t bytes_to_write = min(count, (size_t)(BUFFER_SIZE - *offset));
    if (bytes_to_write == 0) {
        printk(KERN_INFO "No space left on device\n");
        return -ENOSPC;
    }

    if (copy_from_user(device_buffer + *offset, user_buffer, bytes_to_write)) {
        return -EFAULT;
    }

    *offset += bytes_to_write;
    printk(KERN_INFO "Wrote %zu bytes to device\n", bytes_to_write);
    return bytes_to_write;
}

// Release function
static int device_release(struct inode *inode, struct file *file) {
    printk(KERN_INFO "Device closed\n");
    return 0;
}

// File operations structure
static struct file_operations fops = {
    .open = device_open,
    .read = device_read,
    .write = device_write,
    .release = device_release,
};

// Module initialization
static int __init char_device_init(void) {
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ALERT "Failed to register a major number\n");
        return major_number;
    }

    printk(KERN_INFO "Registered char device with major number %d\n", major_number);
    return 0;
}

// Module cleanup
static void __exit char_device_exit(void) {
    unregister_chrdev(major_number, DEVICE_NAME);
    printk(KERN_INFO "Char device unregistered\n");
}

module_init(char_device_init);
module_exit(char_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("1.0");