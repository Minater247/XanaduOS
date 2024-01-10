#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>
#include <stddef.h>

void *memset(void *s, int c, size_t n);
char* itoa(int num, char *buffer, int base);
int strcmp(const char *str1, const char *str2);
int strncmp(const char *str1, const char *str2, size_t n);
void *memcpy(void *dest, const void *src, size_t n);
size_t strlen(const char* str);

#endif