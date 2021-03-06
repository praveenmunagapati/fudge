struct video_interface
{

    struct resource resource;
    struct system_node root;
    struct system_node ctrl;
    struct system_node data;
    struct system_node colormap;
    struct system_node event;
    struct list eventstates;
    struct spinlock eventlock;
    unsigned int id;
    struct ctrl_videosettings settings;
    void (*setmode)(struct ctrl_videosettings *settings);
    unsigned int (*rdata)(unsigned int offset, void *buffer, unsigned int count);
    unsigned int (*wdata)(unsigned int offset, void *buffer, unsigned int count);
    unsigned int (*rcolormap)(unsigned int offset, void *buffer, unsigned int count);
    unsigned int (*wcolormap)(unsigned int offset, void *buffer, unsigned int count);

};

void video_notifymode(struct video_interface *interface, unsigned int w, unsigned int h, unsigned int bpp);
void video_registerinterface(struct video_interface *interface, unsigned int id);
void video_unregisterinterface(struct video_interface *interface);
void video_initinterface(struct video_interface *interface, void (*setmode)(struct ctrl_videosettings *settings), unsigned int (*rdata)(unsigned int offset, void *buffer, unsigned int count), unsigned int (*wdata)(unsigned int offset, void *buffer, unsigned int count), unsigned int (*rcolormap)(unsigned int offset, void *buffer, unsigned int count), unsigned int (*wcolormap)(unsigned int offset, void *buffer, unsigned int count));
