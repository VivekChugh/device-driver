#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>

#define DEVICE_NAME "char_device"
#define BUFFER_SIZE 1024

static int major_number;
static char device_buffer[BUFFER_SIZE];
static int open_count = 0;
static dev_t dev_number;
static struct cdev char_cdev;

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
    int ret;

    // Allocate a device number
    ret = alloc_chrdev_region(&dev_number, 0, 1, DEVICE_NAME);
    if (ret < 0) {
        printk(KERN_ALERT "Failed to allocate a major number\n");
        return ret;
    }

    // Initialize the cdev structure and add it to the kernel
    cdev_init(&char_cdev, &fops);
    char_cdev.owner = THIS_MODULE;
    ret = cdev_add(&char_cdev, dev_number, 1);
    if (ret < 0) {
        unregister_chrdev_region(dev_number, 1);
        printk(KERN_ALERT "Failed to add cdev\n");
        return ret;
    }

    printk(KERN_INFO "Char device registered with major number %d and minor number %d\n", MAJOR(dev_number), MINOR(dev_number));
    return 0;
}

// Module cleanup
static void __exit char_device_exit(void) {
    // Remove the cdev from the kernel
    cdev_del(&char_cdev);

    // Unregister the device number
    unregister_chrdev_region(dev_number, 1);

    printk(KERN_INFO "Char device unregistered\n");
}

module_init(char_device_init);
module_exit(char_device_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Vivek");
MODULE_DESCRIPTION("A simple character device driver");
MODULE_VERSION("1.0");