#include <stddef.h>
#include "include/string.h"
#include "include/liballoc.h"

int strtok(char *srcstr, char sep, char ***output)
{
    /* Assume we have a properly NULL-terminated string. */
    int len = strlen(srcstr);

    /* Initialize the number of tokens to 0. */
    int numparts = 0;

    /* A temporary pointer to a char pointer we'll use later. */
    char **currentpart;

    /*
     * Replace every instance of sep with a NULL character.
     * This has the effect of turning one string into many; you can
     * think of the first string beginning where the original string
     * did, and ending at the first NULL, then the second string
     * beginning at the character right after that NULL.
     */
    int i;
    for (i = 0; i < len; i++)
    {
        if (srcstr[i] == sep)
        {
            srcstr[i] = '\0';
            numparts++;
        }
    }

    /*
     * Increment numparts one more time, for the last substring.
     * i.e. if we found N separators we have N+1 parts. If two
     * separator characters appear together, or there is a
     * separator at the end, then some of these become the empty
     * string "".
     */
    numparts++;

    /* Allocate space for our pointers to the various substrings
     * Note that here we dereference output once, to assign to the
     * char ** we declared in the calling function (i.e. main())
     */
    *output = malloc(numparts * sizeof(char *));

    /* You should always check the return value of malloc! */
    /*   if (*output == NULL)
      {
          perror("malloc() call in str_separate");
          exit(1);
      } */

    /* Start with currentpart pointing to the first allocated pointer.
     * We'll use this to step through the array of char * pointers.
     */
    currentpart = *output;

    /*
     * The first char * pointer should point to the beginning of the
     * original string, as that is necessarily the first token.
     */
    *currentpart = srcstr;

    /*
     * We've placed \0's in the place of every occurrence of sep; so
     * a new token/substring begins at the character AFTER a \0
     * (excluding the very last \0, which we will never hit with this
     * loop).
     *
     * This loop finds all the NULLs we placed in the string, and stores
     * a pointer to the character after each NULL in *currentpart, after
     * incrementing currentpart to move to the next char * slot.
     */

    for (i = 0; i < len; i++) /* < len: we never get to the last '\0',
                               * since strlen gives us the length of
                               * the string, not including the NULL */
    {
        if (srcstr[i] == '\0')
        {
            currentpart++;
            *currentpart = &(srcstr[i + 1]);
        }
    }

    /*
     * We only return the integer numparts, but in fact we are
     * "implicitly returning" the contents of *output, since this
     * was declared outside str_separate but modified by it.
     */
    return numparts;
}
