/* mp2.c - MP2 Kernel Module
 * Copyright (C) 2016 Quytelda Kahja <quytelda@tamalin.org>
 */

#include <linux/kernel.h>
#include <linux/module.h>

#define DEBUG 1

int __init mp2_init(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Loading MP2 module...");
#endif
    return 0;
}

void __exit mp2_exit(void)
{
#ifdef DEBUG
    printk(KERN_INFO "Cleaning up MP2 module...");
#endif
}

module_init(mp2_init);
module_exit(mp2_exit);
