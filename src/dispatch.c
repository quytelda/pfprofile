#include <linux/kernel.h>
#include <linux/module.h>
#include "dispatch.h"
#include "task.h"

#define SAMPLE_DELAY 50

struct workqueue_struct * workqueue = NULL;
DECLARE_DELAYED_WORK(dtask, sample_tasks);

void setup_workqueue()
{
    workqueue = create_workqueue("mp3");

    if(!workqueue)
    {
	printk(KERN_ERR "Unable to initialize workqueue.\n");
	return;
    }

    if(!queue_delayed_work(workqueue, &dtask, msecs_to_jiffies(SAMPLE_DELAY)))
    {
	printk(KERN_ERR "Failed to enqueue sampling task.\n");
	return;
    }
}

void cleanup_workqueue()
{
    if(workqueue)
    {
	cancel_delayed_work(&dtask);
	flush_workqueue(workqueue);
	destroy_workqueue(workqueue);

	workqueue = NULL;
    }
}

static void do_register(pid_t pid)
{
    int num_tasks = register_task(pid);
    printk(KERN_ALERT "Registered %d (%d).\n", pid, num_tasks);

    // upon registering the first task, setup a work queue
    if(num_tasks == 1)
	setup_workqueue();
}

static void do_unregister(pid_t pid)
{
    int num_tasks = deregister_task(pid);
    printk(KERN_ALERT "Deregistered %d (%d).\n", pid, num_tasks);

    // upon removing the last tasks, clean up the work queue
    if(num_tasks == 0)
	cleanup_workqueue();
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
