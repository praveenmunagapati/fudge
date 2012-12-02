#include <memory.h>
#include <runtime.h>
#include <vfs.h>

unsigned int runtime_set_task_event(struct runtime_task *task, unsigned int index, unsigned int callback)
{

    if (index >= RUNTIME_TASK_EVENT_SLOTS)
        return 0;

    task->events[index].callback = callback;

    return 1;

}

unsigned int runtime_unset_task_event(struct runtime_task *task, unsigned int index)
{

    if (index >= RUNTIME_TASK_EVENT_SLOTS)
        return 0;

    task->events[index].callback = 0;

    return 1;

}

static struct runtime_task *notify_interrupt(struct runtime_task *self, unsigned int index)
{

    return self;

}

struct runtime_descriptor *runtime_get_task_descriptor(struct runtime_task *task, unsigned int index)
{

    if (!index || index >= RUNTIME_TASK_DESCRIPTOR_SLOTS)
        return 0;

    return &task->descriptors[index];

}

struct runtime_mount *runtime_get_task_mount(struct runtime_task *task, unsigned int index)
{

    if (!index || index >= RUNTIME_TASK_MOUNT_SLOTS)
        return 0;

    return &task->mounts[index];

}

unsigned int runtime_walk(struct runtime_task *task, struct runtime_descriptor *descriptor, struct vfs_interface *interface, unsigned int id, unsigned int count, char *path)
{

    unsigned int i;
    unsigned int slash;
    unsigned int nid;

    if (!count)
    {

        for (i = 1; i < RUNTIME_TASK_MOUNT_SLOTS; i++)
        {

            if (task->mounts[i].child == interface && task->mounts[i].childid == id)
                continue;

            if (task->mounts[i].parent == interface && task->mounts[i].parentid == id)
            {

                return runtime_walk(task, descriptor, task->mounts[i].child, task->mounts[i].childid, count, path);

            }

        }

        runtime_init_descriptor(descriptor, id, interface);

        return id;

    }

    for (slash = 0; slash < count; slash++)
    {

        if (path[slash] == '/')
            break;

    }

    if (slash < count)
        slash++;

    nid = interface->walk(interface, id, slash, path);

    if (nid)
        return runtime_walk(task, descriptor, interface, nid, count - slash, path + slash);

    for (i = 1; i < RUNTIME_TASK_MOUNT_SLOTS; i++)
    {

        if (task->mounts[i].parent == interface && task->mounts[i].parentid == id)
        {

            return runtime_walk(task, descriptor, task->mounts[i].child, task->mounts[i].childid, count, path);

        }

    }

    return 0;

}

void runtime_init_descriptor(struct runtime_descriptor *descriptor, unsigned int id, struct vfs_interface *interface)
{

    memory_clear(descriptor, sizeof (struct runtime_descriptor));

    descriptor->id = id;
    descriptor->interface = interface;

}

void runtime_init_mount(struct runtime_mount *mount, struct vfs_interface *parent, unsigned int parentid, struct vfs_interface *child, unsigned int childid)
{

    memory_clear(mount, sizeof (struct runtime_mount));

    mount->parent = parent;
    mount->parentid = parentid;
    mount->child = child;
    mount->childid = childid;

}

void runtime_init_registers(struct runtime_registers *registers, unsigned int ip, unsigned int sp, unsigned int sb, unsigned int status)
{

    memory_clear(registers, sizeof (struct runtime_registers));

    registers->ip = ip;
    registers->sp = sp;
    registers->sb = sb;
    registers->status = status;

}

void runtime_init_task(struct runtime_task *task)
{

    memory_clear(task, sizeof (struct runtime_task));

    task->status.used = 1;
    task->notify_interrupt = notify_interrupt;

}

