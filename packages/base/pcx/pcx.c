#include <fudge.h>
#include <gfx/gfx.h>
#include <gfx/pcx.h>

static struct pcx_header header;

static unsigned int size()
{

    unsigned char buffer[FUDGE_BSIZE];
    unsigned int offset;
    unsigned int count;

    for (offset = 0; (count = call_read(FUDGE_IN, offset, FUDGE_BSIZE, buffer)); offset += count);

    return offset;

}

static void set_resolution()
{

    call_open(3, FUDGE_ROOT, 27, "system/video/vga/resolution");
    call_write(3, 0, 9, "320x200x8");
    call_close(3);

}

static void set_colormap()
{

    char colormap[769];
    unsigned int i;

    call_read(FUDGE_IN, size() - 769, 769, &colormap);

    if (colormap[0] != PCX_COLORMAP_MAGIC)
        return;

    call_open(3, FUDGE_ROOT, 25, "system/video/vga/colormap");

    for (i = 0; i < PCX_COLORMAP_SLOTS; i++)
        call_write(3, i * 4, 3, &colormap[i * 3 + 1]);

    call_close(3);

}

static void render()
{

    unsigned int width = header.xend - header.xstart + 1;
    unsigned int height = header.yend - header.ystart + 1;
    unsigned int scanline = header.nplanes * header.bpl;
    unsigned int offset = 128;
    unsigned int row;

    for (row = 0; row < height; row++)
    {

        char raw[FUDGE_BSIZE];
        char buffer[FUDGE_BSIZE];
        unsigned int rindex = 0;
        unsigned int bindex = 0;
        unsigned int count;
        char current;

        call_read(FUDGE_IN, offset, scanline, &raw);

        do
        {

            count = 1;
            current = raw[rindex];
            rindex++;

            if ((current & 0xC0) == 0xC0)
            {

                count = current & 0x3F;
                current = raw[rindex];
                rindex++;

            }

            while (count--)
            {

                buffer[bindex] = current;
                bindex++;

            }

        } while (bindex < width);

        offset += rindex;

        call_write(FUDGE_OUT, row * width, width, &buffer);

    }

}

void main()
{

    call_read(FUDGE_IN, 0, sizeof (struct pcx_header), &header);
    call_open(FUDGE_OUT, FUDGE_ROOT, 21, "system/video/vga/data");
    set_resolution();
    set_colormap();
    render();

}
