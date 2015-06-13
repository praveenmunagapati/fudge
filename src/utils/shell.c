#include <abi.h>
#include <fudge.h>

static void interpret(struct buffer *buffer)
{

    char command[FUDGE_BSIZE];
    unsigned int count = buffer_rcfifo(buffer, 1, FUDGE_BSIZE, command);

    /* This is a temporary fix */
    if (memory_match(command, "cd ", 3))
    {

        unsigned int ok;

        if (count < 4)
            return;

        if (command[3] == '/')
            ok = call_walk(CALL_L1, CALL_PR, count - 5, command + 4);
        else
            ok = call_walk(CALL_L1, CALL_PW, count - 4, command + 3);

        if (ok)
        {

            call_walk(CALL_PW, CALL_L1, 0, 0);
            call_walk(CALL_CW, CALL_L1, 0, 0);

        }

        return;

    }

    if (!call_walk(CALL_CP, CALL_PR, 9, "bin/slang"))
        return;

    if (!call_walk(CALL_L0, CALL_PR, 12, "system/pipe/"))
        return;

    call_walk(CALL_L1, CALL_L0, 1, "0");
    call_walk(CALL_C0, CALL_L0, 1, "1");
    call_spawn();
    call_open(CALL_L1);
    call_write(CALL_L1, 0, 1, count, command);
    call_close(CALL_L1);

}

static void complete(struct buffer *buffer)
{

}

static void handle(struct buffer *buffer, unsigned char c)
{

    switch (c)
    {

    case '\0':
        break;

    case '\t':
        complete(buffer);

        break;

    case '\b':
    case 0x7F:
        if (!buffer_ecfifo(buffer, 1, 1))
            break;

        call_write(CALL_PO, 0, 3, 1, "\b \b");

        break;

    case '\r':
        c = '\n';

    case '\n':
        call_write(CALL_PO, 0, 1, 1, &c);
        buffer_wcfifo(buffer, 1, 1, &c);
        interpret(buffer);
        call_write(CALL_PO, 0, 2, 1, "$ ");

        break;

    default:
        buffer_wcfifo(buffer, 1, 1, &c);
        call_write(CALL_PO, 0, 1, 1, &c);

        break;

    }

}

void main()
{

    unsigned char buffer[FUDGE_BSIZE];
    unsigned int count, roff;
    unsigned char inputbuffer[FUDGE_BSIZE];
    struct buffer input;

    buffer_init(&input, FUDGE_BSIZE, inputbuffer);

    call_open(CALL_P0);
    call_open(CALL_PO);
    call_write(CALL_PO, 0, 2, 1, "$ ");

    for (roff = 0; (count = call_read(CALL_P0, roff, 1, FUDGE_BSIZE, buffer)); roff += count)
    {

        unsigned int i;

        for (i = 0; i < count; i++)
            handle(&input, buffer[i]);

    }

    call_close(CALL_PO);
    call_close(CALL_P0);

}
