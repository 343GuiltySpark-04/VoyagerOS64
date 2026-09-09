#include "include/mm/kmalloc.h"
#include "include/string.h"
#include <stddef.h>

int strtok(char *srcstr, char sep, char ***output)
{
    int len = strlen(srcstr);
    int numparts = 0;
    char **currentpart;

    for (int i = 0; i < len; i++)
    {
        if (srcstr[i] == sep)
        {
            srcstr[i] = '\0';
            numparts++;
        }
    }

    numparts++;
    *output = kmalloc((size_t) numparts * sizeof(char *));
    if (*output == NULL)
        return 0;

    currentpart = *output;
    *currentpart = srcstr;

    for (int i = 0; i < len; i++)
    {
        if (srcstr[i] == '\0')
        {
            currentpart++;
            *currentpart = &srcstr[i + 1];
        }
    }

    return numparts;
}
