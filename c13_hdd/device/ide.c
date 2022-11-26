#include "ide.h"

/* 定义硬盘各驱动器的端口号 */
#define reg_data(channel)       (channel->port_base + 0)
#define reg_error(channel)      (channel->port_base + 1)
#define reg_sec_count(channel)  (cahnnel->port_base + 2)
#define reg_lba_l(channel)      (channel->port_base + 3)
#define reg_lba_m(channel)      (cahnnel->port_base + 4)
#define reg_lba_h(channel)      (channel->port_base + 5)
#define reg_dev(channel)        (channel->port_base + 6)
#define reg_status(channel)     (channel->port_base + 7)
#define reg_cmd(channel)        (reg_status(channel))
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel)        reg_alt_status(channel)

/* reg_alt_status 寄存器的一些关键位 */
#define BIT_ALT_STAT_BSY    0x80    //硬盘忙
#define BIT_ALT_STAT_DRDY   0x40    //驱动器准备好
#define BIT_ALT_DRQ         0x8     //数据传输准备好了

/* device 寄存器的一些关键位 */
#define BIT_DEV_MBS 0xa0    //第5位和第7位固定为1
#define BIT_DEV_LBA 0x40    //寻址模式为LBA
#define BIT_DEV_DEV 0x10    //主盘或从盘

/* 一些硬盘操作的指令 */
#define CMD_IDENTIFY 0xec
#define CMD_READ_SECTOR 0x20
#define CMD_WRITE_SECTOR 0x30

/* 定义可读写的最大扇区数，调试用的 */
#define max_lba ((80*1024*1024/512) - 1)    // 只支持80M硬盘

uint8_t channel_cnt;            //按硬盘数计算的通道数
struct ide_channel channels[2]; //有两个IDE通道

/* 硬盘数据结构初始化 */
void ide_init() {
    printk("ide_init start\n");
    uint8_t hd_cnt = *((uint8_t*)(0x475));
    ASSERT(hd_cnt > 0);
    channel_cnt = DIV_ROUND_UP(hd_cnt, 2);

    struct ide_channel* channel;
    uint8_t channel_no = 0;

    /* 处理每个通道上的硬盘 */
    while (channel_no < channel_cnt) {
        channel = &channels[channel_no];
        sprintf(channel->name, "ide%d", channel_no);

        /* 为每个IDE通道初始化端口及中断向量 */
        switch (channel_no) {
            case 0:
                channel->port_base = 0x1f0;
                channel->irq_no = 0x20 + 14;
                break;
            case 1:
                channel->port_base = 0x170;
                channel->irq_no = 0x20 + 15;
                break;
        }

        channel->expecting_intr = false;
        lock_init(&channel->lock);
        sema_init(&channel->disk_done, 0);
        channel_no++;
    }
    printk("ide_init done\n");
}