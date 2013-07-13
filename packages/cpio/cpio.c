#include <fudge.h>
#include <format/cpio.h>

unsigned int find_top()
{

    struct cpio_header header;
    unsigned int offset = 0;
    unsigned int current = 0;
    unsigned short namesize = 0xFFFF;

    do
    {

        if (call_read(CALL_DI, offset, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
            break;

        if (!cpio_validate(&header))
            break;

        if ((header.mode & 0xF000) == 0x0000)
            continue;

        if (header.namesize < namesize)
        {

            namesize = header.namesize;
            current = offset;

        }

    } while ((offset = cpio_next(&header, offset)));

    return current;

}

unsigned int parent(unsigned int count, char *path)
{

    struct cpio_header header;
    unsigned char name[1024];
    unsigned int offset = 0;

    while (--count && path[count - 1] != '/');

    do
    {

        if (call_read(CALL_DI, offset, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
            break;

        if (!cpio_validate(&header))
            break;

        if ((header.mode & 0xF000) == 0x4000)
        {

            if (call_read(CALL_DI, offset + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
                break;

            if (memory_match(name, path, count - 1))
                return offset;

        }

    } while ((offset = cpio_next(&header, offset)));

    return 0;

}

unsigned int getslash(unsigned int count, const char *path)
{

    unsigned int i;

    for (i = 0; i < count; i++)
    {

        if (path[i] == '/')
            return i;

    }

    return count;

}

unsigned int walk(unsigned int id, unsigned int count, const char *path)
{

    struct cpio_header header;
    char name[1024];
    unsigned int length;

    if (!count)
        return id;

    if (id == 0xFFFFFFFF)
        id = find_top();

    if (call_read(CALL_DI, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
        return 0;

    if (!cpio_validate(&header))
        return 0;

    if (call_read(CALL_DI, id + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
        return 0;

    length = header.namesize;

    if (memory_match(path, "../", 3))
        return walk(parent(header.namesize, name), count - 3, path + 3);

    id = 0;

    do
    {

        unsigned int c;

        if (call_read(CALL_DI, id, sizeof (struct cpio_header), &header) < sizeof (struct cpio_header))
            break;

        if (!cpio_validate(&header))
            break;

        if (header.namesize < length)
            continue;

        if (header.namesize - length == 0)
            continue;

        if (call_read(CALL_DI, id + sizeof (struct cpio_header), header.namesize, name) < header.namesize)
            break;

        c = getslash(count, path);

        if (c + 1 != header.namesize - length)
            continue;

        if (memory_match(name + length, path, c))
        {

            call_write(CALL_DO, 0, header.namesize - length, name + length);
            call_write(CALL_DO, 0, 1, "\n");

            if (c == count)
                return walk(id, count - c, path + c);
            else
                return walk(id, count - c - 1, path + c + 1);

        }

    } while ((id = cpio_next(&header, id)));

    return 0;

}

void print_name(unsigned int offset)
{

    struct cpio_header header;
    unsigned char buffer[512];
    unsigned int count;

    call_read(CALL_DI, offset, sizeof (struct cpio_header), &header);

    if (!cpio_validate(&header))
        return;

    if ((header.mode & 0xF000) == 0x8000)
        call_write(CALL_DO, 0, 2, "D ");
    else if ((header.mode & 0xF000) == 0x4000)
        call_write(CALL_DO, 0, 2, "F ");
    else
        call_write(CALL_DO, 0, 2, "X ");

    count = call_read(CALL_DI, offset + sizeof (struct cpio_header), header.namesize, buffer);
    call_write(CALL_DO, 0, count, buffer);
    call_write(CALL_DO, 0, 1, "\n");

}

void print_names(unsigned int offset)
{

    struct cpio_header header;
    unsigned char buffer[512];
    unsigned int count;

    do
    {

        call_read(CALL_DI, offset, sizeof (struct cpio_header), &header);

        if (!cpio_validate(&header))
            break;

        if ((header.mode & 0xF000) == 0x8000)
            call_write(CALL_DO, 0, 2, "D ");
        else if ((header.mode & 0xF000) == 0x4000)
            call_write(CALL_DO, 0, 2, "F ");
        else
            call_write(CALL_DO, 0, 2, "X ");

        count = call_read(CALL_DI, offset + sizeof (struct cpio_header), header.namesize, buffer);
        call_write(CALL_DO, 0, count, buffer);
        call_write(CALL_DO, 0, 1, "\n");

    } while ((offset = cpio_next(&header, offset)));

}

void print_content(unsigned int offset)
{

    struct cpio_header header;
    unsigned char buffer[1024];
    unsigned int count;
    unsigned int size;

    call_read(CALL_DI, offset, sizeof (struct cpio_header), &header);

    if (!cpio_validate(&header))
        return;

    size = ((header.filesize[0] << 16) | header.filesize[1]);
    count = call_read(CALL_DI, offset + sizeof (struct cpio_header) + header.namesize + (header.namesize & 1), size, buffer);
    call_write(CALL_DO, 0, count, buffer);

}

void main()
{

/*
    print_names(0);
    print_content(0);
*/

    walk(0xFFFFFFFF, 13, "home/help.txt");

}
