/* mp2.c - MP2 Kernel Module
 * Copyright (C) 2016 Quytelda Kahja <quytelda@tamalin.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/proc_fs.h>
#include "mp2_given.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Quytelda Kahja");
MODULE_DESCRIPTION("Rate Monotonic Scheduler");

#define DEBUG 1
#define PROC_DIR "mp2"

struct proc_dir_entry * proc_dir;

int __init mp2_init(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Loading MP2 module...");
#endif

    // set up proc filesystem entries
    proc_dir = proc_mkdir(PROC_DIR, NULL);
    return 0;
}

void __exit mp2_exit(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Cleaning up MP2 module...");
#endif
    remove_proc_entry(PROC_DIR, NULL);
}

module_init(mp2_init);
module_exit(mp2_exit);
