/*
This program creates a key file of specified length. The characters in the file generated will be any of the 27 allowed characters,
generated using the standard Unix randomization methods. Do not create spaces every five characters, as has been historically done.
Note that you specifically do not have to do any fancy random number generation: we’re not looking for cryptographically secure random number generation.
rand()Links to an external site. is just fine. The last character keygen outputs should be a newline.
Any error text must be output to stderr
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

char intToChar(int val)
{
    int base = 65;       // Capital A
    int modo = val % 27; // Limit to accepted chars
    int ascii;
    if (modo == 26)
    { // 26 will represent space
        ascii = 32;
    }
    else
    {
        ascii = base + modo; // Otherwise we get the character ascii value
    };

    return (char)ascii; // Cast on return
};

int charToInt(char c)
{
    int base = 65;
    int value;
    if ((int)c == 32)
    {
        value = 26; // 26 represents space
    }
    else
    {
        value = ((int)c) - 65; // subtract base (Capital A) to get val
    }

    return value;
};

void main(int argc, char **argv)
{
    // Check valid number of arguments
    if (argc != 2)
    {
        fprintf(stderr, "Invalid number of arguments.\n");
        exit(1);
    };
    // Parse input
    int numChars = atoi(argv[1]);
    if (!numChars || numChars < 1)
    {
        fprintf(stderr, "Invalid number of characters requested.\n");
        exit(1);
    };

    // Var declarations
    int i;
    char keyStr[numChars + 1];

    // Init rand
    srand(time(NULL));

    for (i = 0; i < numChars; i++)
    {
        int rand_num = rand() % 100;
        char letter = intToChar(rand_num);
        keyStr[i] = letter;
    };

    keyStr[numChars + 1] = '\0'; // Terminator
    printf("%s\n", keyStr);      // Add newline
    exit(0);
};