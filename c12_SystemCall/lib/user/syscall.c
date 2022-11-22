#include "syscall.h"
#include "stdint.h"

/* 无参数的系统调用, 注意大括号分行写要有\连续行符号 */
#define _syscall0(NUMBER) ({                                            \
    int retval;                                                         \
    asm volatile ("int $0x80" : "=a"(retval) : "a"(NUMBER) : "memory"); \
    retval;                                                             \
})

#define _syscall1(NUMBER, ARG1) ({                                            \
    int retval;                                                         \
    asm volatile ("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1) : "memory"); \
    retval;                                                             \
})

#define _syscall2(NUMBER, ARG1, ARG2) ({                                            \
    int retval;                                                         \
    asm volatile ("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2) : "memory"); \
    retval;                                                             \
})

#define _syscall3(NUMBER, ARG1, ARG2, ARG3) ({                                            \
    int retval;                                                         \
    asm volatile ("int $0x80" : "=a"(retval) : "a"(NUMBER), "b"(ARG1), "c"(ARG2), "d"(ARG3) : "memory"); \
    retval;                                                             \
})

/* 返回当前任务pid */
uint32_t getpid() {
    return _syscall0(SYS_GETPID);
}

/* 打印字符串str */
uint32_t write(char* str) {
    return _syscall1(SYS_WRITE, str);
}