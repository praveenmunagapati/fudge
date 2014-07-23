#include <module.h>
#include <kernel/resource.h>
#include <kernel/rendezvous.h>
#include <base/base.h>
#include <base/mouse.h>
#include <arch/x86/pic/pic.h>
#include "ps2.h"
#include "mouse_driver.h"

struct ps2_mouse_stream
{

    char buffer[512];
    unsigned int head;
    unsigned int tail;

};

static struct base_driver driver;
static struct base_mouse_interface imouse;
static struct ps2_mouse_stream stream;
static unsigned char cycle;
static char status;
static struct rendezvous rdata;

static unsigned int read_stream(struct ps2_mouse_stream *stream, unsigned int count, void *buffer)
{

    char *b = buffer;
    unsigned int i;

    for (i = 0; i < count; i++)
    {

        unsigned int tail = (stream->tail + 1) % 512;

        if (stream->head == stream->tail)
            break;

        b[i] = stream->buffer[stream->tail];
        stream->tail = tail;

    }

    return i;

}

static unsigned int write_stream(struct ps2_mouse_stream *stream, unsigned int count, void *buffer)
{

    char *b = buffer;
    unsigned int i;

    for (i = 0; i < count; i++)
    {

        unsigned int head = (stream->head + 1) % 512;

        if (head == stream->tail)
            break;

        stream->buffer[stream->head] = b[i];
        stream->head = head;

    }

    return i;

}

static void reset(struct base_bus *bus)
{

    ps2_bus_write_command(bus, 0xD4);
    ps2_bus_write_data(bus, 0xFF);
    ps2_bus_read_data(bus);

}

static void set_defaults(struct base_bus *bus)
{

    ps2_bus_write_command(bus, 0xD4);
    ps2_bus_write_data(bus, 0xF6);
    ps2_bus_read_data(bus);

}

static void identify(struct base_bus *bus)
{

    ps2_bus_write_command(bus, 0xD4);
    ps2_bus_write_data(bus, 0xF2);
    ps2_bus_read_data(bus);

}

static void enable_scanning(struct base_bus *bus)
{

    ps2_bus_write_command(bus, 0xD4);
    ps2_bus_write_data(bus, 0xF4);
    ps2_bus_read_data(bus);

}

static void disable_scanning(struct base_bus *bus)
{

    ps2_bus_write_command(bus, 0xD4);
    ps2_bus_write_data(bus, 0xF5);
    ps2_bus_read_data(bus);

}

static unsigned int read_data(struct base_bus *bus, unsigned int id, unsigned int offset, unsigned int count, void *buffer)
{

    count = read_stream(&stream, count, buffer);

    rendezvous_sleep(&rdata, !count);

    return count;

}

static void handle_irq(unsigned int irq, struct base_bus *bus, unsigned int id)
{

    unsigned char data = ps2_bus_read_data_async(bus);

    switch (cycle)
    {

    case 0:
        status = data;
        cycle++;

        break;

    case 1:
        imouse.vx = data;
        cycle++;

        break;

    case 2:
        imouse.vy = data;
        cycle = 0;

        break;

    }

    write_stream(&stream, 1, &data);
    rendezvous_unsleep(&rdata);

}

static unsigned int check(struct base_bus *bus, unsigned int id)
{

    if (bus->type != PS2_BUS_TYPE)
        return 0;

    return id == PS2_MOUSE_DEVICE_TYPE;

}

static void attach(struct base_bus *bus, unsigned int id)
{

    unsigned short irq = bus->device_irq(bus, id);

    base_mouse_connect_interface(&imouse, bus, id);
    pic_set_routine(irq, bus, id, handle_irq);
    ps2_bus_enable_device(bus, id);
    ps2_bus_enable_interrupt(bus, id);
    disable_scanning(bus);
    reset(bus);
    set_defaults(bus);
    identify(bus);
    enable_scanning(bus);

}

static void detach(struct base_bus *bus, unsigned int id)
{

    unsigned short irq = bus->device_irq(bus, id);

    base_mouse_unregister_interface(&imouse);
    pic_unset_routine(irq, bus, id);

}

void ps2_mouse_driver_init()
{

    base_mouse_init_interface(&imouse, read_data);
    base_mouse_register_interface(&imouse);
    base_init_driver(&driver, "ps2mouse", check, attach, detach);
    base_register_driver(&driver);

}

void ps2_mouse_driver_destroy()
{

    base_mouse_unregister_interface(&imouse);
    base_unregister_driver(&driver);

}

