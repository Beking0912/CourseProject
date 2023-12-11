#include <stdio.h>

// Define a function matching the signature of the function pointer
void fun(int x) {
    printf("Function fun called with argument %d\n", x);
}

int main() {
    // Function pointer declaration and initialization
    void (*fun_ptr)(int) = &fun;
    
    // Call the function through the pointer with an argument
    int c = 10;
    (*fun_ptr)(c);

    // Simple loop to modify the value of c
    for (int i = 0; i < 3; i++) {
        c = c + 1;
    }

    // Return the modified value of c
    return c;
}
