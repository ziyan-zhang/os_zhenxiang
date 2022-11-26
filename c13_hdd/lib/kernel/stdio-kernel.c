#ifndef __LIB_KERNEL_STDIOKERNEL_H
#define __LIB_KERNEL_STDIOKERNEL_H

#include "stdio-kernel.h"
#include "console.h"
#include "stdio.h"

/* 供内核使用的格式化输出函数 */
void printk(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char buf[1024] = {0};
    vsprintf(buf, format, args);
    va_end(args);
    console_put_str(buf);
}

#endif