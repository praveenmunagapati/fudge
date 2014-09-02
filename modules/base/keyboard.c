#include <module.h>
#include <kernel/resource.h>
#include <kernel/vfs.h>
#include <system/system.h>
#include "base.h"
#include "keyboard.h"

static struct system_node root;

static unsigned int data_read(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct base_keyboard_node *node = (struct base_keyboard_node *)self->parent;

    return node->interface->rdata(node->device->bus, node->device->id, offset, count, buffer);

}

static unsigned int keymap_read(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct base_keyboard_node *node = (struct base_keyboard_node *)self->parent;

    return node->interface->rkeymap(node->device->bus, node->device->id, offset, count, buffer);

}

static unsigned int keymap_write(struct system_node *self, unsigned int offset, unsigned int count, void *buffer)
{

    struct base_keyboard_node *node = (struct base_keyboard_node *)self->parent;

    return node->interface->wkeymap(node->device->bus, node->device->id, offset, count, buffer);

}

void base_keyboard_registerinterface(struct base_keyboard_interface *interface)
{

    base_registerinterface(&interface->base);

}

void base_keyboard_registernode(struct base_keyboard_node *node)
{

    system_addchild(&root, &node->base);
    system_addchild(&node->base, &node->data);
    system_addchild(&node->base, &node->keymap);

}

void base_keyboard_unregisterinterface(struct base_keyboard_interface *interface)
{

    base_unregisterinterface(&interface->base);

}

void base_keyboard_unregisternode(struct base_keyboard_node *node)
{

}

void base_keyboard_initinterface(struct base_keyboard_interface *interface, unsigned int (*rdata)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer), unsigned int (*wdata)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer), unsigned int (*rkeymap)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer), unsigned int (*wkeymap)(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer))
{

    memory_clear(interface, sizeof (struct base_keyboard_interface));
    base_initinterface(&interface->base, BASE_INTERFACE_TYPE_KEYBOARD);

    interface->rdata = rdata;
    interface->wdata = wdata;
    interface->rkeymap = rkeymap;
    interface->wkeymap = wkeymap;

}

void base_keyboard_initnode(struct base_keyboard_node *node, struct base_device *device, struct base_keyboard_interface *interface)
{

    memory_clear(node, sizeof (struct base_keyboard_node));
    system_initmultigroup(&node->base, device->bus->name);
    system_initstream(&node->data, "data");
    system_initstream(&node->keymap, "keymap");

    node->device = device;
    node->interface = interface;
    node->data.read = data_read;
    node->keymap.read = keymap_read;
    node->keymap.write = keymap_write;

}

void base_keyboard_setup()
{

    system_initgroup(&root, "keyboard");
    system_registernode(&root);

}

