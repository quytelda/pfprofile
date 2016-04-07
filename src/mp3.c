/**
 * mp3.c - MP3 Kernel Module
 * Copyright (C) 2016 Quytelda Kahja <quytelda@tamalin.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/cdev.h>
#include "task.h"
#include "dispatch.h"
#include "mem.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quytelda Kahja");
MODULE_DESCRIPTION("Rate Monotonic Scheduler");

#define DEBUG 1
#define PROC_DIR   "mp3"
#define PROC_ENTRY "status"
#define DEV_NAME   "node"

void * buffer;

static ssize_t mp2_read(struct file * file, char __user * buf,
			size_t length, loff_t * offset)
{
    // we have already sent all of our output
    // all output is sent in one chunk
    if(*offset > 0) return 0;

    char * tasklist_str = tasklist_to_str();
    if(!tasklist_str) return 0;

    int len = strlen(tasklist_str);
    if(!copy_to_user(buf, tasklist_str, len))
    {
	*offset += len;
	return len;
    }
    else
    {
	printk(KERN_ERR "mp3: could not copy /proc read data to userspace.\n");
	return 0;
    }

    kfree(tasklist_str);
}

static ssize_t mp2_write(struct file * file, const char * buf,
			 size_t length, loff_t * offset)
{
    char * input = kmalloc(length + 1, GFP_KERNEL);
    if(!copy_from_user(input, buf, length))
    {
	// add null terminator to the input string
	*(input + length) = 0;

	int error = dispatch(input);
	if(error)
	    printk(KERN_ERR "mp3: Dispatch failed (error %d).)\n", error);
    }
    else
    {
	printk(KERN_ERR "mp3: Could not copy /proc write data from userspace.\n");
    }

    kfree(input);
    return length;
}

static int device_mmap(struct file * file, struct vm_area_struct * vma)
{
    long vm_start = vma->vm_start;
    long vm_size = vma->vm_end - vm_start;

    int res;
    long offset = 0;
    void * ph_addr = buffer;
    unsigned long pfn, vm_addr;
    for(int i = 0; (i < NUM_PAGES) && (offset < vm_size); i++)
    {
	offset = i * PAGE_SIZE;

	ph_addr = buffer + offset;
	pfn = vmalloc_to_pfn(ph_addr);

	vm_addr = vm_start + offset;
	if((res = remap_pfn_range(vma, vm_addr, pfn, PAGE_SIZE, PAGE_SHARED)) > 0)
	    return res;
    }

    return 0;
}

static struct proc_dir_entry * proc_dir;
static struct proc_dir_entry * proc_entry;
static const struct file_operations proc_fops =
{
    .owner = THIS_MODULE,
    .read  = mp2_read,
    .write = mp2_write,
};

static dev_t dev_num;
static struct cdev * dev;
static const struct file_operations dev_fops =
{
    .owner   = THIS_MODULE,
    .mmap    = device_mmap,
};

int __init mp2_init(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Loading MP3 module...\n");
#endif
    // initialize the registration list
    init_tasklist();

    // setup shared user-space memory buffer
    buffer = vmalloc(NUM_PAGES * PAGE_SIZE);

    void * addr;
    for(int i = 0; i < NUM_PAGES; i++)
    {
	addr = buffer + (i * PAGE_SIZE);
	set_bit(PG_reserved, &vmalloc_to_page(addr)->flags);
    }

    // setup character device
    int err;

    err = alloc_chrdev_region(&dev_num, 0, 1, DEV_NAME);
    dev = cdev_alloc();
    if(err || !dev)
    {
	printk(KERN_ERR "mp3: Unable to allocate character device.\n");
	return 0;
    }
    
    cdev_init(dev, &dev_fops);
    err = cdev_add(dev, dev_num, 1);
    if(err)
    {
	printk(KERN_ERR "mp3: Unable to register character device.\n");
	return 0;
    }

    // set up proc filesystem entries
    proc_dir = proc_mkdir(PROC_DIR, NULL);
    proc_entry = proc_create(PROC_ENTRY, 0666, proc_dir, &proc_fops);

    return 0;
}

void __exit mp2_exit(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Cleaning up MP3 module...\n");
#endif
    // clean up proc filesystem entry
    remove_proc_entry(PROC_ENTRY, proc_dir);
    remove_proc_entry(PROC_DIR, NULL);

    // clean up character device
    cdev_del(dev);
    unregister_chrdev_region(dev_num, 1);

    // clean up shared user-space memory buffer
    void * addr;
    for(int i = 0; i < NUM_PAGES; i++)
    {
	addr = buffer + (i * PAGE_SIZE);
	clear_bit(PG_reserved, &vmalloc_to_page(addr)->flags);
    }

    vfree(buffer);

    // deallocate task registration
    cleanup_tasklist();
    cleanup_workqueue();
}

module_init(mp2_init);
module_exit(mp2_exit);
