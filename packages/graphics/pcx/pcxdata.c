#include <fudge.h>
#include <gfx/gfx.h>
#include <gfx/pcx.h>

static struct pcx_surface pcx;

void main()
{

    pcx_init_surface(&pcx, FUDGE_IN);
    pcx_load(&pcx);
    gfx_write_surface(FUDGE_OUT, &pcx.base);

}
