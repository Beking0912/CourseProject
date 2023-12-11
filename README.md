## Contribution
- Part 1: Biqing Su(bsu5)
- Part 2: Veerendra Gottiveeti (vrgottiv)

## Compile & Run
It's for LLVM 17.
```
chmod +x setup_and_run.sh
./setup_and_run.sh
```

# Part 1
## Input
```c
#include <stdio.h>

int c;

void fun(int a) {
  printf("Value of a is %d\n", a);
}

int main() {
  void (*fun_ptr)(int) = &fun;
  (*fun_ptr)(10);

  int b;
  for (c = 0; c < 3; c++) {
    b = c + 1;
  }

  return c;
}
```

## Output
```
main: func_0x147e4af98
Branch Dictionary:
br_0: fileX.c, 14, 15
br_1: fileX.c, 14, 18

Value of a is 10
br_0
br_0
br_0
br_1
```

## Test Files

### **test1.c** - For Part 1 & 3
- **Type:** small contrived program
- **Code source:** from project description
- **Run test:** execute the `setup_and_run.sh` script.
- **Result:**
<img src="https://s2.loli.net/2023/12/11/rLUSTMjKIlOZw9A.png" width="50%">

### **test2.c** - For Part 1 & 3
- **Type:** small contrived program
- **Code source:** generate by gpt4
- **Run test:** change `test0` to `test1` in `setup_and_run.sh` before executing the script.
- **Result:**
<img src="https://s2.loli.net/2023/12/11/IN2onFDPiHdLk4A.png" width="50%">

### **test3.c** - For Part 1 & 3
- **Type:** real-world substitute
- **Code source:** [Employee Management System github link](https://github.com/ssoad/Employee-Management-System/blob/master/Employee%20Management%20System-github.c)
- **Number of non-comment non-blank lines:** 583 lines
- **Changes made:** added function getch; moved the main to the end.
- **Run test:** change `test0` to `test3` in `setup_and_run.sh` before executing the script.
- **Result:**
<img src="https://s2.loli.net/2023/12/11/OuDQChPKmrBtyVc.png" width="50%">

# Part 2
## Input
```c
#include <stdio.h>
#include <stdlib.h> 

int main() {
    char str1[1000];
    FILE *name = fopen("file.txt", "r");
    int c;
    int len = 0;
    if (name == NULL) {
        perror("Error opening file");
        return -1;
    }
    while (1) {
        c = getc(name);
        if (c == EOF) break;
        str1[len++] = c;
        if (len >= 999) break;
    }
    str1[len] = '\0';
    printf("%s\n", str1);
    fclose(name);
    return 0;
}
```

## Output
```
Seminal Inputs Detection:
Line 6: name
```

## Test Files
### **test4.c** - For Part 2 & 3
- **Type:** small contrived program
- **Code source:** from project description
- **Run test:** change `test0` to `test1` in `setup_and_run.sh` before executing the script.
- **Result:**
<img src="https://s2.loli.net/2023/12/11/fB2gUZ36xOKTvha.png" width="50%">

### **test5.c** - For Part 2 & 3
- **Type:** small contrived program
- **Code source:** from project description
- **Run test:** change `test0` to `test1` in `setup_and_run.sh` before executing the script.
- **Result:**
<img src="https://s2.loli.net/2023/12/11/TGwaouyHV4BZFkO.png" width="50%">

# Repo Links
1. Part 1 Goal 1 dev & submission: https://github.com/Beking0912/llvm-pass-skeleton
2. Part 1 Goal 2 dev: https://github.com/Beking0912/valgrind-customize-tool
3. Part 1 Goal 2 submission: https://github.com/Beking0912/valgrind-submission
4. part 2 dev: https://github.com/vrgottiv/CompilerProject
5. part 2 submission: https://github.com/VeerendraG28/CourseProject
6. Part 3: https://github.com/Beking0912/CourseProject

# Manually Compile & Run
```
$ mkdir build
$ cd build
$ cmake ..
$ make
$ cd ..

$ cc -c logPrint.c
$ clang -fpass-plugin='build/skeleton/SkeletonPass.dylib' -c test1.c -g
$ cc test1.o logPrint.o
$ ./a.out
```
