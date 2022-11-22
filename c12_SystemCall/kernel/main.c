#include "print.h"
#include "init.h"
#include "debug.h"
#include "memory.h"
#include "thread.h"
#include "interrupt.h"
#include "console.h"
#include "ioqueue.h"
#include "keyboard.h"
#include "process.h"

void k_thread_a(void*);
void k_thread_b(void*);

void u_prog_a(void);
void u_prog_b(void);

int test_var_a = 0, test_var_b = 0;

int main(void) {
   put_str("I am kernel\n");
   init_all();

   thread_start("consumer_a", 31, k_thread_a, " A_");
   thread_start("consumer_b", 31, k_thread_b, " B_");
   process_execute(u_prog_a, "user_prog_a");
   process_execute(u_prog_b, "user_prog_b");
   
   intr_enable();
   while(1); 
   return 0;
}

/* 在线程中运行的函数, 用void*来通用表示参数,被调用的函数知道自己需要什么类型的参数,自己转换再用 */
void k_thread_a(void* arg) {
   while(1) {
	console_put_str("v_a:0x");
	console_put_int(test_var_a);
   }
}

/* 在线程中运行的函数 */
void k_thread_b(void* arg) {
	while(1) {
		console_put_str(" v_b:0x");
		console_put_int(test_var_b);
   }
}

/* 测试用户进程 */
void u_prog_a(void) {
	while(1) {
		test_var_a++;
	}
}

void u_prog_b(void) {
	while(1) {
		test_var_b++;
	}
}