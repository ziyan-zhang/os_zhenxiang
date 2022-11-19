#include "ioqueue.h"
#include "interrupt.h"
#include "global.h"
#include "debug.h"

/* 初始化IO队列ioq */
void ioqueue_init(struct ioqueue* ioq) {
    lock_init(&ioq->lock);
    ioq->producer = ioq->consumer = NULL;
    ioq->head = ioq->tail = 0;
}

/* 返回pos在缓冲区的下一个位置值 */
static int32_t next_pos(int32_t pos) {
    return (pos+1) % bufsize;
}

/* 判断队列是否已满 */
bool ioq_full(struct ioqueue* ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    return next_pos(ioq->head) == ioq->tail;
}

/* 判断队列是否为空 */
bool ioq_empty(struct ioqueue* ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    return ioq->head == ioq->tail;
}

/* 使当前生产者或消费者在此缓冲区上等待 */
// static函数, 用在某个操作ioq的函数中, 操作其waiter: 
// 把waiter置为当前线程自己, 并自己阻塞自己
//todo: 能多次wait吗?
static void ioq_wait(struct task_struct** waiter) {
    ASSERT(*waiter == NULL && waiter != NULL);
    *waiter = running_thread();
    thread_block(TASK_BLOCKED);
}

static void wakeup(struct task_struct** waiter) {
    ASSERT(*waiter != NULL);
    thread_unblock(*waiter);
    *waiter = NULL;
}

/* 消费者从ioq队列中获取一个字符 */
char ioq_getchar(struct ioqueue* ioq) {
    ASSERT(intr_get_status() == INTR_OFF);
    /* 若缓冲区为空, 则该ioq的consumer成为当前线程自己, 自身线程主动阻塞 */
    while(ioq_empty(ioq)) {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->consumer);
        lock_release(&ioq->lock);
    }

    char byte = ioq->buf[ioq->tail];
    ioq->tail = next_pos(ioq->tail);

    if (ioq->producer != NULL) {
        wakeup(&ioq->producer);
    }

    return byte;
}

/* 生产者往ioq队列中写入一个字符byte */
void ioq_putchar(struct ioqueue* ioq, char byte) {
    ASSERT(intr_get_status() == INTR_OFF);
    /* 若缓冲区已满, 把生产者ioq->producer置为自己,
    为的是当缓冲区里的东西被消费者取完后让消费者知道唤醒哪个生产者,
    也就是唤醒当前线程自己 */
    while (ioq_full(ioq)) {
        lock_acquire(&ioq->lock);
        ioq_wait(&ioq->producer);
        lock_release(&ioq->lock);
    }
    ioq->buf[ioq->head] = byte;
    ioq->head = next_pos(ioq->head);

    if (ioq->consumer != NULL) {
        wakeup(&ioq->consumer);
    }
}

// 起始从上面的实现中可以看到, 可以唤醒任意线程, 但只能阻塞自己线程