#ifndef MEMORY_H
#define MEMORY_H

extern void *memory_copy(void *dest, const void *src, unsigned int count);
extern void *memory_set(void *dest, int value, unsigned int count);
extern unsigned short *memory_setw(unsigned short *dest, unsigned short value, unsigned int count);

#endif

