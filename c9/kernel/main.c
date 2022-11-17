#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"

void k_thread_a(void*);
void k_thread_b(void*);

int main(void) {
   put_str("I am kernel\n");
   init_all();
   /*  thread_id, thread_type, func_ptr, func_args  */
   thread_start("k_thread_a", 31, k_thread_a, "argA ");
   thread_start("k_thread_b", 8, k_thread_b, "argB ");

   intr_enable();    // 打开中断, 使时钟中断起作用
   while(1) {
      put_str("Main ");
   };
   return 0;
}

void k_thread_a(void* arg) {
   /* 用void来通用表示参数, 被调用的函数知道自己需要什么类型的参数, 自己转换调用 */
   char* para = arg;
   while(1) {
      put_str(para);
   }
}

void k_thread_b(void* arg) {
   char* para = arg;
   while(1) {
      put_str(para);
   }
}