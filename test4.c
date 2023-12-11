#include <stdio.h>
#include <stdlib.h> 

int main() {
    char str1[1000]; // Array of characters, not character pointers
    FILE *name = fopen("file.txt", "r"); // Correct type is FILE, not File
    int c; // getc returns an int
    int len = 0;
    if (name == NULL) {
        perror("Error opening file");
        return -1;
    }
    while (1) {
        c = getc(name);
        if (c == EOF) break;
        str1[len++] = c;
        if (len >= 999) break; // Leave space for null terminator
    }
    str1[len] = '\0'; // Null-terminate the string
    printf("%s\n", str1); // Use straight quotes
    fclose(name); // Close the file
    return 0;
}

