#include <lib/call.h>
#include <lib/memory.h>
#include <lib/string.h>
#include <kernel/vfs.h>
#include <kernel/modules.h>
#include <modules/tty/tty.h>

static struct tty_device ttyDevice;
static struct modules_device cwdDevice;

static struct vfs_node *ttyVgaNode;
static struct vfs_node *ttyVgaColorNode;
static struct vfs_node *ttyVgaCursorNode;

static char ttyCwd[256];

static void tty_scroll()
{

    char buffer[TTY_CHARACTER_SIZE];

    ttyVgaNode->operations.read(ttyVgaNode, TTY_CHARACTER_WIDTH, TTY_CHARACTER_SIZE - TTY_CHARACTER_WIDTH, buffer);
    memory_set(buffer + TTY_CHARACTER_SIZE - TTY_CHARACTER_WIDTH, ' ', TTY_CHARACTER_WIDTH);
    ttyVgaNode->operations.write(ttyVgaNode, 0, TTY_CHARACTER_SIZE, buffer);

    ttyDevice.cursorOffset -= TTY_CHARACTER_WIDTH;

}

static void tty_putc(char c)
{

    if (c == '\b')
    {

        ttyDevice.cursorOffset--;

    }

    else if (c == '\t')
    {

        ttyDevice.cursorOffset = (ttyDevice.cursorOffset + 8) & ~(8 - 1);

    }

    else if (c == '\r')
    {

        ttyDevice.cursorOffset -= (ttyDevice.cursorOffset % TTY_CHARACTER_WIDTH);

    }

    else if (c == '\n')
    {

        ttyDevice.cursorOffset += TTY_CHARACTER_WIDTH - (ttyDevice.cursorOffset % TTY_CHARACTER_WIDTH);

    }
    
    else if (c >= ' ')
    {

        ttyVgaNode->operations.write(ttyVgaNode, ttyDevice.cursorOffset, 1, &c);
        ttyDevice.cursorOffset++;

    }

    if (ttyDevice.cursorOffset >= TTY_CHARACTER_WIDTH * TTY_CHARACTER_HEIGHT)
        tty_scroll();

}

static void tty_vga_clear()
{

    char c = ' ';
    int i;

    for (i = 0; i < TTY_CHARACTER_SIZE; i++)
        ttyVgaNode->operations.write(ttyVgaNode, i, 1, &c);

}

static unsigned int tty_write(struct vfs_node *node, unsigned int offset, unsigned int count, void *buffer)
{

    unsigned int i;
    unsigned int j = 0;

    for (i = offset; i < offset + count; i++, j++)
        tty_putc(((char *)buffer)[j]);

    ttyVgaCursorNode->operations.write(ttyVgaCursorNode, 0, 1, &ttyDevice.cursorOffset);

    return count;

}

static unsigned int tty_cwd_read(struct vfs_node *node, unsigned int offset, unsigned int count, void *buffer)
{

    count = string_length(ttyCwd) - offset;

    string_copy(buffer, ttyCwd + offset);

    return count;

}

static unsigned int tty_cwd_write(struct vfs_node *node, unsigned int offset, unsigned int count, void *buffer)
{

    count = string_length(ttyCwd) - offset;

    string_copy(ttyCwd + offset, buffer);

    return count;

}

static void tty_set_color(unsigned char fg, unsigned char bg)
{

    ttyDevice.cursorColor = (bg << 4) | (fg & 0x0F);

}

void tty_init()
{

    string_copy(ttyDevice.base.name, "tty");
    string_copy(ttyDevice.base.node.name, "tty");
    ttyDevice.base.node.length = TTY_CHARACTER_SIZE;
    ttyDevice.cursorOffset = 0;
    ttyDevice.set_color = tty_set_color;
    ttyDevice.set_color(TTY_COLOR_WHITE, TTY_COLOR_BLACK);
    modules_register_device(MODULES_DEVICE_TYPE_KEYBOARD, &ttyDevice.base);

    string_copy(cwdDevice.name, "cwd");
    string_copy(cwdDevice.node.name, "cwd");
    cwdDevice.node.length = 256;
    cwdDevice.node.operations.read = tty_cwd_read;
    cwdDevice.node.operations.write = tty_cwd_write;
    modules_register_device(MODULES_DEVICE_TYPE_KEYBOARD, &cwdDevice);

    ttyVgaNode = vfs_find_root("/dev/vga_fb");
    ttyVgaColorNode = vfs_find_root("/dev/vga_fb_color");
    ttyVgaCursorNode = vfs_find_root("/dev/vga_fb_cursor");

    ttyVgaColorNode->operations.write(ttyVgaColorNode, 0, 1, &ttyDevice.cursorColor);

    tty_vga_clear();
    string_copy(ttyCwd, "/");

    // TODO:REMOVE ALL BELOW
    struct vfs_node *ttyStdoutNode = vfs_add_node("tty", TTY_CHARACTER_SIZE);
    ttyStdoutNode->operations.write = tty_write;

    struct vfs_node *ttyCwdNode = vfs_add_node("cwd", 256);
    ttyCwdNode->operations.read = tty_cwd_read;
    ttyCwdNode->operations.write = tty_cwd_write;

    struct vfs_node *devNode = vfs_find_root("/dev");
    devNode->operations.write(devNode, devNode->length, 1, ttyStdoutNode);
    devNode->operations.write(devNode, devNode->length, 1, ttyCwdNode);

}

