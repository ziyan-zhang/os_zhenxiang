#include "sync.h"
#include "interrupt.h"
#include "debug.h"
#include "thread.h"


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

/* 信号量down操作: 若信号量值为0, 当前线程把自己的general_tag加入到信号量等待队列中并阻塞自己; 从阻塞回来后,或信号量为1, 则信号量-1 */
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

/* 信号量up操作: 此时信号量的值一定为0. 有人等待则unblock队首等待者; 信号量+1 */
void sema_up(struct semaphore* psema) {
    /* 关中断, 保证原子操作 */
    enum intr_status old_status = intr_disable();
    ASSERT(psema->value == 0);
    if (!list_empty(&psema->waiters)) {
        struct task_struct* thread_blocked = elem2entry(struct task_struct, 
                                                general_tag, list_pop(&psema->waiters));
        thread_unblock(thread_blocked);
    }
    psema->value++;
    ASSERT(psema->value == 1);
    /* 恢复之前中断状态 */
    intr_set_status(old_status);
}

/* 获取锁plock */
void lock_acquire(struct lock* plock) {
    /* case1, 自己不持有锁: 降一个信号量, plock->holder改为当前线程, 重复次数自然变成1 */
    if (plock->holder != running_thread()) {
        sema_down(&plock->semaphore);   // 信号量为0, 当前线程阻塞自己; 阻塞回来或信号量为1, 信号量-1
        plock->holder = running_thread();
        ASSERT(plock->holder_repeat_nr == 0);
        plock->holder_repeat_nr = 1;
    } else { /* case2, 自己已持有锁但还未释放 */
        plock->holder_repeat_nr++;
    }
}

/* 释放锁 */
void lock_release(struct lock* plock) {
    ASSERT(plock->holder == running_thread());  // 释放锁的一定是当前线程
    /* case1, 重复请求数>1: 把重复请求数-1即可返回, 还不到真正放弃锁的时候 */
    if (plock->holder_repeat_nr > 1) {
        plock->holder_repeat_nr--;
        return;
    }
    ASSERT(plock->holder_repeat_nr == 1);
    // 把锁的持有者置空, 重复请求书清零
    plock->holder = NULL;
    plock->holder_repeat_nr = 0;
    sema_up(&plock->semaphore);     // 有人等待则unlock队首等者; 信号量+1
}