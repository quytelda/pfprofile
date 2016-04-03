#include <linux/kernel.h>
#include <linux/module.h>
#include "dispatch.h"
#include "task.h"

static void do_register(pid_t pid)
{
    printk(KERN_ALERT "Registered %d.\n", pid);
    register_task(pid);
}

static void do_unregister(pid_t pid)
{
    printk(KERN_ALERT "Deregistered %d.\n", pid);
    deregister_task(pid);
}

int dispatch(char * input)
{
    // parse out the command character
    char * ptr = input;
    char * command = strsep(&ptr, DELIMITER);
    if(strlen(command) < 1) return 1;

    // parse out the PID
    if(!ptr) return 1;

    pid_t pid;
    int parse_failure = kstrtouint(ptr, 10, &pid);
    if(parse_failure)
    {
	printk(KERN_ERR "mp3: Unable to parse pid: '%s'.", ptr);
	return parse_failure;
    }

    // actual dispatch
    switch(command[0])
    {
    case 'R':
        do_register(pid);
	break;
    case 'U':
	do_unregister(pid);
	break;
    case 'Y':
    default:
	printk(KERN_ERR "mp3: Unrecognized command.\n");
	return 1;
    }

    return 0;
}
