#include <module.h>
#include <kernel/resource.h>
#include <kernel/vfs.h>
#include <system/system.h>
#include "base.h"
#include "terminal.h"

static struct system_node root;

static unsigned int data_read(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct base_terminal_node *node = (struct base_terminal_node *)self->parent;
 
    return node->interface->rdata(node->device->bus, node->device->id, offset, count, buffer);

}

static unsigned int data_write(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct base_terminal_node *node = (struct base_terminal_node *)self->parent;

    return node->interface->wdata(node->device->bus, node->device->id, offset, count, buffer);

}

void base_terminal_registerinterface(struct base_terminal_interface *interface)
{

    base_registerinterface(&interface->base);

}

void base_terminal_registernode(struct base_terminal_node *node)
{

    system_addchild(&root, &node->base);
    system_addchild(&node->base, &node->data);

}

void base_terminal_unregisterinterface(struct base_terminal_interface *interface)
{

    base_unregisterinterface(&interface->base);

}

void base_terminal_unregisternode(struct base_terminal_node *node)
{

}

void base_terminal_initinterface(struct base_terminal_interface *interface, unsigned int (*rdata)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer), unsigned int (*wdata)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer))
{

    memory_clear(interface, sizeof (struct base_terminal_interface));
    base_initinterface(&interface->base, BASE_INTERFACE_TYPE_TERMINAL);

    interface->rdata = rdata;
    interface->wdata = wdata;

}

void base_terminal_initnode(struct base_terminal_node *node, struct base_device *device, struct base_terminal_interface *interface)
{

    memory_clear(node, sizeof (struct base_terminal_node));
    system_initmultigroup(&node->base, device->bus->name);
    system_initstream(&node->data, "data");

    node->device = device;
    node->interface = interface;
    node->data.read = data_read;
    node->data.write = data_write;

}

void base_terminal_setup()
{

    system_initgroup(&root, "terminal");
    system_registernode(&root);

}

