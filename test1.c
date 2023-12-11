#include <stdio.h>

// Define a function matching the signature of the function pointer
void fun() {
    printf("Function fun called\n");
}

int main() {
    // Function pointer declaration and initialization
    void (*fun_ptr)() = &fun;
    
    // Call the function through the pointer
    (*fun_ptr)();

    // Simple loop to perform some operations
    int c = 10;
    for (int i = 0; i < 3; i++) {
        c = c + 1;
    }

    // Return a value
    return c;
}
