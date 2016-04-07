#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include "task.h"
#include "mem.h"
#include "mp3_given.h"

static struct mp2_task_struct reg_list;
static int num_tasks = 0;

extern void * buffer;
unsigned long * ptr = NULL;
extern struct workqueue_struct * workqueue;
extern struct delayed_work dtask;
void sample_tasks(struct work_struct * work)
{
    unsigned long major = 0, minor = 0;
    unsigned long cpu_util = 0;

    struct list_head * cursor;
    struct mp2_task_struct * current_task;
    list_for_each(cursor, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);

	int pid = (int) current_task->pid;
	unsigned long min_flt, maj_flt;
	unsigned long utime, stime;
	if(get_cpu_use(pid, &min_flt, &maj_flt, &utime, &stime))
	{
	    printk(KERN_ERR "Unable to sample process (pid = %d).\n", pid);
	    continue;
	}

	major += maj_flt;
	minor += min_flt;
	cpu_util += ((utime + stime) * 1000) / jiffies;
    }

    // put in shared memory
    if(!ptr)
	ptr = (unsigned long *) buffer;

    ptr[0] = jiffies;
    ptr[1] = minor;
    ptr[2] = major;
    ptr[3] = cpu_util;

    ptr += 4 * sizeof(unsigned long);
    if((void *) ptr >= buffer + NUM_PAGES * PAGE_SIZE)
	ptr = buffer;

    if(!queue_delayed_work(workqueue, &dtask, msecs_to_jiffies(50)))
    {
	printk(KERN_ERR "Failed to enqueue sampling task.\n");
    }
}

void init_tasklist(void)
{
    INIT_LIST_HEAD(&reg_list.list);
}

void cleanup_tasklist(void)
{
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

/**
 * Allocates and fills new task mp2_task_struct for this PID
 * and adds it to the registered task list.
 * TODO: Duplicate registrations will be ignored.
 *
 * Returns the new number of tasks in the list.
 */
int register_task(pid_t pid)
{
    // TODO: move this to slab allocated memory
    struct mp2_task_struct * new_task = (struct mp2_task_struct *)
	kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);
    new_task->pid = pid;

    list_add(&new_task->list, &reg_list.list);
    return ++num_tasks;
}

/**
 * Removes and deallocates the task with pid @pid from the registered task list.
 * Requests with non-existant PIDs will be complained about.
 *
 * Returns the new number of tasks in the list or -1.
 */
int deregister_task(pid_t pid)
{
    // lookup the task
    struct list_head * cursor, * tmp;
    struct mp2_task_struct * current_task = NULL;
    list_for_each_safe(cursor, tmp, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);
	if(current_task->pid == pid)
	    break;

	current_task = NULL;
    }

    // if we found it, get rid of it
    if(current_task)
    {
	list_del(cursor);
	kfree(current_task);

	return --num_tasks;
    }
    else
    {
	printk(KERN_ERR "mp3: Can't deregister task (pid = %d).\n", pid);
	return -1;
    }
}

static char * task_to_str(struct mp2_task_struct * task)
{
    char * desc = kmalloc(DESC_MAX_SIZE, GFP_KERNEL);
    snprintf(desc, DESC_MAX_SIZE, "%d\n", task->pid);

    return desc;
}

/**
 * Returns a pointer to a character buffer in kernel space
 * describing the list of currently registered tasks.
 * This buffer should be freed later with kfree()
 * If the tasklist is empty, this function returns NULL
 */
char * tasklist_to_str(void)
{
    if(num_tasks <= 0) return NULL;

    char * buffer = kmalloc(num_tasks * DESC_MAX_SIZE, GFP_KERNEL);
    memset(buffer, 0, num_tasks * DESC_MAX_SIZE);

    struct list_head * cursor;
    struct mp2_task_struct * current_task;
    list_for_each(cursor, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);
	char * desc = task_to_str(current_task);

	strlcat(buffer, desc, num_tasks * DESC_MAX_SIZE);
    }

    return buffer;
}
