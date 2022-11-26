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

