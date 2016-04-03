/* mp2.c - MP2 Kernel Module
 * Copyright (C) 2016 Quytelda Kahja <quytelda@tamalin.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include "mp3_given.h"
#include "task.h"
#include "dispatch.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quytelda Kahja");
MODULE_DESCRIPTION("Rate Monotonic Scheduler");

#define DEBUG 1
#define PROC_DIR   "mp2"
#define PROC_ENTRY "status"

static ssize_t mp2_read(struct file * file, char __user * buf,
			size_t length, loff_t * offset)
{
    // we have already sent all of our output
    // all output is sent in one chunk
    if(*offset > 0) return 0;

    char * tasklist_str = tasklist_to_str();
    int len = strlen(tasklist_str);

    if(!copy_to_user(buf, tasklist_str, len))
    {
	*offset += len;
	return len;
    }
    else
    {
	printk(KERN_ERR "mp2: could not copy /proc read data to userspace.\n");
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
	    printk(KERN_ERR "mp3: Dispatch failed (error %d).)\n", error, input);
    }
    else
    {
	printk(KERN_ERR "mp3: Could not copy /proc write data from userspace.\n");
    }

    kfree(input);
    return length;
}

static struct proc_dir_entry * proc_dir;
static struct proc_dir_entry * proc_entry;
static const struct file_operations proc_fops =
{
    .owner = THIS_MODULE,
    .read  = mp2_read,
    .write = mp2_write,
};

int __init mp2_init(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Loading MP2 module...\n");
#endif
    // initialize the registration list
    init_tasklist();

    // set up proc filesystem entries
    proc_dir = proc_mkdir(PROC_DIR, NULL);
    proc_entry = proc_create(PROC_ENTRY, 0666, proc_dir, &proc_fops);

    return 0;
}

void __exit mp2_exit(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Cleaning up MP2 module...\n");
#endif
    // clean up proc filesystem entry
    remove_proc_entry(PROC_ENTRY, proc_dir);
    remove_proc_entry(PROC_DIR, NULL);

    // deallocate task registration
    cleanup_tasklist();
}

module_init(mp2_init);
module_exit(mp2_exit);
