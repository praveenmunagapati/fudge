#include <fudge/kernel.h>
#include "kernel.h"
#include "vfs.h"
#include "vfs_cpio.h"

static struct
{

    struct vfs_protocol base;
    unsigned int physical;

} protocol;

static unsigned int parent(struct vfs_backend *backend, unsigned int count, char *path)
{

    struct cpio_header header;
    unsigned char name[1024];
    unsigned int offset = 0;

    while (--count && path[count - 1] != '/');

    do
    {

        if (backend->read(backend, offset, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
            break;

        if (!cpio_validate(&header))
            break;

        if (backend->read(backend, offset + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
            break;

        if (memory_match(name, path, count))
            return offset;

    } while ((offset = cpio_next(&header, offset)));

    return 0;

}

static unsigned int match(struct vfs_backend *backend)
{

    struct cpio_header header;

    if (backend->read(backend, 0, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    return cpio_validate(&header);

}

static unsigned int get_physical(struct vfs_backend *backend, unsigned int id)
{

    struct cpio_header header;

    if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    return protocol.physical + id + sizeof (struct cpio_header) + header.namesize + (header.namesize & 1);

}

static unsigned int open(struct vfs_backend *backend, unsigned int id)
{

    return id;

}

static unsigned int close(struct vfs_backend *backend, unsigned int id)
{

    return id;

}

static unsigned int read(struct vfs_backend *backend, unsigned int id, unsigned int offset, unsigned int count, void *buffer)
{

    struct cpio_header header;
    unsigned int size;

    if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    size = ((header.filesize[0] << 16) | header.filesize[1]) - offset;

    if ((header.mode & 0xF000) == 0x8000)
        return backend->read(backend, id + sizeof (struct cpio_header) + header.namesize + (header.namesize & 1) + offset, (count > size) ? size : count, buffer);

    if ((header.mode & 0xF000) == 0x4000)
    {

        char name[1024];
        unsigned char *b = buffer;
        unsigned int c = memory_read(b, count, "../\n", 4, offset);
        unsigned int length = header.namesize;
        unsigned int idold = id;

        offset -= (offset > 4) ? 4 : offset;

        while ((id = cpio_next(&header, id)))
        {

            unsigned int l;

            if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
                break;

            if (!cpio_validate(&header))
                break;

            if (backend->read(backend, id + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
                break;

            if (parent(backend, header.namesize, name) != idold)
                continue;

            l = header.namesize - length;
            c += memory_read(b + c, count - c, name + length, l, offset);
            offset -= (offset > l) ? l : offset;
            c += memory_read(b + c, count - c, "\n", 1, offset);
            offset -= (offset > 1) ? 1 : offset;

        }

        return c;

    }

    return 0;

}

static unsigned int write(struct vfs_backend *backend, unsigned int id, unsigned int offset, unsigned int count, void *buffer)
{

    struct cpio_header header;
    unsigned int size;

    if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    size = ((header.filesize[0] << 16) | header.filesize[1]) - offset;

    if ((header.mode & 0xF000) == 0x8000)
        return backend->write(backend, id + sizeof (struct cpio_header) + header.namesize + (header.namesize & 1) + offset, (count > size) ? size : count, buffer);

    return 0;

}

static unsigned int walk(struct vfs_backend *backend, unsigned int id, unsigned int count, const char *path)
{

    struct cpio_header header;
    char name[1024];
    unsigned int length;

    if (!count)
        return id;

    if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    if (!cpio_validate(&header))
        return 0;

    if (backend->read(backend, id + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
        return 0;

    length = header.namesize;

    if (memory_match(path, "../", 3))
        return walk(backend, parent(backend, header.namesize, name), count - 3, path + 3);

    while ((id = cpio_next(&header, id)))
    {

        unsigned int l;

        if (backend->read(backend, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
            break;

        if (!cpio_validate(&header))
            break;

        if (backend->read(backend, id + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
            break;

        l = header.namesize;

        if (l < length)
            break;

        l -= length;

        if (memory_match(name + length, path, l))
            return walk(backend, id, count - l, path + l);

    }

    return 0;

}

struct vfs_protocol *vfs_cpio_setup(unsigned int physical)
{

    vfs_init_protocol(&protocol.base, 0, match, open, close, read, write, walk, get_physical);

    protocol.physical = physical;

    return &protocol.base;

}

