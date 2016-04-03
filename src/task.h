#define DESC_MAX_SIZE 64

typedef enum {SLEEPING, RUNNING, READY} task_state_t;

struct mp2_task_struct
{
    struct list_head list;
    struct task_struct * linux_task;

    pid_t pid;
};

void init_tasklist(void);
void cleanup_tasklist(void);

void register_task(pid_t pid);
void deregister_task(int pid);

char * tasklist_to_str(void);
