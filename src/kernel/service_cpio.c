#include <fudge.h>
#include <cpio/cpio.h>
#include "resource.h"
#include "binary.h"
#include "task.h"
#include "service.h"

static struct service_protocol protocol;

static unsigned int readheader(struct service_backend *backend, struct cpio_header *header, unsigned int id)
{

    unsigned int count = backend->read(header, sizeof (struct cpio_header), id);

    return (count == sizeof (struct cpio_header)) ? cpio_validate(header) : 0;

}

static unsigned int readname(struct service_backend *backend, struct cpio_header *header, unsigned int id, char *buffer, unsigned int count, unsigned int offset)
{

    return (header->namesize <= count) ? backend->read(buffer, header->namesize - offset, id + sizeof (struct cpio_header) + offset) : 0;

}

static unsigned int parent(struct service_backend *backend, struct cpio_header *header, unsigned int id)
{

    struct cpio_header eheader;
    char name[1024];
    char dname[1024];
    unsigned int length;
    unsigned int current = id;

    if (!readname(backend, header, id, name, 1024, 0))
        return id;

    length = ascii_dname(dname, 1024, name, header->namesize - 1, '/');

    do
    {

        if (!readheader(backend, &eheader, current))
            break;

        if ((eheader.mode & 0xF000) != 0x4000)
            continue;

        if (eheader.namesize == length + 1)
            return current;

    } while ((current = cpio_next(&eheader, current)));

    return id;

}

static unsigned int child(struct service_backend *backend, struct cpio_header *header, unsigned int id, char *path, unsigned int length)
{

    struct cpio_header eheader;
    char name[1024];
    unsigned int current = 0;

    if (!readname(backend, header, id, name, 1024, 0))
        return id;

    do
    {

        char cname[1024];

        if (current == id)
            break;

        if (!readheader(backend, &eheader, current))
            break;

        if (eheader.namesize - header->namesize != length + 1)
            continue;

        if (!readname(backend, &eheader, current, cname, 1024, header->namesize))
            break;

        if (memory_match(cname, path, length))
            return current;

    } while ((current = cpio_next(&eheader, current)));

    return id;

}

static unsigned int protocol_match(struct service_backend *backend)
{

    struct cpio_header header;

    return readheader(backend, &header, 0);

}

static unsigned int protocol_root(struct service_backend *backend)
{

    struct cpio_header header;
    unsigned int id = 0;
    unsigned int current = id;

    do
    {

        if (!readheader(backend, &header, current))
            break;

        if ((header.mode & 0xF000) != 0x4000)
            continue;

        id = current;

    } while ((current = cpio_next(&header, current)));

    return id;

}

static unsigned int protocol_parent(struct service_backend *backend, struct service_state *state, unsigned int id)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return id;

    return parent(backend, &header, id);

}

static unsigned int protocol_child(struct service_backend *backend, struct service_state *state, unsigned int id, char *path, unsigned int length)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return id;

    return child(backend, &header, id, path, length);

}

static unsigned int protocol_create(struct service_backend *backend, struct service_state *state, unsigned int id, char *name, unsigned int length)
{

    return 0;

}

static unsigned int protocol_destroy(struct service_backend *backend, struct service_state *state, unsigned int id, char *name, unsigned int length)
{

    return 0;

}

static unsigned int stepdirectory(struct service_backend *backend, unsigned int id, unsigned int current)
{

    struct cpio_header eheader;

    if (!readheader(backend, &eheader, current))
        return 0;

    while ((current = cpio_next(&eheader, current)))
    {

        if (current == id)
            break;

        if (!readheader(backend, &eheader, current))
            break;

        if (parent(backend, &eheader, current) == id)
            return current;

    }

    return 0;

}

static unsigned int protocol_step(struct service_backend *backend, struct service_state *state, unsigned int id, unsigned int current)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return 0;

    switch (header.mode & 0xF000)
    {

    case 0x4000:
        return stepdirectory(backend, id, (id == current) ? 0 : current);

    }

    return 0;

}

static unsigned int protocol_open(struct service_backend *backend, struct service_state *state, unsigned int id)
{

    return id;

}

static unsigned int protocol_close(struct service_backend *backend, struct service_state *state, unsigned int id)
{

    return id;

}

static unsigned int readfile(struct service_backend *backend, void *buffer, unsigned int count, unsigned int offset, unsigned int id, struct cpio_header *header)
{

    unsigned int s = cpio_filesize(header) - offset;
    unsigned int o = cpio_filedata(header, id) + offset;

    return backend->read(buffer, (count > s) ? s : count, o);

}

static unsigned int readdirectory(struct service_backend *backend, void *buffer, unsigned int count, unsigned int offset, unsigned int current, struct cpio_header *header)
{

    struct record record;
    struct cpio_header eheader;

    if (!current)
        return 0;

    if (!readheader(backend, &eheader, current))
        return 0;

    record.id = current;
    record.size = cpio_filesize(&eheader);
    record.length = readname(backend, &eheader, current, record.name, RECORD_NAMESIZE, header->namesize) - 1;

    return memory_read(buffer, count, &record, sizeof (struct record), offset);

}

static unsigned int protocol_read(struct service_backend *backend, struct service_state *state, unsigned int id, unsigned int current, void *buffer, unsigned int count, unsigned int offset)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return 0;

    switch (header.mode & 0xF000)
    {

    case 0x8000:
        return readfile(backend, buffer, count, offset, id, &header);

    case 0x4000:
        return readdirectory(backend, buffer, count, offset, current, &header);

    }

    return 0;

}

static unsigned int writefile(struct service_backend *backend, void *buffer, unsigned int count, unsigned int offset, unsigned int id, struct cpio_header *header)
{

    unsigned int s = cpio_filesize(header) - offset;
    unsigned int o = cpio_filedata(header, id) + offset;

    return backend->write(buffer, (count > s) ? s : count, o);

}

static unsigned int protocol_write(struct service_backend *backend, struct service_state *state, unsigned int id, unsigned int current, void *buffer, unsigned int count, unsigned int offset)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return 0;

    switch (header.mode & 0xF000)
    {

    case 0x8000:
        return writefile(backend, buffer, count, offset, id, &header);

    }

    return 0;

}

static unsigned int protocol_seek(struct service_backend *backend, struct service_state *state, unsigned int id, unsigned int offset)
{

    return offset;

}

static unsigned int protocol_map(struct service_backend *backend, struct service_state *state, unsigned int id)
{

    struct cpio_header header;

    if (!readheader(backend, &header, id))
        return 0;

    return backend->map(cpio_filedata(&header, id), cpio_filesize(&header));

}

void service_setupcpio(void)
{

    service_initprotocol(&protocol, 1000, protocol_match, protocol_root, protocol_parent, protocol_child, protocol_create, protocol_destroy, protocol_step, protocol_open, protocol_close, protocol_read, protocol_write, protocol_seek, protocol_map);
    resource_register(&protocol.resource);

}

