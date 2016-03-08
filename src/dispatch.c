#include <linux/kernel.h>
#include <linux/module.h>
#include "dispatch.h"
#include "task.h"

static int do_register(char * args)
{
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

	printk(KERN_ALERT "Registered %d (period %d).\n", pid, period);
	register_task(pid, period);
	return 0;
}

int dispatch(char * input)
{
    // parse out the command character
    char * start = input;
    char * command = strsep(&start, DELIMITER);
    if(strlen(command) < 1) return 1;

    switch(command[0])
    {
    case 'R':
	do_register(start);
	break;
    case 'D':
    case 'Y':
    default:
	printk(KERN_ERR "mp2: unrecognized command.\n");
	return 1;
    }

    return 0;
}
