#include "memory.h"
#include "bitmap.h"
#include "stdint.h"
#include "global.h"
#include "debug.h"
#include "print.h"
#include "string.h"

#define PG_SIZE 4096

/* 0xc009_f000是内核主线程的栈顶, 0xc009e000是内核主线程的pcb
位图的位置安排在0xc009a000, 距pcb起始地址4K, 
这样操作系统最大支持4个页框的位图, 即512GB */
#define MEM_BITMAP_BASE 0xc009a000      // 内存位图基址


/* 内核从虚拟地址3G起, 跨过低端1M内存, 使虚拟地址在逻辑上连续 */
#define K_HEAP_START 0xc0100000         // 内核所使用的堆空间起始地址
#define PDE_IDX(addr) ((addr & 0xffc00000) >> 22)
#define PTE_IDX(addr) ((addr & 0x003ff000) >> 12)

/* 内存池结构, 生成两个实例用于管理内核内存池和用户内存池, 这是两个物理内存池 */
struct pool {
    struct bitmap pool_bitmap;
    uint32_t phy_addr_start;
    uint32_t pool_size;
};

struct pool kernel_pool, user_pool;         // 内核和用户内存池
struct virtual_addr kernel_vaddr;           // 内核虚拟地址, 相比前面两个物理地址, 其没有定义pool_size, 也即内存池字节容量


/* 在pf表示的内存池中申请pg_cnt个虚拟页, 成功则返回虚拟页的地址, 失败在返回NULL */
static void* vaddr_get(enum pool_flags pf, uint32_t pg_cnt) {
    int vaddr_start = 0, bit_idx_start = -1;
    uint32_t cnt = 0;
    if (pf == PF_KERNEL) {
        bit_idx_start = bitmap_scan(&kernel_vaddr.vaddr_bitmap, pg_cnt);
        if (bit_idx_start == -1) {
            return NULL;
        }
        /* 改变已找到的内存的位图 */
        while (cnt < pg_cnt) {
            bitmap_set(&kernel_vaddr.vaddr_bitmap, bit_idx_start + cnt++, 1);
        }
        vaddr_start = kernel_vaddr.vaddr_start + bit_idx_start * PG_SIZE;
    } else {
        // 用户内存池, 将来实现用户进程再补充
    }
    return (void*) vaddr_start;
}

/* 得到虚拟地址vaddr对应的pte指针 */
uint32_t* pte_ptr(uint32_t vaddr) {
    // 页表基址 + 页目录号(表示第几个页目录, 也即第几个页表) * 4K(一个页表4K大) + 页表中的索引 * 4(一个页表项4B大小, 一个页表4K含1K个页表项)
    // 注意中间10位是页表项, 表示页表中的第几个元素, 我们现在在找页表地址
    uint32_t* pte = (uint32_t*) (0xffc00000 + ((vaddr & 0xffc00000) >> 10) + PTE_IDX(vaddr) * 4);
    return pte;
}

/* 得到虚拟地址vaddr对应的pde指针 */
uint32_t pde_ptr(uint32_t vaddr) {
    // 页目录项基地址 + 页目录序号*一个目录项4B
    uint32_t* pde = (uint32_t*) ( 0xfffff000 + PDE(vaddr) * 4 );
    return pde;
}

/* 在m_pool指向的物理内存池中分配1个物理页 *, 成功则返回页框的物理地址, 失败则返回NULL */
static void* palloc(struct pool* m_pool) {
    int bit_idx = bitmap_scan(&m_pool->pool_bitmap, 1);
    if (bit_idx == -1) {
        return NULL;
    }
    bitmap_set(&m_pool->pool_bitmap, bit_idx, 1);
    uint32_t page_phyaddr = ((bit_idx * PG_SIZE) + m_pool->phy_addr_start);
    return (void*) page_phyaddr;
}

/* 初始化内存池 */
static void mem_pool_init(uint32_t all_mem) {
    // 1. : 总体物理内存
    put_str("    mem_pool_init start\n");
    uint32_t page_table_size = PG_SIZE * 256;

    uint32_t used_mem = page_table_size + 0x100000;   // 表示物理内存地址从0开始, 用到了哪里
    uint32_t free_mem = all_mem - used_mem;
    uint16_t all_free_pages = free_mem / PG_SIZE;

    // 2. : 初始化两个物理内存池
    uint16_t kernel_free_pages = all_free_pages / 2;
    uint16_t user_free_pages = all_free_pages - kernel_free_pages;

    /* 物理内存池pool包括三个属性, 位图(包括位图字节长和指针), 物理起始地址, 池子大小 */
    uint32_t kbm_length = kernel_free_pages / 8;
    uint32_t ubm_length = user_free_pages / 8;

    uint32_t kp_start = used_mem;                   // 1M以后的地址, 放万页表就开始放内核内存池
    uint32_t up_start = kp_start + kernel_free_pages * PG_SIZE;

    /* 先设定后两个好设定的: 物理起始地址和池子大小 */
    kernel_pool.phy_addr_start = kp_start;
    user_pool.phy_addr_start = up_start;

    kernel_pool.pool_size = kernel_free_pages * PG_SIZE;
    user_pool.pool_size = user_free_pages * PG_SIZE;

    kernel_pool.pool_bitmap.btmp_bytes_len = kbm_length;
    user_pool.pool_bitmap.btmp_bytes_len = ubm_length;

    /* 内核内存池基址0xc009a000 */
    kernel_pool.pool_bitmap.bits = (void*) MEM_BITMAP_BASE;
    user_pool.pool_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length);

    // 3. : 输出内存池信息并初始化物理内存池
    put_str("   kernel_pool_bitmap_start: ");
    put_int((int)kernel_pool.pool_bitmap.bits);
    put_str("   kernel_pool_phy_addr_start: ");
    put_int(kernel_pool.phy_addr_start);
    put_str("\n");
    put_str("   user_pool_bitmap_start: ");
    put_int((int)user_pool.pool_bitmap.bits);
    put_str("   user_pool_phy_addr_start: ");
    put_int(user_pool.phy_addr_start);
    put_str("\n");

    /* 将两个物理位图置0 */
    bitmap_init(&kernel_pool.pool_bitmap);
    bitmap_init(&user_pool.pool_bitmap);

    /* 内核虚拟地址(内存池)的初始化, 包括位图(包括字节长和位置)和虚拟地址起始 */
    kernel_vaddr.vaddr_bitmap.btmp_bytes_len = kbm_length;
    // 用于维护内核堆的虚拟地址, 所以要和内核内存池大小一致
    
    /* 位图的数组指向一块未使用的内存, 目前定位在内核内存池和虚拟内存池之外 */
    kernel_vaddr.vaddr_bitmap.bits = (void*) (MEM_BITMAP_BASE + kbm_length + ubm_length);

    /* 虚拟内存池的起始地址为3G+1M, 也即 K_HEAP_START = 0xc001_0000 */
    kernel_vaddr.vaddr_start = K_HEAP_START;
    bitmap_init(&kernel_vaddr.vaddr_bitmap);
    put_str("  mem_pool_init done\n");
}

/* 内核管理部分初始化入口 */
void mem_init() {
    put_str("mem_init start\n");
    uint32_t mem_bytes_total = (*(uint32_t*)(0xb00));
    mem_pool_init(mem_bytes_total);
    put_str("mem_init done\n");
}

