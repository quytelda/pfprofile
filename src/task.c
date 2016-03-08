#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include "task.h"

static struct mp2_task_struct reg_list;
static int num_tasks = 0;

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

void register_task(int pid, int period)
{
    // TODO: move this to slab allocated memory
    struct mp2_task_struct * new_task = (struct mp2_task_struct *)
	kmalloc(sizeof(struct mp2_task_struct), GFP_KERNEL);
    new_task->pid        = pid;
    new_task->period     = period;
    new_task->task_state = SLEEPING;
    list_add(&new_task->list, &reg_list.list);

    num_tasks++;
}

void deregister_task(int pid)
{
    // lookup the task
    struct list_head * cursor, * tmp;
    struct mp2_task_struct * current_task = NULL;
    list_for_each_safe(cursor, tmp, &reg_list.list)
    {
	current_task = list_entry(cursor, struct mp2_task_struct, list);
	if(current_task->pid == pid)
	    break;
    }

    // if we found it, get rid of it
    if(current_task)
    {
	list_del(cursor); // remove from the list
	kfree(current_task);
    }
    else
    {
	printk(KERN_ERR "mp2: Can't deregister task (pid = %d).\n", pid);
    }
}

static char * task_to_str(struct mp2_task_struct * task)
{
    char * desc = kmalloc(DESC_MAX_SIZE, GFP_KERNEL);
    snprintf(desc, DESC_MAX_SIZE, "%d, %d\n", task->pid, task->period);

    return desc;
}

/**
 * Returns a pointer to a character buffer in kernel space
 * describing the list of currently registered tasks.
 * This buffer should be freed later with kfree()
 */
char * tasklist_to_str(void)
{
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
