#include <abi.h>
#include <fudge.h>

static void readback(void)
{

    char buffer[FUDGE_BSIZE];
    unsigned int count;

    file_open(FILE_CO);

    while ((count = file_read(FILE_CO, buffer, FUDGE_BSIZE)))
        file_writeall(FILE_PO, buffer, count);

    file_close(FILE_CO);

}

static void interpret(struct ring *ring)
{

    char command[FUDGE_BSIZE];
    unsigned int count = ring_read(ring, command, FUDGE_BSIZE);

    if (count < 2)
        return;

    /* This is a temporary fix */
    if (memory_match(command, "cd ", 3))
    {

        if (count < 4)
            return;

        command[count - 1] = '\0';

        if (file_walk(FILE_L8, command + 3))
        {

            file_duplicate(FILE_PW, FILE_L8);
            file_duplicate(FILE_CW, FILE_L8);

        }

        return;

    }

    if (!file_walk(FILE_CP, "/bin/slang"))
        return;

    if (!file_walk(FILE_L8, "/system/pipe/clone"))
        return;

    file_walkfrom(FILE_CI, FILE_L8, "0");
    file_walkfrom(FILE_CO, FILE_L8, "1");
    file_open(FILE_CI);
    file_writeall(FILE_CI, command, count);
    call_spawn();
    file_close(FILE_CI);
    readback();

}

static void complete(struct ring *ring)
{

}

static void handle(struct ring *ring, unsigned char c)
{

    switch (c)
    {

    case '\0':
        break;

    case '\t':
        complete(ring);

        break;

    case '\b':
    case 0x7F:
        if (!ring_skipreverse(ring, 1))
            break;

        file_writeall(FILE_PO, "\b \b", 3);

        break;

    case '\r':
        c = '\n';

    case '\n':
        file_writeall(FILE_PO, &c, 1);
        ring_write(ring, &c, 1);
        interpret(ring);
        file_writeall(FILE_PO, "$ ", 2);

        break;

    default:
        ring_write(ring, &c, 1);
        file_writeall(FILE_PO, &c, 1);

        break;

    }

}

void main(void)
{

    unsigned char buffer[FUDGE_BSIZE];
    unsigned int count;
    unsigned char inputbuffer[FUDGE_BSIZE];
    struct ring input;

    ring_init(&input, FUDGE_BSIZE, inputbuffer);
    file_open(FILE_PI);
    file_open(FILE_PO);
    file_writeall(FILE_PO, "$ ", 2);

    while ((count = file_read(FILE_PI, buffer, FUDGE_BSIZE)))
    {

        unsigned int i;

        for (i = 0; i < count; i++)
            handle(&input, buffer[i]);

    }

    file_close(FILE_PO);
    file_close(FILE_PI);

}

