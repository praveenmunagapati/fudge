void kernel_copydescriptors(struct task *source, struct task *target);
unsigned int kernel_setupbinary(struct task *task, unsigned int sp);
void kernel_setupramdisk(struct container *container, struct task *task, struct service_backend *backend);
void kernel_setup(void);
