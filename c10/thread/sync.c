#include "sync.h"
#include "interrupt.h"


/* 初始化信号量 */
void sema_init(struct semaphore* psema, uint8_t value) {
    psema->value = value;
    list_init(&psema->waiters);
}

/* 初始化锁plock */
void lock_init(struct lock* plock) {
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_init(&plock->semaphore, 1);        // 信号量初值为1
}

/* 信号量down操作 */
void sema_down(struct semaphore* psema) {
    /* 关中断来保证原子操作 */
    enum intr_status old_status = intr_disable();
    while (psema->value == 0) {     // 信号量值为0, 表示锁已被别的进程持有
        ASSERT(!elem_find(&psema->waiters, &running_thread()->general_tag));    // 当前进程不应该在信号量的waiter队列中
        if (elem_find(&psema->waiters, &running_thread()->general_tag)) {   // 再次panic保险
            PANIC("sema down: thread blocked has been in waiters_list\n");
        }
        /* 若信号量的值为0, 当前进程做两件事: 把自己加入该锁的等待队列; 阻塞自己 */
        list_append(&psema->waiters, &running_thread()->general_tag);
        thread_block(TASK_BLOCKED);
    }
    /* 若value为1或被唤醒后 */
    psema->value--;
    ASSERT(psema->value == 0);
    /* 恢复之前的中断状态*/
    intr_set_status(old_status);
}