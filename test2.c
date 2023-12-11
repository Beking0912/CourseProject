#include <stdio.h>
#include <stdlib.h>

void function1() {
    printf("In function1\n");
}

void function2() {
    printf("In function2\n");
}

int main() {
    void (*fun_ptr)();
    int input;

    printf("Enter a number (1 or 2): ");
    scanf("%d", &input);

    if (input == 1) {
        fun_ptr = &function1;
    } else {
        fun_ptr = &function2;
    }

    (*fun_ptr)();

    int sum = 0;
    for (int i = 0; i < input; i++) {
        sum += i;
    }

    printf("Sum: %d\n", sum);

    return 0;
}
