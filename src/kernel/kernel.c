#include <fudge.h>
#include "resource.h"
#include "binary.h"
#include "task.h"
#include "core.h"
#include "service.h"
#include "kernel.h"

static struct task tasks[KERNEL_TASKS];
static struct service_server servers[KERNEL_SERVERS];
static struct service_mount mounts[KERNEL_MOUNTS];
static struct service services[KERNEL_SERVICES];
static struct list freetasks;
static struct list readytasks;
static struct list blockedtasks;
static struct list unblockedtasks;
static struct spinlock tasklock;
static struct list usedservers;
static struct list freeservers;
static struct spinlock serverlock;
static struct list usedmounts;
static struct list freemounts;
static struct spinlock mountlock;
static struct list freeservices;
static struct spinlock servicelock;

static unsigned int walkmount(struct service *service, struct service_node *from, struct service_node *to)
{

    if (service->server == from->server && service->id == from->id)
    {

        service->server = to->server;
        service->id = to->id;

        return 1;

    }

    return 0;

}

void kernel_walkmountparent(struct service *service)
{

    struct list_item *current;

    spinlock_acquire(&mountlock);

    for (current = usedmounts.head; current; current = current->next)
    {

        struct service_mount *mount = current->data;

        if (walkmount(service, &mount->child, &mount->parent))
            break;

    }

    spinlock_release(&mountlock);

}

void kernel_walkmountchild(struct service *service)
{

    struct list_item *current;

    spinlock_acquire(&mountlock);

    for (current = usedmounts.head; current; current = current->next)
    {

        struct service_mount *mount = current->data;

        if (walkmount(service, &mount->parent, &mount->child))
            break;

    }

    spinlock_release(&mountlock);

}

struct task *kernel_picktask(void)
{

    struct list_item *current;

    spinlock_acquire(&tasklock);

    current = freetasks.tail;

    if (current)
        list_remove(&freetasks, current);

    spinlock_release(&tasklock);

    return (current) ? current->data : 0;

}

void kernel_freetask(struct task *task)
{

    switch (task->state.status)
    {

    case TASK_STATUS_NORMAL:
        list_lockmove(&freetasks, &task->state.item, &tasklock);

        break;

    }

}

void kernel_readytask(struct task *task)
{

    switch (task->state.status)
    {

    case TASK_STATUS_NORMAL:
        list_lockmove(&readytasks, &task->state.item, &tasklock);

        break;

    }

}

void kernel_blocktask(struct task *task)
{

    switch (task->state.status)
    {

    case TASK_STATUS_NORMAL:
        list_lockmove(&blockedtasks, &task->state.item, &tasklock);

        task->state.status = TASK_STATUS_BLOCKED;

        break;

    }

}

void kernel_unblocktask(struct task *task)
{

    switch (task->state.status)
    {

    case TASK_STATUS_BLOCKED:
        list_lockmove(&unblockedtasks, &task->state.item, &tasklock);

        task->state.status = TASK_STATUS_NORMAL;

        break;

    }

}

struct task *kernel_schedule(struct core *core, unsigned int ip, unsigned int sp, void (*assign)(struct task *task))
{

    struct list_item *current;

    spinlock_acquire(&tasklock);

    if (core->task)
    {

        core->task->state.ip = ip;
        core->task->state.sp = sp;

    }

    for (current = unblockedtasks.head; current; current = current->next)
    {

        struct task *task = current->data;

        assign(task);

        task->state.ip -= task->state.rewind;

    }

    for (current = readytasks.head; current; current = current->next)
    {

        struct task *task = current->data;

        assign(task);

    }

    spinlock_release(&tasklock);

    return (core->tasks.tail) ? core->tasks.tail->data : 0;

}

struct service_server *kernel_pickserver(void)
{

    struct list_item *current;

    spinlock_acquire(&serverlock);

    current = freeservers.tail;

    if (current)
        list_remove(&freeservers, current);

    spinlock_release(&serverlock);

    return (current) ? current->data : 0;

}

void kernel_useserver(struct service_server *server)
{

    list_lockmove(&usedservers, &server->item, &serverlock);

}

void kernel_freeserver(struct service_server *server)
{

    list_lockmove(&freeservers, &server->item, &serverlock);

}

struct service_mount *kernel_pickmount(void)
{

    struct list_item *current;

    spinlock_acquire(&mountlock);

    current = freemounts.tail;

    if (current)
        list_remove(&freemounts, current);

    spinlock_release(&mountlock);

    return (current) ? current->data : 0;

}

void kernel_usemount(struct service_mount *mount)
{

    list_lockmove(&usedmounts, &mount->item, &mountlock);

}

void kernel_freemount(struct service_mount *mount)
{

    list_lockmove(&freemounts, &mount->item, &mountlock);

}

struct service *kernel_getservice(struct task *task, unsigned int service)
{

    return &services[task->id * TASK_DESCRIPTORS + (service & (TASK_DESCRIPTORS - 1))];

}

void kernel_freeservice(struct service *service)
{

    list_lockmove(&freeservices, &service->item, &servicelock);

}

static void copyservice(struct service *tservice, struct service *sservice, struct task *task)
{

    tservice->server = (sservice) ? sservice->server : 0;
    tservice->id = (sservice) ? sservice->id : 0;
    tservice->state.task = task;

}

void kernel_copyservices(struct task *source, struct task *target)
{

    unsigned int i;

    for (i = 0; i < 8; i++)
    {

        copyservice(kernel_getservice(target, i + 0), kernel_getservice(source, i + 8), target);
        copyservice(kernel_getservice(target, i + 8), kernel_getservice(source, i + 8), target);
        copyservice(kernel_getservice(target, i + 16), 0, target);
        copyservice(kernel_getservice(target, i + 24), 0, target);

    }

}

unsigned int kernel_readmailbox(struct task *task, void *buffer, unsigned int count)
{

    spinlock_acquire(&task->mailbox.spinlock);

    count = ring_read(&task->mailbox.ring, buffer, count);

    spinlock_release(&task->mailbox.spinlock);

    if (!count)
        kernel_blocktask(task);

    return count;

}

unsigned int kernel_writemailbox(struct task *task, void *buffer, unsigned int count)
{

    spinlock_acquire(&task->mailbox.spinlock);

    count = ring_writeall(&task->mailbox.ring, buffer, count);

    spinlock_release(&task->mailbox.spinlock);

    if (count)
        kernel_unblocktask(task);

    return count;

}

void kernel_unblockall(struct list *states)
{

    struct list_item *current;

    for (current = states->head; current; current = current->next)
    {

        struct service_state *state = current->data;

        kernel_unblocktask(state->task);

    }

}

void kernel_multicast(struct list *states, struct spinlock *spinlock, void *buffer, unsigned int count)
{

    struct list_item *current;

    spinlock_acquire(spinlock);

    for (current = states->head; current; current = current->next)
    {

        struct service_state *state = current->data;

        kernel_writemailbox(state->task, buffer, count);

    }

    spinlock_release(spinlock);

}

unsigned int kernel_setupbinary(struct task *task, unsigned int sp)
{

    struct service *service = kernel_getservice(task, 0);

    task->node.physical = service->server->protocol->map(service->server->backend, &service->state, service->id);

    if (!task->node.physical)
        return 0;

    task->format = binary_findformat(&task->node);

    if (!task->format)
        return 0;

    task->state.ip = task->format->findentry(&task->node);
    task->state.sp = sp;

    return 1;

}

void kernel_setupinit(struct task *task)
{

    struct service *init = kernel_getservice(task, 8);
    struct service *root = kernel_getservice(task, 9);
    struct service_server *server = kernel_pickserver();
    struct service_mount *mount = kernel_pickmount();

    server->backend = service_findbackend(1000);
    server->protocol = service_findprotocol(1000);
    server->root = server->protocol->root(server->backend);
    mount->parent.server = server;
    mount->parent.id = server->root;
    mount->child.server = server;
    mount->child.id = server->root;
    root->server = server;
    root->id = server->root;
    init->server = server;
    init->id = server->root;
    init->id = server->protocol->child(server->backend, &init->state, init->id, "bin", 3);
    init->id = server->protocol->child(server->backend, &init->state, init->id, "init", 4);

    kernel_useserver(server);
    kernel_usemount(mount);

}

void kernel_setuptasks(void)
{

    unsigned int i;

    for (i = 0; i < KERNEL_TASKS; i++)
    {

        struct task *task = &tasks[i];

        task_init(task, i);
        kernel_freetask(task);

    }

}

void kernel_setupservers(void)
{

    unsigned int i;

    for (i = 0; i < KERNEL_SERVERS; i++)
    {

        struct service_server *server = &servers[i];

        service_initserver(server);
        kernel_freeserver(server);

    }

}

void kernel_setupmounts(void)
{

    unsigned int i;

    for (i = 0; i < KERNEL_MOUNTS; i++)
    {

        struct service_mount *mount = &mounts[i];

        service_initmount(mount);
        kernel_freemount(mount);

    }

}

void kernel_setupservices(void)
{

    unsigned int i;

    for (i = 0; i < KERNEL_SERVICES; i++)
    {

        struct service *service = &services[i];

        service_init(service);
        kernel_freeservice(service);

    }

}

