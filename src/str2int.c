
#include "include/string.h"
#include <stdint.h>

// Function to convert string to
// integer without using functions

/**
* @brief Converts a string to an integer.
* @param str []
* @return The integer or NULL if the string is invalid
*/
int str2int(char str[])
{

    int i = 0;
    int result = 0;

    while (str[i] != '\0')
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

uint64_t str2int2(char* str) 
{ 
    uint64_t res = 0; // Initialize result 
  
    // Iterate through all characters of input string and 
    // update result 
    for (uint64_t i = 0; str[i] != '\0'; ++i) {
        uint64_t digit = str[i] - '0'; // convert char to int by subtracting ascii value of '0' from char's ascii value
        res = res * 10 + digit; // add the digit to the result multiplied by 10 (to shift it to the left)
    } 

  
    // return result. 
    return res; 
}