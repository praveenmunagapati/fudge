#define TASK_DESCRIPTORS                32
#define TASK_MAILBOXSIZE                4096
#define TASK_STATUS_NORMAL              0
#define TASK_STATUS_BLOCKED             1

struct task_state
{

    struct list_item item;
    unsigned int ip;
    unsigned int sp;
    unsigned int status;
    unsigned int rewind;

};

struct task_mailbox
{

    struct ring ring;
    unsigned char data[TASK_MAILBOXSIZE];
    struct spinlock spinlock;

};

struct task
{

    unsigned int id;
    struct resource resource;
    struct task_state state;
    struct task_mailbox mailbox;
    struct binary_format *format;
    struct binary_node node;

};

void task_initstate(struct task_state *state, struct task *task);
void task_initmailbox(struct task_mailbox *mailbox);
void task_init(struct task *task, unsigned int id);
