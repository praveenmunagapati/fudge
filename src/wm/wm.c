#include <abi.h>
#include <fudge.h>
#include <event/event.h>
#include <math/box.h>
#include <keymap/keymap.h>
#include <widget/widget.h>
#include "render.h"

#define REMOTES                         64
#define VIEWS                           8

static struct remote
{

    struct list_item item;
    struct widget_window window;
    unsigned int source;

} remotes[REMOTES];

static struct view
{

    struct list_item item;
    struct list remotes;
    unsigned int center;
    struct widget_panel panel;
    char numberstring;
    struct remote *currentremote;

} views[VIEWS];

static unsigned int quit;
static unsigned int keymod = KEYMOD_NONE;
static char outputdata[FUDGE_BSIZE];
static struct ring output;
static struct box size;
static struct box body;
static struct list viewlist;
static struct list remotelist;
static struct widget_fill background;
static struct widget_mouse mouse;
static struct view *currentview = &views[0];
static struct event_handlers handlers;
static unsigned int padding;
static unsigned int lineheight;
static unsigned int steplength;

static void updateremote(struct event_header *header, struct remote *remote)
{

    widget_update(&output, &remote->window, WIDGET_Z_MIDDLE, header->destination, WIDGET_TYPE_WINDOW, sizeof (struct widget_window), remote->window.size.x, remote->window.size.y, remote->window.size.w, remote->window.size.h);
    ring_write(&output, &remote->window, sizeof (struct widget_window));

}

static void updateview(struct event_header *header, struct view *view)
{

    widget_update(&output, &view->panel, WIDGET_Z_MIDDLE, header->destination, WIDGET_TYPE_PANEL, sizeof (struct widget_panel) + view->panel.length, view->panel.size.x, view->panel.size.y, view->panel.size.w, view->panel.size.h);
    ring_write(&output, &view->panel, sizeof (struct widget_panel));
    ring_write(&output, &view->numberstring, view->panel.length);

}

static void updatemouse(struct event_header *header)
{

    widget_update(&output, &mouse, WIDGET_Z_TOP, header->destination, WIDGET_TYPE_MOUSE, sizeof (struct widget_mouse), mouse.size.x, mouse.size.y, mouse.size.w, mouse.size.h);
    ring_write(&output, &mouse, sizeof (struct widget_mouse));

}

static void updatebackground(struct event_header *header)
{

    widget_update(&output, &background, WIDGET_Z_BOTTOM, header->destination, WIDGET_TYPE_FILL, sizeof (struct widget_fill), background.size.x, background.size.y, background.size.w, background.size.h);
    ring_write(&output, &background, sizeof (struct widget_fill));

}

static void removeremote(struct event_header *header, struct remote *remote)
{

    widget_remove(&output, &remote->window, WIDGET_Z_MIDDLE, header->destination);

}

static void removeview(struct event_header *header, struct view *view)
{

    widget_remove(&output, &view->panel, WIDGET_Z_MIDDLE, header->destination);

}

static void removemouse(struct event_header *header)
{

    widget_remove(&output, &mouse, WIDGET_Z_TOP, header->destination);

}

static void removebackground(struct event_header *header)
{

    widget_remove(&output, &background, WIDGET_Z_BOTTOM, header->destination);

}

static void activateview(struct view *view)
{

    view->panel.active = 1;

}

static void activateremote(struct remote *remote)
{

    remote->window.active = 1;

}

static void deactivateview(struct view *view)
{

    view->panel.active = 0;

}

static void deactivateremote(struct remote *remote)
{

    remote->window.active = 0;

}

static void resizeremote(struct remote *remote, unsigned int x, unsigned int y, unsigned int w, unsigned int h)
{

    box_setsize(&remote->window.size, x, y, w, h);
    event_sendwmresize(FILE_L2, remote->source, remote->window.size.x + 2, remote->window.size.y + 2, remote->window.size.w - 4, remote->window.size.h - 4, padding, lineheight);

}

static void showremotes(struct event_header *header, struct list *remotes)
{

    struct list_item *current;

    for (current = remotes->head; current; current = current->next)
    {

        struct remote *remote = current->data;

        event_sendwmshow(FILE_L2, remote->source);
        updateremote(header, remote);

    }

}

static void hideremotes(struct event_header *header, struct list *remotes)
{

    struct list_item *current;

    for (current = remotes->head; current; current = current->next)
    {

        struct remote *remote = current->data;

        event_sendwmhide(FILE_L2, remote->source);
        removeremote(header, remote);

    }

}

static void flipview(struct event_header *header, struct view *view)
{

    deactivateview(currentview);
    hideremotes(header, &currentview->remotes);
    updateview(header, currentview);

    currentview = view;

    activateview(currentview);
    showremotes(header, &currentview->remotes);
    updateview(header, currentview);

}

static void arrangesingle(struct view *view)
{

    resizeremote(view->remotes.tail->data, body.x, body.y, body.w, body.h);

}

static void arrangetiled(struct view *view)
{

    unsigned int y = body.y;
    unsigned int h = body.h / (view->remotes.count - 1);
    struct list_item *current;

    resizeremote(view->remotes.tail->data, body.x, body.y, view->center, body.h);

    for (current = view->remotes.tail->prev; current; current = current->prev)
    {

        resizeremote(current->data, body.x + view->center, y, body.w - view->center, h);

        y += h;

    }

}

static void arrangeview(struct view *view)
{

    switch (view->remotes.count)
    {

    case 0:
        break;

    case 1:
        arrangesingle(view);

        break;

    default:
        arrangetiled(view);

        break;

    }

}

static void onkeypress(struct event_header *header, struct event_keypress *keypress)
{

    struct view *nextview;
    struct remote *nextremote;

    keymod = keymap_modkey(keypress->scancode, keymod);

    if (!(keymod & KEYMOD_ALT))
    {

        if (currentview->currentremote)
            event_sendwmkeypress(FILE_L2, currentview->currentremote->source, keypress->scancode);

        return;

    }

    switch (keypress->scancode)
    {

    case 0x02:
    case 0x03:
    case 0x04:
    case 0x05:
    case 0x06:
    case 0x07:
    case 0x08:
    case 0x09:
        nextview = &views[keypress->scancode - 0x02];

        if (nextview == currentview)
            break;

        if (currentview->currentremote && keymod & KEYMOD_SHIFT)
        {

            list_move(&nextview->remotes, &currentview->currentremote->item);

            currentview->currentremote = (currentview->remotes.tail) ? currentview->remotes.tail->data : 0;

            if (currentview->currentremote)
                activateremote(currentview->currentremote);

            arrangeview(currentview);
            arrangeview(nextview);

            if (nextview->currentremote)
                deactivateremote(nextview->currentremote);

            nextview->currentremote = (nextview->remotes.tail) ? nextview->remotes.tail->data : 0;

        }

        flipview(header, nextview);

        break;

    case 0x10:
        if (!(keymod & KEYMOD_SHIFT))
            break;

        if (!currentview->currentremote)
            break;

        event_sendwmhide(FILE_L2, currentview->currentremote->source);
        event_sendwmexit(FILE_L2, currentview->currentremote->source);

        break;

    case 0x19:
        if (!(keymod & KEYMOD_SHIFT))
            break;

        if (!file_walk(FILE_CP, "/bin/wshell"))
            break;

        call_spawn();

        break;

    case 0x1C:
        if (!currentview->currentremote)
            break;

        list_move(&currentview->remotes, &currentview->currentremote->item);
        arrangeview(currentview);
        showremotes(header, &currentview->remotes);

        break;

    case 0x23:
        if (currentview->center <= 6 * steplength)
            break;

        currentview->center -= 4 * steplength;

        arrangeview(currentview);
        showremotes(header, &currentview->remotes);

        break;

    case 0x24:
        if (!currentview->currentremote)
            break;

        nextremote = currentview->currentremote->item.next ? currentview->currentremote->item.next->data : currentview->remotes.head;

        if (!nextremote || nextremote == currentview->currentremote)
            break;

        deactivateremote(currentview->currentremote);
        updateremote(header, currentview->currentremote);

        currentview->currentremote = nextremote;

        activateremote(currentview->currentremote);
        updateremote(header, currentview->currentremote);

        break;

    case 0x25:
        if (!currentview->currentremote)
            break;

        nextremote = currentview->currentremote->item.prev ? currentview->currentremote->item.prev->data : currentview->remotes.tail;

        if (!nextremote || nextremote == currentview->currentremote)
            break;

        deactivateremote(currentview->currentremote);
        updateremote(header, currentview->currentremote);

        currentview->currentremote = nextremote;

        activateremote(currentview->currentremote);
        updateremote(header, currentview->currentremote);

        break;

    case 0x26:
        if (currentview->center >= 26 * steplength)
            break;

        currentview->center += 4 * steplength;

        arrangeview(currentview);
        showremotes(header, &currentview->remotes);

        break;

    case 0x2C:
        if (!(keymod & KEYMOD_SHIFT))
            break;

        event_sendwmhide(FILE_L2, header->destination);
        event_sendwmexit(FILE_L2, header->destination);

        break;

    }

}

static void onkeyrelease(struct event_header *header, struct event_keyrelease *keyrelease)
{

    keymod = keymap_modkey(keyrelease->scancode, keymod);

    if (!(keymod & KEYMOD_ALT))
    {

        if (currentview->currentremote)
            event_sendwmkeyrelease(FILE_L2, currentview->currentremote->source, keyrelease->scancode);

        return;

    }

}

static void onmousemove(struct event_header *header, struct event_mousemove *mousemove)
{

    mouse.size.x += mousemove->relx;
    mouse.size.y += mousemove->rely;

    if (mouse.size.x < size.x || mouse.size.x >= size.x + size.w)
        mouse.size.x = (mousemove->relx < 0) ? size.x : size.x + size.w - 1;

    if (mouse.size.y < size.y || mouse.size.y >= size.y + size.h)
        mouse.size.y = (mousemove->rely < 0) ? size.y : size.y + size.h - 1;

    updatemouse(header);

    if (currentview->currentremote)
        event_sendwmmousemove(FILE_L2, currentview->currentremote->source, mouse.size.x, mouse.size.y);

}

static void onmousepress(struct event_header *header, struct event_mousepress *mousepress)
{

    struct list_item *current;

    switch (mousepress->button)
    {

    case 0x01:
        for (current = viewlist.head; current; current = current->next)
        {

            struct view *view = current->data;

            if (view == currentview)
                continue;

            if (box_isinside(&view->panel.size, mouse.size.x, mouse.size.y))
            {

                flipview(header, view);

                break;

            }

        }

        for (current = currentview->remotes.head; current; current = current->next)
        {

            struct remote *remote = current->data;

            if (remote == currentview->currentremote)
                continue;

            if (box_isinside(&remote->window.size, mouse.size.x, mouse.size.y))
            {

                deactivateremote(currentview->currentremote);
                updateremote(header, currentview->currentremote);

                currentview->currentremote = remote;

                activateremote(currentview->currentremote);
                updateremote(header, currentview->currentremote);

                break;

            }

        }

        break;

    }

    if (currentview->currentremote)
        event_sendwmmousepress(FILE_L2, currentview->currentremote->source, mousepress->button);

}

static void onmouserelease(struct event_header *header, struct event_mouserelease *mouserelease)
{

    if (currentview->currentremote)
        event_sendwmmouserelease(FILE_L2, currentview->currentremote->source, mouserelease->button);

}

static void onvideomode(struct event_header *header, struct event_videomode *videomode)
{

    unsigned int factor = (videomode->h / 320);

    render_setpaint(videomode->bpp);

    switch (factor)
    {

    case 0:
        lineheight = 12 + factor * 2;
        padding = 4 + factor * 2;

        file_walk(FILE_L9, "/data/ter-112n.pcf");
        render_setfont(FILE_L9, lineheight, padding);
        render_setmouse(&mouse, 16);

        break;

    case 1:
        lineheight = 14 + factor * 2;
        padding = 4 + factor * 2;

        file_walk(FILE_L9, "/data/ter-114n.pcf");
        render_setfont(FILE_L9, lineheight, padding);
        render_setmouse(&mouse, 16);

        break;

    case 2:
        lineheight = 16 + factor * 2;
        padding = 4 + factor * 2;

        file_walk(FILE_L9, "/data/ter-116n.pcf");
        render_setfont(FILE_L9, lineheight, padding);
        render_setmouse(&mouse, 24);

        break;

    default:
        lineheight = 18 + factor * 2;
        padding = 4 + factor * 2;

        file_walk(FILE_L9, "/data/ter-118n.pcf");
        render_setfont(FILE_L9, lineheight, padding);
        render_setmouse(&mouse, 24);

        break;

    }

    event_sendwmresize(FILE_L2, header->destination, 0, 0, videomode->w, videomode->h, padding, lineheight);
    event_sendwmshow(FILE_L2, header->destination);

}

static void onwmmap(struct event_header *header)
{

    if (currentview->currentremote)
        deactivateremote(currentview->currentremote);

    currentview->currentremote = remotelist.head->data;
    currentview->currentremote->source = header->source;

    list_move(&currentview->remotes, &currentview->currentremote->item);
    activateremote(currentview->currentremote);
    arrangeview(currentview);
    showremotes(header, &currentview->remotes);
    event_sendwminit(FILE_L2, header->source);

}

static void onwmunmap(struct event_header *header)
{

    struct list_item *current;

    for (current = viewlist.head; current; current = current->next)
    {

        struct view *view = current->data;
        struct list_item *current2;

        for (current2 = view->remotes.head; current2; current2 = current2->next)
        {

            struct remote *remote = current2->data;

            if (header->source != remote->source)
                continue;

            removeremote(header, remote);
            list_move(&remotelist, &remote->item);

            view->currentremote = (view->remotes.tail) ? view->remotes.tail->data : 0;

            if (view->currentremote)
                activateremote(view->currentremote);

            arrangeview(view);

            if (view == currentview)
                showremotes(header, &view->remotes);

        }

    }

}

static void onwmexit(struct event_header *header)
{

    struct list_item *current;

    for (current = viewlist.head; current; current = current->next)
    {

        struct view *view = current->data;
        struct list_item *current2;

        for (current2 = view->remotes.head; current; current = current->next)
        {

            struct remote *remote = current2->data;

            event_sendwmhide(FILE_L2, remote->source);
            event_sendwmexit(FILE_L2, remote->source);

        }

    }

    quit = 1;

}

static void onwmresize(struct event_header *header, struct event_wmresize *wmresize)
{

    struct list_item *current;
    unsigned int i = 0;

    box_setsize(&size, wmresize->x, wmresize->y, wmresize->w, wmresize->h);
    box_setsize(&body, size.x, size.y + (wmresize->lineheight + wmresize->padding * 2), size.w, size.h - (wmresize->lineheight + wmresize->padding * 2));
    box_setsize(&background.size, size.x, size.y, size.w, size.h);

    steplength = body.w / 32;

    for (current = viewlist.head; current; current = current->next)
    {

        struct view *view = current->data;

        view->center = 18 * steplength;

        box_setsize(&view->panel.size, size.x + i * size.w / viewlist.count, size.y, size.w / viewlist.count, (wmresize->lineheight + wmresize->padding * 2));
        arrangeview(view);

        i++;

    }

    mouse.size.x = size.x + size.w / 4;
    mouse.size.y = size.y + size.h / 4;

}

static void onwmshow(struct event_header *header)
{

    struct list_item *current;

    updatebackground(header);
    updatemouse(header);

    for (current = viewlist.head; current; current = current->next)
        updateview(header, current->data);

    showremotes(header, &currentview->remotes);

}

static void onwmhide(struct event_header *header)
{

    struct list_item *current;

    removebackground(header);
    removemouse(header);

    for (current = viewlist.head; current; current = current->next)
        removeview(header, current->data);

    hideremotes(header, &currentview->remotes);

}

static void onwmflush(struct event_header *header)
{

    render_begin(FILE_L1);
    render_update(FILE_L6, size.w, size.h);
    render_complete();

}

static void setupviews(void)
{

    unsigned int i;

    for (i = 0; i < VIEWS; i++)
    {

        struct view *view = &views[i];

        list_inititem(&view->item, view);
        widget_initpanel(&view->panel, 0);
        list_add(&viewlist, &view->item);

        view->numberstring = '1' + i;
        view->panel.length = 1;

    }

}

static void setupremotes(void)
{

    unsigned int i;

    for (i = 0; i < REMOTES; i++)
    {

        struct remote *remote = &remotes[i];

        list_inititem(&remote->item, remote);
        widget_initwindow(&remote->window, 0);
        list_add(&remotelist, &remote->item);

    }

}

void main(void)
{

    handlers.keypress = onkeypress;
    handlers.keyrelease = onkeyrelease;
    handlers.mousemove = onmousemove;
    handlers.mousepress = onmousepress;
    handlers.mouserelease = onmouserelease;
    handlers.videomode = onvideomode;
    handlers.wmmap = onwmmap;
    handlers.wmunmap = onwmunmap;
    handlers.wmexit = onwmexit;
    handlers.wmresize = onwmresize;
    handlers.wmshow = onwmshow;
    handlers.wmhide = onwmhide;
    handlers.wmflush = onwmflush;

    ring_init(&output, FUDGE_BSIZE, outputdata);
    widget_initfill(&background, 2);
    widget_initmouse(&mouse, WIDGET_MOUSETYPE_DEFAULT);
    setupviews();
    setupremotes();
    activateview(currentview);

    if (!file_walk(FILE_L0, "/system/event"))
        return;

    if (!file_walk(FILE_L1, "/system/wm/data"))
        return;

    if (!file_walk(FILE_L2, "/system/wm/event"))
        return;

    if (!file_walk(FILE_L3, "/system/keyboard/if:0/event"))
        return;

    if (!file_walk(FILE_L4, "/system/mouse/if:0/event"))
        return;

    if (!file_walk(FILE_L5, "/system/video/if:0/event"))
        return;

    if (!file_walk(FILE_L6, "/system/video/if:0/data"))
        return;

    if (!file_walk(FILE_L7, "/system/video/if:0/ctrl"))
        return;

    if (!file_walk(FILE_L8, "/system/video/if:0/colormap"))
        return;

    file_open(FILE_L0);
    file_open(FILE_L1);
    file_open(FILE_L2);
    file_open(FILE_L3);
    file_open(FILE_L4);
    file_open(FILE_L5);
    render_init();
    render_setvideo(FILE_L7, 1024, 768, 32);
    render_setcolormap(FILE_L8);

    while (!quit && event_read(&handlers, FILE_L0))
    {

        if (ring_count(&output))
        {

            file_writeall(FILE_L1, outputdata, ring_count(&output));
            ring_reset(&output);
            event_sendwmflush(FILE_L2, EVENT_ADDR_BROADCAST);

        }

    }

    file_close(FILE_L5);
    file_close(FILE_L4);
    file_close(FILE_L3);
    file_close(FILE_L2);
    file_close(FILE_L1);
    file_close(FILE_L0);

}

