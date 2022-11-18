#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"

void k_thread_a(void*);
void k_thread_b(void*);
int main(void) {
   put_str("I am kernel\n");
   init_all();

   // thread_start("k_thread_a", 31, k_thread_a, "argA ");
   // thread_start("k_thread_b", 8, k_thread_b, "argB ");

   intr_enable();	// 打开中断,使时钟中断起作用
   while(1) {     // 注意while(1)里面可以为空,但是这句话不能被注释掉,否则程序就不知道跑哪里去了(没有人撑着中断)
      // console_put_str("Main ");
   };
   return 0;
}

/* 在线程中运行的函数 */
void k_thread_a(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      console_put_str(para);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {     
/* 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
   char* para = arg;
   while(1) {
      console_put_str(para);
   }
}
