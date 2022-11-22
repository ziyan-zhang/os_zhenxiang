#include "process.h"
#include "thread.h"
#include "global.h"
#include "memory.h"
#include "interrupt.h"
#include "string.h"
#include "debug.h"
#include "tss.h"
#include "console.h"

 // kernel.kernel.S:38, 弹出所有通用寄存器，弹出gs, fs, es, ds四个寄存器， 跳过error_code, 调用iretd
extern void intr_exit(void);

/* 构建用户进程上下文信息 */
void start_process(void* filename_) {
    void* function = filename_;
    struct task_struct* cur = running_thread();
    cur->self_kstack += sizeof(struct thread_stack);    // 此时cur->self_kstack指向了 cur指向的结构体中 intr_stack的底端（最低地址）
    struct intr_stack* proc_stack = (struct intr_stack*)cur->self_kstack;   // 可见进程栈用的中断栈的地址和形式
    /*proc_stack框出来后，指定一些段*/
    proc_stack->edi = proc_stack->esi = \
        proc_stack->ebp = proc_stack->esp_dummy = 0;
    proc_stack->ebx = proc_stack->edx = \
        proc_stack->ecx = proc_stack->eax = 0;
    proc_stack->gs = 0;
    proc_stack->ds = proc_stack->es = proc_stack->fs = SELECTOR_U_DATA;
    proc_stack->eip = function;
    proc_stack->cs = SELECTOR_U_CODE;
    proc_stack->eflags = (EFLAGS_IOPL_0 | EFLAGS_MBS | EFLAGS_IF_1);
    proc_stack->esp = (void*)((uint32_t)get_a_page(PF_USER, USER_STACK3_VADDR) + PG_SIZE);
    proc_stack->ss = SELECTOR_U_DATA;
    asm volatile ("movl %0, %%esp; jmp intr_exit" : : "g" (proc_stack) : "memory"); // 将proc_stack的值写到esp寄存器里面，并跳转到intr_stack
}

/* 激活页表 */
void page_dir_activate(struct task_struct* p_thread) {
    /* 上一次调度的可能是线程，但也可能是进程，为了防止线程使用别的进程的页表，所以对线程也要重新安装页表 */
    /* 若为内核线程，需要重新填充页表为0x100000 */
    uint32_t pagedir_phy_addr = 0x100000;
    /* 用户态进程有自己的页目录表 */
    if (p_thread->pgdir != NULL) {
        pagedir_phy_addr = addr_v2p((uint32_t)p_thread->pgdir);
    }
    /* 更新页目录寄存器cr3, 使新页表生效 */
    asm volatile ("movl %0, %%cr3" : : "r" (pagedir_phy_addr) : "memory");
}

/* 激活线程或进程的页表，更新tss中的esp0为进程的特权级0的栈 */
void process_activate(struct task_struct* p_thread) {
    ASSERT(p_thread != NULL);
    /* 激活线程或进程的页表 */
    page_dir_activate(p_thread);

    /* 内核进程的特权级本身就是0，处理器进入中断时并不会从tss中获取0特权级栈地址，故不需要更新esp0 */
    if (p_thread->pgdir) {
        update_tss_esp(p_thread);
    }
}

/* 创建页目录表，将当前页表的表示内核空间的pde复制，成功则返回页目录的虚拟地址，否则返回-1 */
uint32_t* create_page_dir(void) {
    /* 用户进程的页表不能让用户直接访问到，所以在内核空间申请 */
    uint32_t* page_dir_vaddr = get_kernel_pages(1);
    /* 申请完就判断，养成好习惯 */
    if (page_dir_vaddr == NULL) {
        console_put_str("create_page_dir: get_kernel_page failed!");
        return NULL;
    }

    /***********************   1 先复制页表      ********************/
    /* page_dir_vaddr + 0x300*4 是内核页目录的第768项 */
    memcpy((uint32_t*)((uint32_t)page_dir_vaddr + 0x300*4), (uint32_t*)(0xfffff000 + 0x300*4), 1024);

    /**********************     2 更新页目录地址    ******************/
    uint32_t new_page_dir_phy_addr = addr_v2p((uint32_t)page_dir_vaddr);
    /* 页目录地址是存入在页目录的最后一项，更新页目录地址为新页目录的物理地址 */
    page_dir_vaddr[1023] = new_page_dir_phy_addr | PG_US_U | PG_RW_W | PG_P_1;
    return page_dir_vaddr;
}

/* 创建用户进程虚拟地址位图 */
void create_user_vaddr_bitmap(struct task_struct* user_prog) {
    user_prog->userprog_vaddr.vaddr_start = USER_VADDR_START;   // 虚拟地址结构体包括：虚拟地址位图，和虚拟地位起始，Linux虚拟内存入口地址一般是0x8048000
    uint32_t bitmap_pg_cnt = DIV_ROUND_UP( (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8, PG_SIZE);
    user_prog->userprog_vaddr.vaddr_bitmap.bits = get_kernel_pages(bitmap_pg_cnt);
    user_prog->userprog_vaddr.vaddr_bitmap.btmp_bytes_len = (0xc0000000 - USER_VADDR_START) / PG_SIZE / 8;
    bitmap_init(&user_prog->userprog_vaddr.vaddr_bitmap);
}

/* 创建用户进程 */
void process_execute(void* filename, char* name) {
    /* pcb是内核中的数据结构，由内核来维护进程信息，因此要在内核内存池中申请 */ //TODO: 指的是他的地址在内核区域吗，还是PVL=0, 怎样理解真相还原512页内核用户进程的区别？
    struct task_struct* thread = get_kernel_pages(1);
    init_thread(thread, name, default_prio);    // 初始化线程基本信息：内核栈地址，名字，优先级
    create_user_vaddr_bitmap(thread);
    thread_create(thread, start_process, filename); // 初始化线程栈
    thread->pgdir = create_page_dir();

    enum intr_status old_status = intr_disable();
    
    ASSERT(!elem_find(&thread_ready_list, &thread->general_tag));
    list_append(&thread_ready_list, &thread->general_tag);
    ASSERT(!elem_find(&thread_all_list, &thread->all_list_tag));
    list_append(&thread_all_list, &thread->all_list_tag);

    intr_set_status(old_status);
}