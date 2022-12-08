
#include "include/string.h"

// Function to convert string to
// integer without using functions
int str2int(char str[])
{

    int i = 0;
    int result = 0;

    while (str[i] != NULL)
    {

        if (str[i] < 48 || str[i] > 57)
        {

            return NULL;
        }
        else
        {

            result = result * 10 + (str[i] - 48);
            i++;
        }
    }

    return result;
}
