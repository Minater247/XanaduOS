#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

void *memset(void *s, int c, size_t n);
char* itoa(int num, char *buffer, int base);

#endif