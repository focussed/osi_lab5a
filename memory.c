/*
 * Memory Character Driver - Lab 5 Solution
 * Combines Part 1 (automatic device creation) and Part 2 (5-byte storage)
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/uaccess.h>  // for copy_to_user/copy_from_user

#define DEVICE_NAME "memory"
#define BUFFER_SIZE 5        // Store 5 bytes as required in Part 2

static int major_number;     // Will be dynamically assigned (Part 1)
static struct class *memory_class = NULL;
static struct device *memory_device = NULL;
static struct cdev memory_cdev;

// The memory buffer that stores our data (now 5 bytes as per Part 2)
static char memory_buffer[BUFFER_SIZE];

// Track how many bytes are actually stored in the buffer
static size_t data_size = 0;

/**
 * memory_open - Called when device is opened
 */
static int memory_open(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "memory: Device opened\n");
    return 0;
}

/**
 * memory_release - Called when device is closed
 */
static int memory_release(struct inode *inode, struct file *filp)
{
    printk(KERN_INFO "memory: Device closed\n");
    return 0;
}

/**
 * memory_read - Read data from the device
 * @filp: File structure
 * @buf: User buffer to copy data to
 * @count: Number of bytes requested
 * @f_pos: File position
 * 
 * Returns: Number of bytes read, or negative error
 */
static ssize_t memory_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    size_t available_bytes;
    size_t bytes_to_read;
    
    printk(KERN_INFO "memory: Read request - count=%zu, f_pos=%lld, data_size=%zu\n", 
           count, *f_pos, data_size);
    
    // Check if we've already read all data
    if (*f_pos >= data_size) {
        printk(KERN_INFO "memory: No more data to read (EOF)\n");
        return 0;  // EOF
    }
    
    // Calculate how many bytes we can actually read
    available_bytes = data_size - *f_pos;
    bytes_to_read = count;
    
    // Don't read more than available
    if (bytes_to_read > available_bytes) {
        bytes_to_read = available_bytes;
        printk(KERN_INFO "memory: Limiting read to %zu available bytes\n", bytes_to_read);
    }
    
    // Don't exceed our buffer size (safety check)
    if (bytes_to_read > BUFFER_SIZE) {
        bytes_to_read = BUFFER_SIZE;
        printk(KERN_INFO "memory: Limiting read to buffer size %d\n", BUFFER_SIZE);
    }
    
    // Copy data from kernel space to user space
    if (copy_to_user(buf, memory_buffer + *f_pos, bytes_to_read)) {
        printk(KERN_ERR "memory: Failed to copy data to user\n");
        return -EFAULT;
    }
    
    // Update file position
    *f_pos += bytes_to_read;
    
    printk(KERN_INFO "memory: Successfully read %zu bytes, new f_pos=%lld\n", 
           bytes_to_read, *f_pos);
    
    return bytes_to_read;
}

/**
 * memory_write - Write data to the device
 * @filp: File structure
 * @buf: User buffer containing data to write
 * @count: Number of bytes to write
 * @f_pos: File position
 * 
 * Returns: Number of bytes written, or negative error
 */
static ssize_t memory_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    size_t bytes_to_write;
    
    printk(KERN_INFO "memory: Write request - count=%zu, f_pos=%lld\n", count, *f_pos);
    
    // We only support writing at the beginning (position 0) for simplicity
    if (*f_pos != 0) {
        printk(KERN_INFO "memory: Write position must be 0, ignoring\n");
        return -EINVAL;
    }
    
    // Determine how many bytes to write (max BUFFER_SIZE as per Part 2)
    bytes_to_write = (count > BUFFER_SIZE) ? BUFFER_SIZE : count;
    printk(KERN_INFO "memory: Will write %zu bytes (limited from %zu)\n", bytes_to_write, count);
    
    // Copy data from user space to kernel space
    if (copy_from_user(memory_buffer, buf, bytes_to_write)) {
        printk(KERN_ERR "memory: Failed to copy data from user\n");
        return -EFAULT;
    }
    
    // Update data size and file position
    data_size = bytes_to_write;
    *f_pos = bytes_to_write;
    
    printk(KERN_INFO "memory: Successfully wrote %zu bytes\n", bytes_to_write);
    printk(KERN_INFO "memory: Buffer contents: %.*s\n", (int)bytes_to_write, memory_buffer);
    
    return bytes_to_write;
}

// File operations structure - defines which functions handle device operations
static struct file_operations memory_fops = {
    .owner = THIS_MODULE,
    .open = memory_open,
    .release = memory_release,
    .read = memory_read,
    .write = memory_write,
};

/**
 * memory_init - Module initialization function
 * Called when module is loaded (insmod)
 * 
 * Part 1: Dynamically allocates major number and creates /dev/memory
 */
static int __init memory_init(void)
{
    dev_t dev_num;
    int result;
    
    printk(KERN_INFO "memory: Initializing memory driver\n");
    
    // PART 1: Dynamically allocate a major number
    result = alloc_chrdev_region(&dev_num, 0, 1, DEVICE_NAME);
    if (result < 0) {
        printk(KERN_ERR "memory: Failed to allocate major number\n");
        return result;
    }
    
    major_number = MAJOR(dev_num);
    printk(KERN_INFO "memory: Allocated major number %d\n", major_number);
    
    // Initialize the character device structure
    cdev_init(&memory_cdev, &memory_fops);
    memory_cdev.owner = THIS_MODULE;
    
    // Add the character device to the system
    result = cdev_add(&memory_cdev, dev_num, 1);
    if (result < 0) {
        printk(KERN_ERR "memory: Failed to add cdev\n");
        unregister_chrdev_region(dev_num, 1);
        return result;
    }
    
    // Create device class (appears in /sys/class)
    memory_class = class_create(THIS_MODULE, DEVICE_NAME);
    if (IS_ERR(memory_class)) {
        printk(KERN_ERR "memory: Failed to create class\n");
        cdev_del(&memory_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(memory_class);
    }
    
    // Create the device file /dev/memory (Part 1 - automatic creation)
    memory_device = device_create(memory_class, NULL, dev_num, NULL, DEVICE_NAME);
    if (IS_ERR(memory_device)) {
        printk(KERN_ERR "memory: Failed to create device\n");
        class_destroy(memory_class);
        cdev_del(&memory_cdev);
        unregister_chrdev_region(dev_num, 1);
        return PTR_ERR(memory_device);
    }
    
    // Initialize buffer to zeros (safety)
    memset(memory_buffer, 0, BUFFER_SIZE);
    data_size = 0;
    
    printk(KERN_INFO "memory: Driver initialized successfully\n");
    printk(KERN_INFO "memory: Device /dev/%s created with major number %d\n", 
           DEVICE_NAME, major_number);
    
    return 0;
}

/**
 * memory_exit - Module cleanup function
 * Called when module is removed (rmmod)
 * 
 * Part 1: Cleans up and removes /dev/memory
 */
static void __exit memory_exit(void)
{
    dev_t dev_num = MKDEV(major_number, 0);
    
    printk(KERN_INFO "memory: Cleaning up memory driver\n");
    
    // Remove the device file (Part 1 - automatic removal)
    if (memory_device) {
        device_destroy(memory_class, dev_num);
        printk(KERN_INFO "memory: Removed /dev/%s\n", DEVICE_NAME);
    }
    
    // Destroy the class
    if (memory_class) {
        class_destroy(memory_class);
        printk(KERN_INFO "memory: Destroyed device class\n");
    }
    
    // Remove the character device
    cdev_del(&memory_cdev);
    
    // Unregister the major number
    unregister_chrdev_region(dev_num, 1);
    
    printk(KERN_INFO "memory: Driver removed, major number %d freed\n", major_number);
    printk(KERN_INFO "memory: Goodbye!\n");
}

// Register the init and exit functions
module_init(memory_init);
module_exit(memory_exit);

// Module information
MODULE_LICENSE("GPL");
MODULE_AUTHOR("OSI Lab 5 Student");
MODULE_DESCRIPTION("Memory driver that stores 5 bytes and auto-creates device file");
MODULE_VERSION("1.0");