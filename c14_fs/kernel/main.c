#include "print.h"
#include "init.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "process.h"
#include "syscall-init.h"
#include "syscall.h"
#include "stdio.h"
#include "memory.h"
#include "fs.h"
#include "string.h"

void k_thread_a(void*);
void k_thread_b(void*);
void u_prog_a(void);
void u_prog_b(void);

int main(void) {
	put_str("I am kernel\n");
	init_all();
   uint32_t fd;
   fd = sys_open("/file1", O_CREAT);
   printf("opened with create fd: %d\n", fd);
   sys_close(fd);

   fd = sys_open("/file1", O_RDWR);
   printf("opened with read/write fd:%d\n", fd);
   sys_write(fd, "hello,world\n", 12);
   sys_write(fd, "hello,world\n", 12);
   sys_close(fd);

   fd = sys_open("/file1", O_RDWR);
   printf("open /file1, fd:%d\n", fd);
   char buf[64] = {0};
   int read_bytes = sys_read(fd, buf, 18);
   printf("1_ read %d bytes:\n%s\n", read_bytes, buf);

   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 6);
   printf("2_ read %d bytes:\n%s", read_bytes, buf);

   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 6);
   printf("3_ read %d bytes:\n%s\n", read_bytes, buf);

   printf("________  SEEK_SET 0  ________\n");
   sys_lseek(fd, 0, SEEK_SET);
   memset(buf, 0, 64);
   read_bytes = sys_read(fd, buf, 24);
   printf("4_ read %d bytes:\n%s", read_bytes, buf);

   sys_close(fd);

   // printf("%d closed now\n", fd);
   while(1);
   return 0;
}

// int main(void) {
// 	put_str("I am kernel\n");
// 	init_all();
//    uint32_t fd;
//    fd = sys_open("/file1", O_CREAT);
//    printf("opened with create fd: %d\n", fd);
//    sys_close(fd);

//    uint32_t fd0 = sys_open("/file1", O_RDWR);
//    printf("opened with read/write fd0:%d\n", fd0);
//    sys_write(fd0, "vvvvvv", 6);

//    uint32_t fd1 = sys_open("/file1", O_RDWR);
//    printf("opened with read/write fd1:%d\n", fd1);
//    sys_write(fd1, "hello,", 6);

//    // sys_close(fd2);
//    sys_close(fd1);
//    sys_close(fd0);


//    uint32_t fd2 = sys_open("/file1", O_RDWR);
//    char buf[64] = {0};
//    int read_bytes = sys_read(fd2, buf, 12);
//    printf("fd2_ read %d bytes:\n%s\n", read_bytes, buf);

//    sys_close(fd2);
//    // sys_close(fd1);

//    uint32_t fd3 = sys_open("/file1", O_RDWR);
//    printf("open /file1, fd3:%d\n", fd3);
//    char buf2[64] = {0};
//    read_bytes = sys_read(fd3, buf2, 12);
//    printf("fd3_ read %d bytes:\n%s\n", read_bytes, buf2);
//    sys_close(fd3);

//    // printf("%d closed now\n", fd);
//    while(1);
//    return 0;
// }

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
   void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_a malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while(1);
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
   void* addr1 = sys_malloc(256);
   void* addr2 = sys_malloc(255);
   void* addr3 = sys_malloc(254);
   console_put_str(" thread_b malloc addr:0x");
   console_put_int((int)addr1);
   console_put_char(',');
   console_put_int((int)addr2);
   console_put_char(',');
   console_put_int((int)addr3);
   console_put_char('\n');

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   sys_free(addr1);
   sys_free(addr2);
   sys_free(addr3);
   while(1);
}

/* 测试用户进程 */
void u_prog_a(void) {
   void* addr1 = malloc(256);
   void* addr2 = malloc(255);
   void* addr3 = malloc(254);
   printf(" prog_a malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   free(addr1);
   free(addr2);
   free(addr3);
   while(1);
}

/* 测试用户进程 */
void u_prog_b(void) {
   void* addr1 = malloc(256);
   void* addr2 = malloc(255);
   void* addr3 = malloc(254);
   printf(" prog_b malloc addr:0x%x,0x%x,0x%x\n", (int)addr1, (int)addr2, (int)addr3);

   int cpu_delay = 100000;
   while(cpu_delay-- > 0);
   free(addr1);
   free(addr2);
   free(addr3);
   while(1);
}
