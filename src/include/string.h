#ifndef _STRING_H
#define _STRING_H 1

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

    int memcmp(const void *, const void *, size_t);
    void *memcpy(void *__restrict, const void *__restrict, size_t);
    void *memmove(void *, const void *, size_t);
    void *memset(void *, int, size_t);
    size_t strlen(const char *);
    int strtok(char *srcstr, char sep, char ***output);
    int str2int(char str[]);
    uint64_t str2int2(char* str);

#ifdef __cplusplus
}
#endif

#endif
