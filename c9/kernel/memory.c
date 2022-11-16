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
这样操作系统最大支持4个页框的位图, 即512M */
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
uint32_t* pde_ptr(uint32_t vaddr) {
    // 页目录项基地址 + 页目录序号*一个目录项4B
    uint32_t* pde = (uint32_t*) ( (0xfffff000) + PDE_IDX(vaddr) * 4 );
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



/* 页表中添加虚拟地址_vaddr与物理地址_page_phyaddr的映射 */
static void page_table_add(void* _vaddr, void* _page_phyaddr) {
    uint32_t vaddr = (uint32_t)_vaddr, page_phyaddr = (uint32_t)_page_phyaddr;
    uint32_t* pde = pde_ptr(vaddr);
    uint32_t* pte = pte_ptr(vaddr);

    /*************************    注意  ************************/
    /* 执行 *pte, 会访问到空的pde. 所以确保pde创建完成后才能执行 *pte, 否则会引发 page fault */
    /* 对于某个vaddr, 如果它所在的 pde 已被创建过(末位P为1), 就不用再创建 pde, 否则, 创建 */
    if (*pde & 0x00000001) {
        ASSERT (!(*pte & 0x00000001));
        if (!(*pte & 0x00000001)) {                                     //  pte_phyaddr = page_phyaddr
            *pte = ( page_phyaddr | PG_US_U | PG_RW_W | PG_P_1 );       //  页表项存的是页的物理地址(指向某页)和属性
        } else {
            PANIC("pte repeat!");
            *pte = ( page_phyaddr | PG_US_U | PG_RW_W | PG_P_1 );
        }
    } else {  // 页表项不存在, 要装载页表项. 页表项对应的页表物理地址要放在内核空间, 很合理. 
        uint32_t pde_phyaddr = (uint32_t) palloc(&kernel_pool);         //  pde_phyaddr = pt_phyaddr, 起始叫后者更好, 跟前面if对应
        *pde = ( pde_phyaddr | PG_US_U | PG_RW_W | PG_P_1 );            //  页目录项存的是页表的物理地址和属性
        memset((void*) ((int)pte & 0xfffff000), 0, PG_SIZE);            //  页表项地址的4K前对齐是页表地址, 置零

        ASSERT(!(*pte & 0x00000001));
        *pte = (page_phyaddr | PG_US_U | PG_RW_W | PG_P_1);
    }
}

/* 分配pg_cnt个页空间, 成功则返回起始虚拟地址, 失败时返回NULL */
void* malloc_page(enum pool_flags pf, uint32_t pg_cnt) {
    ASSERT(pg_cnt > 0 && pg_cnt < 3840);
    /*
    malloc_page的原理是三个动作的合成
    1. 通过vaddr_get在虚拟内存池中申请虚拟地址
    2. 通过palloc在物理内存池中申请物理地址
    3. 通过page_table_add将以上得到的虚拟地址和物理地址在页表中完成映射
    */
   void* vaddr_start = vaddr_get(pf, pg_cnt);
   if (vaddr_start == NULL) {
    return NULL;
   }

   uint32_t vaddr = (uint32_t)vaddr_start, cnt = pg_cnt;        // vaddr从1M开始分配
   struct pool* mem_pool = pf & PF_KERNEL ? &kernel_pool : &user_pool;

   /* 因为虚拟地址是连续的,但物理地址可以是不连续的,所以逐个做映射*/
   while (cnt-- > 0) {
    void* page_phyaddr = palloc(mem_pool);                      // phyaddr从2M开始分配
    if (page_phyaddr == NULL) {
        return NULL;
    }
    page_table_add((void*)vaddr, page_phyaddr);
    vaddr += PG_SIZE;
   }
   return vaddr_start;
}

/* 从内核物理内存池中申请pg_cnt页内存, 成功则返回其虚拟地址, 失败则返回NULL */
void* get_kernel_pages(uint32_t pg_cnt) {
    void* vaddr = malloc_page(PF_KERNEL, pg_cnt);
    if (vaddr != NULL) {
        memset(vaddr, 0, pg_cnt * PG_SIZE);
    }
    return vaddr;
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

