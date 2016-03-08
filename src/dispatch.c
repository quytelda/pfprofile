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
	unsigned int comp_time;
	int parse_failure = 0;
	parse_failure |= kstrtouint(str_pid, 10, &pid);
	parse_failure |= kstrtouint(str_per, 10, &period);
	parse_failure |= kstrtouint(str_cmp, 10, &comp_time);

	if(parse_failure)
	    return parse_failure;

	printk(KERN_ALERT "Registered %d (period %d).\n", pid, period);
	register_task((pid_t) pid, period, comp_time);
	return 0;
}

static int do_deregister(char * args)
{
    	// parse registration information
	char * str_pid = strsep(&args, DELIMITER);

	unsigned int pid;
	int parse_failure = kstrtouint(str_pid, 10, &pid);

	if(parse_failure)
	    return parse_failure;

	printk(KERN_ALERT "Deregistered %d.\n", pid);
	deregister_task(pid);
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
	do_deregister(start);
	break;
    case 'Y':
    default:
	printk(KERN_ERR "mp2: Unrecognized command.\n");
	return 1;
    }

    return 0;
}
