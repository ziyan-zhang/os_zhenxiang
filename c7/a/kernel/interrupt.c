#include "interrupt.h"
#include "stdint.h"
#include "global.h"
#include "io.h"

#define IDT_DESC_CNT 0x21
#define PIC_M_CTRL 0x20
#define PIC_M_DATA 0x21
#define PIC_S_CTRL 0xa0
#define PIC_S_DATA 0xa1

/* 中断门描述符结构体 */
struct gate_desc {
    uint16_t func_offset_low_word;
    uint16_t selector;
    uint8_t dcount;

    uint8_t attribute;
    uint16_t func_offset_high_word;
};

// 静态函数声明,非必须
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function);
static struct gate_desc idt[IDT_DESC_CNT];      //idt是中段描述符表,本质是是一个中段描述符数组
char* intr_name[IDT_DESC_CNT];
intr_handler idt_table[IDT_DESC_CNT];
extern intr_handler intr_entry_table[IDT_DESC_CNT];

/* 初始化可编程中断控制器8259A */
static void pic_init(void) {

    /* 初始化主片 */
    outb(PIC_M_CTRL, 0x11);
    outb(PIC_M_DATA, 0x20);

    outb(PIC_M_DATA, 0x04);
    outb(PIC_M_DATA, 0x01);

    outb(PIC_S_CTRL, 0x11);
    outb(PIC_S_DATA, 0x28);

    outb(PIC_S_DATA, 0x02);
    outb(PIC_S_DATA, 0x01);
    // OCW1
    outb(PIC_M_DATA, 0xfe);
    outb(PIC_S_DATA, 0xff);
    // OCW2，或许需要
    // outb(PIC_M_CTRL, 0x00);
    // outb(PIC_S_CTRL, 0x00);

    put_str("  pic init done\n");
}

/* 创建中断门描述符 */
static void make_idt_desc(struct gate_desc* p_gdesc, uint8_t attr, intr_handler function) {
    p_gdesc->func_offset_low_word = (uint32_t)function & 0x0000FFFF;
    p_gdesc->selector = SELECTOR_K_CODE;
    p_gdesc->dcount = 0;
    p_gdesc->attribute = attr;
    p_gdesc->func_offset_high_word = ( (uint32_t)function & 0xFFFF0000 ) >> 16;
}

/* 初始化中段描述符表 */
static void idt_desc_init(void) {
    int i;
    for (i=0; i<IDT_DESC_CNT; i++) {
        make_idt_desc(&idt[i], IDT_DESC_ATTR_DPL0, intr_entry_table[i]);
    }
    put_str("  idt_desc_init done\n");
}
/* 通用的中断处理函数，一般用在异常出现时的处理 */
static void general_intr_handler(uint8_t vec_nr) {
    if (vec_nr == 0x27 || vec_nr == 0x2f) {
        return;
    }
    put_str("int vector : 0x");
    put_int(vec_nr);
    put_char('\n');
}

/* 完成一般中断处理函数注册及异常名注册 */
static void exception_init(void) {
    int i;
    for (i=0; i<IDT_DESC_CNT; i++) {
        idt_table[i] = general_intr_handler;
        intr_name[i] = "unknown";
    }
    intr_name[0] = "write latter here. ";
    intr_name[1] = "write latter here. ";
    intr_name[2] = "write latter here. ";
    intr_name[3] = "write latter here. ";
    intr_name[4] = "write latter here. ";
    intr_name[5] = "write latter here. ";
    intr_name[6] = "write latter here. ";
    intr_name[7] = "write latter here. ";
    intr_name[8] = "write latter here. ";
    intr_name[9] = "write latter here. ";
    intr_name[10] = "write latter here. ";
    intr_name[11] = "write latter here. ";
    intr_name[12] = "write latter here. ";
    intr_name[13] = "write latter here. ";
    intr_name[14] = "write latter here. ";
    intr_name[15] = "write latter here. ";
    intr_name[16] = "write latter here. ";
    intr_name[17] = "write latter here. ";
    intr_name[18] = "write latter here. ";
    intr_name[19] = "write latter here. ";
    intr_name[20] = "write latter here. ";

}



/* 完成有关中断的所有初始化工作 */
void idt_init() {
    put_str("idt_init start\n");
    idt_desc_init();
    exception_init();
    pic_init();

    /* 加载idt */
    // uint64_t idt_operand = ( (sizeof(idt)-1) | ( (uint64_t)(uint32_t)idt  << 16) );
    uint64_t idt_operand = ( (sizeof(idt)-1) |     ((uint64_t)((uint32_t)idt << 16)));

    asm volatile("lidt %0": : "m"(idt_operand));
    put_str("idt_init done\n");
}