#include <fudge.h>
#include <kernel.h>
#include <event/event.h>
#include <modules/system/system.h>
#include <modules/event/event.h>
#include "timer.h"

static struct system_node root;

void timer_notify(struct timer_interface *interface, void *buffer, unsigned int count)
{

    kernel_multicast(&interface->sleepstates, &interface->sleeplock, buffer, count);

}

void timer_notifytick(struct timer_interface *interface, unsigned int counter)
{

    struct {struct event_header header; struct event_timertick timertick;} message;

    message.header.type = EVENT_TIMERTICK;
    message.header.source = EVENT_ADDR_BROADCAST;
    message.header.destination = EVENT_ADDR_BROADCAST;
    message.timertick.counter = counter;

    spinlock_acquire(&interface->eventlock);
    event_multicast(&interface->eventstates, &message.header, sizeof (struct event_header) + sizeof (struct event_timertick));
    spinlock_release(&interface->eventlock);

}

static struct system_node *interfacesleep_open(struct system_node *self, struct service_state *state)
{

    struct timer_interface *interface = self->resource->data;

    list_lockadd(&interface->sleepstates, &state->item, &interface->sleeplock);

    return self;

}

static struct system_node *interfacesleep_close(struct system_node *self, struct service_state *state)
{

    struct timer_interface *interface = self->resource->data;

    list_lockremove(&interface->sleepstates, &state->item, &interface->sleeplock);

    return self;

}

static struct system_node *interfaceevent_open(struct system_node *self, struct service_state *state)
{

    struct timer_interface *interface = self->resource->data;

    list_lockadd(&interface->eventstates, &state->item, &interface->eventlock);

    return self;

}

static struct system_node *interfaceevent_close(struct system_node *self, struct service_state *state)
{

    struct timer_interface *interface = self->resource->data;

    list_lockremove(&interface->eventstates, &state->item, &interface->eventlock);

    return self;

}

void timer_registerinterface(struct timer_interface *interface, unsigned int id)
{

    resource_register(&interface->resource);
    system_addchild(&interface->root, &interface->sleep);
    system_addchild(&interface->root, &interface->event);
    system_addchild(&root, &interface->root);

    interface->id = id;

}

void timer_unregisterinterface(struct timer_interface *interface)
{

    resource_unregister(&interface->resource);
    system_removechild(&interface->root, &interface->sleep);
    system_removechild(&interface->root, &interface->event);
    system_removechild(&root, &interface->root);

}

void timer_initinterface(struct timer_interface *interface)
{

    resource_init(&interface->resource, RESOURCE_TIMERINTERFACE, interface);
    system_initresourcenode(&interface->root, SYSTEM_NODETYPE_GROUP | SYSTEM_NODETYPE_MULTI, "if", &interface->resource);
    system_initresourcenode(&interface->sleep, SYSTEM_NODETYPE_MAILBOX, "sleep", &interface->resource);
    system_initresourcenode(&interface->event, SYSTEM_NODETYPE_NORMAL, "event", &interface->resource);

    interface->sleep.open = interfacesleep_open;
    interface->sleep.close = interfacesleep_close;
    interface->event.open = interfaceevent_open;
    interface->event.close = interfaceevent_close;

}

void module_init(void)
{

    system_initnode(&root, SYSTEM_NODETYPE_GROUP, "timer");

}

void module_register(void)
{

    system_registernode(&root);

}

void module_unregister(void)
{

    system_unregisternode(&root);

}

