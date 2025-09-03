/* Minimal string functions implementation */
#include "string.h"

void* memset(void* dest, int val, uint32_t len) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char v = (unsigned char)val;
    
    while (len--) {
        *d++ = v;
    }
    
    return dest;
}

void* memcpy(void* dest, const void* src, uint32_t len) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    while (len--) {
        *d++ = *s++;
    }
    
    return dest;
}

int memcmp(const void* s1, const void* s2, uint32_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (*str++) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

char* strncpy(char* dest, const char* src, uint32_t n) {
    char* d = dest;
    while (n-- && (*d++ = *src++));
    while (n--) {
        *d++ = '\0';
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char* s1, const char* s2, uint32_t n) {
    while (n-- && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (uint32_t)-1) {
        return 0;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}