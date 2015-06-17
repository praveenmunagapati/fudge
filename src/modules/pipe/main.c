#include <fudge.h>
#include <kernel.h>
#include <modules/system/system.h>

static struct system_node root;
static struct task *t0;
static struct task *t1;
static struct system_node endpoint0;
static struct system_node endpoint1;

static struct task *openpipe()
{

    return scheduler_findactive();

}

static struct task *closepipe(struct task *task)
{

    if (task)
        scheduler_release(task);

    return 0;

}

static unsigned int readpipe(struct task *task, unsigned int size, unsigned int count, void *buffer)
{

    if (task)
        return scheduler_rmessage(task, size, count, buffer);
    else
        return 0;

}

static unsigned int sendpipe(struct task *task, unsigned int size, unsigned int count, void *buffer)
{

    if (task)
        return scheduler_wmessage(task, size, count, buffer);
    else
        return 0;

}

static unsigned int endpoint0_open(struct system_node *self)
{

    t0 = openpipe();

    return system_open(self);

}

static unsigned int endpoint0_close(struct system_node *self)
{

    t0 = closepipe(t0);
    t1 = closepipe(t1);

    return system_close(self);

}

static unsigned int endpoint0_read(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer)
{

    return readpipe(t0, size, count, buffer);

}

static unsigned int endpoint0_write(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer)
{

    return sendpipe(t1, size, count, buffer);

}

static unsigned int endpoint1_open(struct system_node *self)
{

    t1 = openpipe();

    return system_open(self);

}

static unsigned int endpoint1_close(struct system_node *self)
{

    t1 = closepipe(t1);
    t0 = closepipe(t0);

    return system_close(self);

}

static unsigned int endpoint1_read(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer)
{

    return readpipe(t1, size, count, buffer);

}

static unsigned int endpoint1_write(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer)
{

    return sendpipe(t0, size, count, buffer);

}

void module_init()
{

    t0 = 0;
    t1 = 0;

    system_initnode(&endpoint0, SYSTEM_NODETYPE_NORMAL, "0");

    endpoint0.open = endpoint0_open;
    endpoint0.close = endpoint0_close;
    endpoint0.read = endpoint0_read;
    endpoint0.write = endpoint0_write;

    system_initnode(&endpoint1, SYSTEM_NODETYPE_NORMAL, "1");

    endpoint1.open = endpoint1_open;
    endpoint1.close = endpoint1_close;
    endpoint1.read = endpoint1_read;
    endpoint1.write = endpoint1_write;

    system_initnode(&root, SYSTEM_NODETYPE_GROUP | SYSTEM_NODETYPE_MULTI, "pipe");
    system_addchild(&root, &endpoint0);
    system_addchild(&root, &endpoint1);

}

void module_register()
{

    system_registernode(&root);

}

void module_unregister()
{

    system_unregisternode(&root);

}

