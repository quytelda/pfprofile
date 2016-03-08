/* mp2.c - MP2 Kernel Module
 * Copyright (C) 2016 Quytelda Kahja <quytelda@tamalin.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quytelda Kahja");
MODULE_DESCRIPTION("Rate Monotonic Scheduler");

#define DEBUG 1
#define PROC_DIR   "mp2"
#define PROC_ENTRY "status"
#define DELIMITER  ","

#define BUFF_SIZE 1024

typedef enum {SLEEPING, RUNNING, READY} task_state_t;

struct mp2_task_struct
{
    struct list_head list;

    struct task_struct * linux_task;
    struct timer_list wakeup_timer;
    task_state_t task_state;

    pid_t pid;
    unsigned int period;
    unsigned int computation;
};

struct mp2_task_struct reg_list;

int register_pid(int pid, int period)
{
    // TODO: move this to slab allocated memory
    struct mp2_task_struct * new_task = (struct mp2_task_struct *)
	kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);
    new_task->task_state = SLEEPING;
    list_add(&new_task->list, &reg_list.list);

    return 0;
}

int dispatch(const char * cmd, char * args)
{
    if(strcmp(cmd, "R") == 0)
    {
	printk(KERN_INFO "args: %s\n", args);
	// parse registration information
	char * str_pid = strsep(&args, DELIMITER);
	char * str_per = strsep(&args, DELIMITER);
	char * str_cmp = strsep(&args, DELIMITER);

	unsigned int pid;
	unsigned int period;
	unsigned int computation;
	int parse_failure = 0;
	parse_failure |= kstrtouint(str_pid, 10, &pid);
	parse_failure |= kstrtouint(str_per, 10, &period);
	parse_failure |= kstrtouint(str_cmp, 10, &computation);

	if(parse_failure)
	    return parse_failure;

	printk(KERN_ALERT "Registering %d (period %d)\n", pid, period);
	register_pid(pid, period);
    }
    else if(strcmp(cmd, "Y") == 0)
    {
    }
    else if(strcmp(cmd, "D") == 0)
    {
    }
    else
    {
	printk(KERN_ERR "mp2: unrecognized command.\n");
	return 1;
    }

    return 0;
}

static ssize_t mp2_read(struct file * file, char __user * buf,
			size_t length, loff_t * offset)
{
    list_head * cursor;
    struct mp2_task_struct * current_task;
    list_for_each(cursor, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);
	printk(KERN_ALERT "%d, %d\n", current_task->pid, current_task->period);
    }

    return 0;
}

static ssize_t mp2_write(struct file * file, const char * buf,
			 size_t length, loff_t * offset)
{
    int err;

    // parse request
    char * input = kmalloc(length + 1, GFP_KERNEL);
    err = copy_from_user(input, buf, length);
    *(input + length) = 0;
    if(err)
    {
	printk(KERN_ERR "mp2: could not copy /proc write data from userspace.\n");
	return 0;
    }

    char * start = input;
    char * command = strsep(&input, DELIMITER);

    // go to dispatch
    err = dispatch(command, input);
    if(err)
    {
	printk(KERN_ERR "mp2: dispatch failed for command '%s'\n", command);
    }

    kfree(start);
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
    INIT_LIST_HEAD(&reg_list.list);

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

    // clean up the registration list
    struct list_head * cursor, * tmp;
    struct mp2_task_struct * current_task;
    list_for_each_safe(cursor, tmp, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);
	list_del(cursor); // remove from the list
	kfree(current_task); // free allocated memory
    }
}

module_init(mp2_init);
module_exit(mp2_exit);
