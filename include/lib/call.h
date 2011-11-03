#ifndef LIB_CALL_H
#define LIB_CALL_H

struct file_info;

extern unsigned int call_open(char *path);
extern void call_close(unsigned int fd);
extern unsigned int call_read(unsigned int fd, void *buffer, unsigned int count);
extern unsigned int call_write(unsigned int fd, void *buffer, unsigned int count);
extern unsigned int call_info(unsigned int fd, struct file_info *info);
extern unsigned int call_execute(char *path, unsigned int argc, char **argv);
extern unsigned int call_load(char *path);
extern unsigned int call_unload(char *path);
extern unsigned int call_exit();
extern unsigned int call_wait();
extern unsigned int call_attach(int index, void (*handler)());
extern unsigned int call_detach(int index);
extern void call_halt();
extern void call_reboot();

#endif

