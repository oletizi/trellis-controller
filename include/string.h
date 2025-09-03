/* Minimal string.h for bare metal ARM Cortex-M4 */
#ifndef _STRING_H
#define _STRING_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Basic memory operations */
void* memset(void* dest, int val, uint32_t len);
void* memcpy(void* dest, const void* src, uint32_t len);
int memcmp(const void* s1, const void* s2, uint32_t n);

/* String operations */
uint32_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, uint32_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, uint32_t n);

#ifdef __cplusplus
}
#endif

#endif /* _STRING_H */