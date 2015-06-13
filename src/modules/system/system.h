#define SYSTEM_NODETYPE_NORMAL          0
#define SYSTEM_NODETYPE_GROUP           1
#define SYSTEM_NODETYPE_MULTI           2
#define SYSTEM_NODETYPE_MERGE           4
#define SYSTEM_NODETYPE_MAILBOX         8

struct system_header
{

    char id[12];
    unsigned int root;

};

struct system_node
{

    struct list_item item;
    struct system_node *parent;
    struct list children;
    struct list mailboxes;
    unsigned int type;
    const char *name;
    struct resource *resource;
    unsigned int index;
    unsigned int refcount;
    unsigned int (*open)(struct system_node *self);
    unsigned int (*close)(struct system_node *self);
    unsigned int (*read)(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer);
    unsigned int (*write)(struct system_node *self, unsigned int offset, unsigned int size, unsigned int count, void *buffer);
    unsigned int (*child)(struct system_node *self, unsigned int count, const char *path);

};

unsigned int system_open(struct system_node *node);
unsigned int system_close(struct system_node *node);
void system_addchild(struct system_node *group, struct system_node *node);
void system_removechild(struct system_node *group, struct system_node *node);
void system_registernode(struct system_node *node);
void system_unregisternode(struct system_node *node);
void system_initnode(struct system_node *node, unsigned int type, const char *name);