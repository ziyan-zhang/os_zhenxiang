#include "fork.h"
#include "string.h"
#include "global.h"
#include "thread.h"
#include "debug.h"
#include "process.h"

extern void intr_exit(void);	//在kernel.S中定义，今番引用过来

/* 将父进程的pcb拷贝给子进程 */
statis int32_t copy_pcb_vaddrbitmap_stack0(struct task_struct* child_thread, struct task_struct* parent_thread) {
	/* a 复制pcb所在的整个页，里面包含进程pcb信息及特权级0级的栈，里面包含了返回地址 */
	memcpy(child_thread, parent_thread, PG_SIZE);
	child_thread->pid = fork_pid();
	child_thread->elapsed_ticks = 0;
	child_thread->status = TASK_READY;
	child_thread->ticks = child_thread->priority;	//为新进程把时间片充满
	child_thread->parent_pid = parent_thread->pid;
	child_thread->general_tag.prev = child_thread->general_tag.next = NULL;
	child_thread->all_list_tag.prev = child_thread->all_list_tag.next = NULL;
	block_desc_init(child_thread->u_block_desc);
	/* 复制父进程的虚拟地址池位图 */
	uint32_t bitmap_pg_cnt = DIV_ROUND_UP((0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
	void* vaddr_btmp = get_kernel_pages(bitmap_pg_cnt);	//准备内存
	/* 此时child_thread->userprog_vaddr.vaddr_bitmap.bit还是指向父进程虚拟地址的位图地址
	 * 下面将child_thread->userprog_vaddr.vaddr_bitmap.bit
	 * 指向自己的位图vaddr_btmp*/
	memcpy(vaddr_btmp, child_thread->userprog_vaddr.vaddr_bitmap.bits, bitmap_pg_cnt*PG_SIZE);	//复制数据
	child_thread->userprog_vaddr.vaddr_bitmap.bits = vaddr_btmp;
	/* 调试用 */
	ASSERT(strlen(child_thread->name) < 11);
	strcat(child_thread->name, "_fork");
	return 0;
}

/* 复制子进程的进程体（代码和数据）及用户栈 */
static void copy_body_stack3(struct task_struct* child_thread, struct task_struct* parent_thread, void* buf_page) {
	uint8_t* vaddr_btmp = parent_thread->userprog_vaddr.vaddr_bitmap.bits;
	uint32_t btmp_bytes_len = parent_thread->userprog_vaddr.vaddr_bitmap.btmp_bytes_len;
	uint32_t vaddr_start = parent_thread->userprog_vaddr.vaddr_start;
	uint32_t idx_byte = 0;
	uint32_t idx_bit = 0;
	uint32_t prog_vaddr = 0;

	/* 在父进程的用户空间查找已有的数据页 */
	while (idx_byte < btmp_bytes_len) {
		if (vaddr_btmp[idx_byte]) {
			idx_bit = 0;
			while (idx_bit < 8) {
				if ((BITMAP_MASK << idx_bit) & vaddr_btmp[idx_byte]) {
					prog_vaddr = (idx_byte*8 + idx_bit)*PG_SIZE + vaddr_start;
					/* 下面的操作是将父进程用户空间的数据通过内核空间做中转，
					 * 最终复制到子进程的用户空间*/

					/* a 将父进程在用户空间的数据复制到内核缓冲区buf_page，
					 * 目的是下面切换到子进程的页表后，还能访问到父进程的数据*/
					memcpy(buf_page, (void*)prog_vaddr, PG_SIZE);

					/* b 将页表切换到子进程，目的是避免下面申请内存的函数将pte及pde安装到父进程的页表中 */
					page_dir_activate(child_thread);
					/* c 申请虚拟地址prog_vaddr */
					get_a_page_without_opvaddrbitmap(PF_USER, prog_vaddr);

					/* d 从内核缓冲区中将父进程数据复制到子进程的用户空间 */
					memcpy((void*)prog_vaddr, buf_page, PG_SIZE);

					/* e 恢复父进程页表 */
					page_dir_activate(parent_thread);
				}
				idx_bit++;
			}
		}
		idx_byte++;
	}
}