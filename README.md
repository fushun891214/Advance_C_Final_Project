# Adv C final project

## Commands
- [x] `ls`: list directory
- [x] `cd`: change directory
- [x] `rm`: remove
- [x] `mkdir`: make directory
- [x] `rmdir`: remove directory
- [x] `put`: put file into the space
- [x] `get`: get file from the space
- [x] `vi`:  Visual Editor
- [x] `cat`: show content
- [x] `status`: show status of the space
- [x] `help`
- [x] `exit and store img`


## Example
### Example 1
```shell
options:
1, loads from file
2. create new partition in memory
2

Input size of a new partition (example 102400 2048000)
partition size = 2048000

Make new partition successful !
List of commands
'ls' list directory
'cd' change directory
'rm' remove
'mkdir' make directory
'rmdir' remove directory
'put' put file into the space
'get' get file from the space
'cat' show content
'status' show status of the space
'help'
'exit and store img'
```

### Example 2
```shell
/ $ put hello.c
/ $ ls
hello.c
/ $ mkdir test
/ $ cd test
/test/ $ ls
/test/ $ put test1.o
failed to open file 'test1.o'
/test/ $ put test.o
/test/ $ ls
test2.o test.o
/test/ $ cd ..
/ $ ls
test hello.c
/ $ status
partition size: 2048000
total inodes: 221
used inodes: 5
total blocks: 2000
used blocks: 52
files' blocks: 21
block size: 1024
free space: 1994752
/ $ cat hello.c
#include <stdio.h>

int main (void) {
    printf("HELLO!!");
    return 0;
}
/ $ get hello.c
```

