#ifndef ___SYS_H_
#define ___SYS_H_

        #define SYSCALL_INT     0x30

        #define SYS_GETCH       1
        #define SYS_WRITE       2


int os_write (int handle, void *buf, unsigned len);
int getch (void);

#endif //___SYS_H
