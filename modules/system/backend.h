struct system_backend
{

    struct vfs_backend base;
    struct system_group root;
    struct system_header header;

};

void system_init_backend(struct system_backend *backend);