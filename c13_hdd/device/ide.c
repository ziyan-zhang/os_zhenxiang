#include "ide.h"
#include "io.h"

/* 定义硬盘各驱动器的端口号 */
#define reg_data(channel)       (channel->port_base + 0)
#define reg_error(channel)      (channel->port_base + 1)
#define reg_sec_count(channel)  (cahnnel->port_base + 2)
#define reg_lba_l(channel)      (channel->port_base + 3)
#define reg_lba_m(channel)      (channel->port_base + 4)
#define reg_lba_h(channel)      (channel->port_base + 5)
#define reg_dev(channel)        (channel->port_base + 6)
#define reg_status(channel)     (channel->port_base + 7)
#define reg_cmd(channel)        (reg_status(channel))
#define reg_alt_status(channel) (channel->port_base + 0x206)
#define reg_ctl(channel)        reg_alt_status(channel)

/* reg_alt_status 寄存器的一些关键位 */
#define BIT_STAT_BSY    0x80    //硬盘忙
#define BIT_STAT_DRDY   0x40    //驱动器准备好
#define BIT_STAT_DRQ         0x8     //数据传输准备好了

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

// disk结构体包含：名称name，通道ide channel，编号dev_no(主0从1)，主、逻辑分区partition prim, logic
void select_disk(struct disk* hd) {
    uint8_t reg_device = BIT_DEV_MBS | BIT_DEV_LBA;
    if (hd->dev_no == 1) {
        reg_device |= BIT_DEV_DEV;
    }
    outb(reg_dev(hd->my_channel), reg_device);
}

/* 向硬盘控制器写入起始扇区地址及要读写的扇区数 */
static void select_sector(struct disk* hd, uint32_t lba, uint8_t sec_cnt) {
    ASSERT(lba < max_lba);
    struct ide_channel* channel = hd->my_channel;

    /* 写入要读取的扇区数 */
    outb(reg_sect_cnt(channel), sec_cnt);

    /* 写入LBA地址，即扇区号 */
    outb(reg_lba_l(channel), lba);
    outb(reg_lba_m(channel), lba>>8);
    outb(reg_lba_h(channel), lba>>16);

    /* 写lba24-17位，因与device寄存器在同一个四位里，故把device寄存器的reg_device位再写一次 */
    outb(reg_dev(channel), BIT_DEV_MBS | BIT_DEV_LBA | (hd->dev_no == 1 ? BIT_DEV_DEV :0) | lba >> 24);
}

/* 项通道channel发命令cmd */
static void cmd_out(struct ide_channel* channel, uint8_t cmd) {
    /* 只要向硬盘发出了命令便将通道的expecting_intr置为true，硬盘中断处理程序需要根据它来判断 */
    channel->expecting_intr = true;
    outb(reg_cmd(channel), cmd);
}

/* 把数据从硬盘的缓冲区取出：256扇区一组 */
static void read_from_sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
    uint32_t size_in_byte;
	if (sec_cnt == 0) {
		/* sec_cnt是8位变量，由主调函数将其赋值时，若为256则会将最高位1丢掉变0 */
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	insw(reg_data(hd->my_channel), buf, size_in_byte/2);
}

/* 将buf中sec_cnt个扇区的数据写入硬盘 */
static void write2sector(struct disk* hd, void* buf, uint8_t sec_cnt) {
	uint32_t size_in_byte;
	if (sec_cnt == 0) {
		size_in_byte = 256 * 512;
	} else {
		size_in_byte = sec_cnt * 512;
	}
	outsw(reg_data(hd->my_channel), buf, size_in_byte/2);
}

/* 等待30秒 */
static bool busy_wait(struct disk* hd) {
	struct ide_channel* channel = hd->my_channel;
	uint16_t time_limit = 30*1000;
	while (time_limit -= 10 >= 0) {
		if (!(inb(reg_status(channel)) & BIT_STAT_BSY)) {
			return (inb(reg_status(channel)) & BIT_STAT_DRQ);
		} else {
			mtime_sleep(10);
		}
	}
	return false;
}

/* 从硬盘读取sec_cnt个扇区到buf: lbs上的封装 */
void ide_read(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba);
	ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);

	/* 1. 先选择操作的硬盘 */
	select_disk(hd);	//往寄存器写点东西指定硬盘

	uint32_t secs_op;	//每次操作的扇区数
	uint32_t secs_done;	//已完成的扇区数
	while ((secs_done+256) < sec_cnt) {
		if ((secs_done+256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}

		/* 2. 写入待读取的扇区数和起始扇区号 */
		select_sector(hd, lba+secs_done, secs_op);

		/* 3. 执行的命令写入reg_cmd寄存器 */
		cmd_out(hd->my_channel, CMD_READ_SECTOR);
		/* 现在硬盘已经开始忙了，阻塞自己 */
		sema_down(&hd->my_channel->disk_done);

		/* 4. 检测硬盘状态是否可读 */
		/* 醒来后开始执行下面代码 */
		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s read sector %d failed!!!\n", hd->name, lba);
			PANIC(error);
		}

		/* 5. 把数据从硬盘的缓冲区读出 */
		read_from_sector(hd, (void*)((uint32_t)buf + secs_done*512), secs_op);
		secs_done += secs_op;
	}
	lock_release(&hd->my_channel->lock);
}

/* 将buf中sec_cnt扇区数据写入硬盘 */
void ide_write(struct disk* hd, uint32_t lba, void* buf, uint32_t sec_cnt) {
	ASSERT(lba <= max_lba);
	ASSERT(sec_cnt > 0);
	lock_acquire(&hd->my_channel->lock);

	/* 1. 先选择要操作的硬盘 */
	select_disk(hd);

	uint32_t secs_op;
	uint32_t secs_done = 0;
	while (secs_done < sec_cnt) {
		if ((secs_done + 256) <= sec_cnt) {
			secs_op = 256;
		} else {
			secs_op = sec_cnt - secs_done;
		}

		/* 2. 写入待写入的扇区数和起始扇区号 */
		select_sector(hd, lba + secs_done, secs_op);

		/* 3. 执行的命令写入到reg_cmd寄存器 */
		cmd_out(&hd->my_channel, CMD_WRITE_SECTOR);

		/* 4. 检测硬盘状态是否可读 */
		if (!busy_wait(hd)) {
			char error[64];
			sprintf(error, "%s write sector %d failed!!!\n", hd->name, lba);
			PANIC(error);
		}

		/* 5. 将数据写入硬盘 */
		write2sector(hd, (void*)((uint32_t)buf + secs_done * 512), secs_op);

		/* 在硬盘响应期间阻塞自己 */
		sema_down(&hd->my_channel->disk_done);
		secs_done += secs_op;
	}
	/* 醒来后释放锁 */
	lock_release(&hd->my_channel->lock);
}

/* 硬盘中断处理程序 */
void intr_hd_handler(uint8_t irq_no) {
	ASSERT(irq_no == 0x2e || irq_no == 0x2f);
	uint8_t ch_no = irq_no - 0x2e;
	struct ide_channel* channel = &channels[ch_no];
	ASSERT(channel->irq_no == irq_no);
	if (channel->expecting_intr) {
		channel->expecting_intr = false;
		sema_up(&channel->disk_done);
		/* 读取状态寄存器使硬盘认为此次的中断已被处理，从而硬盘可以继续执行新的读写 */
		inb(reg_status(channel));
	}
}


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
		register_handler(channel->irq_no, intr_hd_handler);
        channel_no++;
    }
    printk("ide_init done\n");
}