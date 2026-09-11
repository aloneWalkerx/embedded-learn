#include "./MALLOC/malloc.h"

/* ====================================================================
   编译环境判断：AC5（ARM Compiler 5）vs AC6（ARM Compiler 6）
   ★ 不同编译器对对齐和地址定位的语法不同
   ==================================================================== */

#if !(__ARMCC_VERSION >= 6010050)   /* 不是AC6编译器，即使用AC5编译器时 */

/* ====================================================================
   AC5 编译器：内存池定义（64字节对齐）
   ==================================================================== */

// ★ ① 内部 SRAM 内存池（mem1_base_addr）
// 大小：100KB，地址由编译器自动分配
// __align(64) 保证 64 字节对齐（提高访问效率）
static __align(64) uint8_t mem1_base_addr[SRAM_IN_MEM1_MAX_SIZE];

// ★ ② CCM 内存池（mem2_base_addr）
// 大小：60KB，地址固定在 0x10000000（CCM 起始地址）
// __attribute__((at(0x10000000))) 指定绝对地址
// ★ 注意：CCM 内存只能由 CPU 访问，DMA 无法访问！
static __align(64) uint8_t mem2_base_addr[SRAM_IN_CCM_MEM2_MAX_SIZE] __attribute__((at(0x10000000)));

// ★ ③ 外部 SRAM mem3_base_addr
// 大小：963KB，地址固定在 0x68000000（外部 SRAM 起始地址）
static __align(64) uint8_t mem3_base_addr[SRAM_EX_MEM3_MAX_SIZE] __attribute__((at(0x68000000)));
/* ====================================================================
   AC5 编译器：内存管理表定义
   ==================================================================== */

// ★ ④ 内部 SRAM 内存管理表（mem1_table_status）
// 每个表项对应一个内存块，记录该块的空闲/占用状态 
static SRAM_TABLE_DATA_TYPE mem1_table_block_value[SRAM_IN_MEM1_ALLOC_TABLE_SIZE];

// ★ ⑤ CCM 内存管理表 
// 位于 CCM 内存池后面（0x10000000 + SRAM_IN_CCM_MEM2_MAX_SIZE）
static SRAM_TABLE_DATA_TYPE mem2_table_block_value[SRAM_IN_CCM_MEM2_ALLOC_TABLE_SIZE] __attribute__((at(0x10000000 + SRAM_IN_CCM_MEM2_MAX_SIZE)));

// ★ ⑥ 外部 SRAM 内存管理表
// 位于外部 SRAM 内存池后面（0x68000000 + SRAM_EX_MEM3_MAX_SIZE）
static SRAM_TABLE_DATA_TYPE mem3_table_block_value[SRAM_EX_MEM3_ALLOC_TABLE_SIZE] __attribute__((at(0x68000000 + SRAM_EX_MEM3_MAX_SIZE)));

#else      /* 使用AC6编译器时 */

/* ====================================================================
   AC6 编译器：内存池定义（64字节对齐）
   ★ AC6 的语法不同：使用 __ALIGNED(64) 和 section 属性
   ==================================================================== */

// ★ ⑦ 内部 SRAM 内存池
static __ALIGNED(64) uint8_t mem1_base_addr[SRAM_IN_MEM1_MAX_SIZE];

// ★ ⑧ CCM 内存池（通过 section 指定地址）
static __ALIGNED(64) uint8_t mem2_base_addr[SRAM_IN_CCM_MEM2_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x10000000")));

// ★ ⑨ 外部 SRAM 内存池
static __ALIGNED(64) uint8_t mem3_base_addr[SRAM_EX_MEM3_MAX_SIZE] __attribute__((section(".bss.ARM.__at_0x68000000")));

/* ====================================================================
   AC6 编译器：内存管理表定义
   ==================================================================== */

// ★ ⑩ 内部 SRAM 内存管理表
static SRAM_TABLE_DATA_TYPE mem1_table_block_value[SRAM_IN_MEM1_ALLOC_TABLE_SIZE];

// ★ ? CCM 内存管理表（位于 CCM 内存池后面）
static SRAM_TABLE_DATA_TYPE mem2_table_block_value[SRAM_IN_CCM_MEM2_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x1000F000")));

// ★ ? 外部 SRAM 内存管理表
static SRAM_TABLE_DATA_TYPE mem3_table_block_value[SRAM_EX_MEM3_ALLOC_TABLE_SIZE] __attribute__((section(".bss.ARM.__at_0x680F0C00")));

#endif    // 结束 AC5/AC6 条件编译

/* ====================================================================
   内存管理参数（只读数组）
   ==================================================================== */

// ★ ? 每个内存池的管理表大小（表项数） 
const uint32_t mem_table_size[SRAM_BANKS] = {
    // 内部 SRAM：3200 项
    SRAM_IN_MEM1_ALLOC_TABLE_SIZE,
    // CCM：1920 项
    SRAM_IN_CCM_MEM2_ALLOC_TABLE_SIZE,
    // 外部 SRAM：30816 项
    SRAM_EX_MEM3_ALLOC_TABLE_SIZE
};

// ★ ? 每个内存池的块大小（字节） 
const uint32_t mem_block_size[SRAM_BANKS] = {
    // 32 字节
    SRAM_IN_MEM1_BLOCK_SIZE,
    // 32 字节
    SRAM_IN_CCM_MEM2_BLOCK_SIZE,
    // 32 字节
    SRAM_EX_MEM3_BLOCK_SIZE
};

// ★ ? 每个内存池的总大小（字节） 
const uint32_t mem_total_size[SRAM_BANKS] = {
    // 100KB
    SRAM_IN_MEM1_MAX_SIZE,
    // 60KB
    SRAM_IN_CCM_MEM2_MAX_SIZE,
    // 963KB
    SRAM_EX_MEM3_MAX_SIZE
};

/* ====================================================================
   内存管理控制器实例
   ★ 将以上所有资源整合到一个结构体中
   ==================================================================== */
struct _m_mallco_dev mallco_dev =
{
    my_mem_init,                                // 初始化函数指针
    my_mem_perused,                             // 内存使用率函数指针
    mem1_base_addr, mem2_base_addr, mem3_base_addr,               // ★ 三个内存池的基地址
    mem1_table_block_value, mem2_table_block_value, mem3_table_block_value,      // ★ 三个内存管理表
    0, 0, 0,                                    // ★ 三个内存池的就绪标志（初始为 0，未就绪）
};

/* ====================================================================
   ① 内存复制函数（类似 memcpy）
   ==================================================================== */
/**
 * @brief       复制内存
 * @param       *des : 目的地址
 * @param       *src : 源地址
 * @param       n    : 需要复制的内存长度(字节为单位)
 * @retval      无
 */
void my_mem_copy(void *des, void *src, uint32_t n)
{
    // 目的地址指针（按字节操作）
    uint8_t *temp_des = des;
    // 源地址指针（按字节操作）
    uint8_t *temp_src = src;

    // ★ 逐字节复制
    while (n--)
        // 先复制，再指针后移
        *temp_des++ = *temp_src++;
}

/* ====================================================================
   ② 内存设置函数（类似 memset）
   ==================================================================== */
/**
 * @brief       设置内存值
 * @param       *s    : 内存首地址
 * @param       c     : 要设置的值
 * @param       count : 需要设置的内存大小(字节为单位)
 * @retval      无
 */
void my_mem_set(void *s, uint8_t c, uint32_t count)
{
    uint8_t *xs = s;   // 转换为字节指针

    // ★ 逐字节填充
    while (count--)
        *xs++ = c;
}

/* ====================================================================
   ③ 内存管理初始化
   ==================================================================== */
/**
 * @brief       内存管理初始化
 * @param       memx : 所属内存块（SRAM_IN / SRAM_IN_CCM / SRAM_EX）
 * @retval      无
 * @note        将内存管理表全部清零（所有块标记为空闲）
 */
void my_mem_init(uint8_t memx)
{
    // ★ 获取内存管理表每个表项的大小（uint16_t 或 uint32_t）
    uint8_t mttsize = sizeof(SRAM_TABLE_DATA_TYPE);

    // ★ 将内存管理表全部清零
    // 0 表示该块为空闲（未被分配）
    my_mem_set(mallco_dev.arr_mem_table_block_value[memx], 0, mem_table_size[memx] * mttsize);

    // ★ 标记该内存池已就绪（可以分配内存了）
    mallco_dev.arr_mem_ready_status[memx] = 1;
}

/* ====================================================================
   ④ 获取内存使用率
   ==================================================================== */
/**
 * @brief       获取内存使用率
 * @param       memx : 所属内存块
 * @retval      使用率(扩大了10倍,0~1000,代表0.0%~100.0%)
 * @note        返回值除以 10 即为实际百分比
 *              例如：250 → 25.0%
 */
uint16_t my_mem_perused(uint8_t memx)
{
    // 已使用的块数
    uint32_t used = 0;   
    uint32_t i;

    // ★ 遍历所有块，统计已使用的块数
    for (i = 0; i < mem_table_size[memx]; i++)
    {
        //非 0 表示已分配
        if (mallco_dev.arr_mem_table_block_value[memx][i])
        {
            used++;
        }
    }

    // ★ 返回使用率（乘以 1000 保留一位小数），(已使用块数*1000)/总块数
    return (used * 1000) / (mem_table_size[memx]);
}

/* ====================================================================
   ⑤ 内存分配（内部函数）
   ==================================================================== */
/**
 * @brief       内存分配(内部调用)
 * @param       memx : 所属内存块
 * @param       size : 要分配的内存大小(字节)
 * @retval      内存偏移地址
 *   @arg       0 ~ 0xFFFFFFFE : 有效的内存偏移地址
 *   @arg       0xFFFFFFFF     : 无效（分配失败）
 * @note        核心算法：查找连续的空闲块
 */
static uint32_t my_mem_malloc(uint8_t memx, uint32_t size)
{
    // 当前扫描的位置（从后往前）
    signed long offset = 0;
    // 需要的内存块数
    uint32_t need_mem_blocks;
    //// 当前找到的连续空闲块数
    uint32_t con_idle_Blocks = 0; 
    uint32_t i;

    //--- 步骤 1：确保内存池已初始化
    if (!mallco_dev.arr_mem_ready_status[memx])
    {
        //// 未初始化，先执行初始化
        mallco_dev.init(memx);   
    }

    //步骤 2：参数检查
    // 不需要分配
    if (size == 0) return 0xFFFFFFFF;   

    // 步骤 3：计算需要的块数
    // 需要分配的块数 = size / 块大小（向上取整）
    need_mem_blocks = size / mem_block_size[memx];
    // 有余数则多占一块
    if (size % mem_block_size[memx]) need_mem_blocks++;

    /* --- 步骤 4：搜索连续的空闲块 --- */
    // ★ 从后往前搜索（提高内存利用率）
    for (offset = mem_table_size[memx] - 1; offset >= 0; offset--)
    {
        if (!mallco_dev.arr_mem_table_block_value[memx][offset])   // 当前块空闲
        {
            // 连续空闲块数 +1
            con_idle_Blocks++;
        }
        // 当前块已被占用
        else
        {
            // 连续空闲块计数清零,重新累计连续内存块
            con_idle_Blocks = 0;
        }

        // ★ 找到了足够多的连续空闲块，连续内存块等于需要的内存块
        if (con_idle_Blocks == need_mem_blocks)
        {
            // ★ 标记这些块为已分配
            // 表项值记录的是连续块的数量（用于释放时知道要释放多少块）
            for (i = 0; i < need_mem_blocks; i++)
            {
                //将当前内存块的值设置为需要的内存块的数值
                mallco_dev.arr_mem_table_block_value[memx][offset + i] = need_mem_blocks;
            }

            // ★ 返回偏移地址（块首地址 = 块号 × 块大小）
            return (offset * mem_block_size[memx]);
        }
    }

    // ★ 未找到足够的连续空闲块（内存不足）
    return 0xFFFFFFFF;
}

/* ====================================================================
   ⑥ 内存释放（内部函数）
   ==================================================================== */
/**
 * @brief       释放内存(内部调用)
 * @param       memx   : 所属内存块
 * @param       offset : 内存地址偏移
 * @retval      释放结果
 *   @arg       0, 释放成功;
 *   @arg       1, 释放失败（未初始化）;
 *   @arg       2, 超区域了(失败);
 */
static uint8_t my_mem_free(uint8_t memx, uint32_t offset)
{
    int i;

    //步骤 1：如果从未初始化过，初始化后直接返回1
    if (!mallco_dev.arr_mem_ready_status[memx])
    {
        // 初始化
        mallco_dev.init(memx);
        return 1;   
    }

    //步骤 2：检查偏移是否在有效范围内,即要释放的内存是否在其规定的内存范围内
    // 偏移在内存池内
    if (offset < mem_total_size[memx])
    {
        // ★ 计算起始块号
        int current_block_number = offset / mem_block_size[memx];

        // ★ 获取该块的大小（记录在管理表中）
        int nmemb = mallco_dev.arr_mem_table_block_value[memx][current_block_number];

        // ★ 将这些块标记为空闲（清零）
        for (i = 0; i < nmemb; i++)
        {
            //将当前内存表的内存块的值设置为0，空闲状态
            mallco_dev.arr_mem_table_block_value[memx][current_block_number + i] = 0;
        }
        // 释放成功
        return 0;
    }
    else
    {
        // 偏移超区了（非法地址）
        return 2;
    }
}

/* ====================================================================
   ⑦ 内存释放（外部函数，类似 free）
   ==================================================================== */
/**
 * @brief       释放内存(外部调用)
 * @param       memx : 所属内存块
 * @param       ptr  : 内存首地址
 * @retval      无
 * @note        ① 必须传入由 mymalloc 返回的指针
 *              ② 释放后该内存可被再次分配
 */
void myfree(uint8_t memx, void *ptr)
{
    //偏移地址
    uint32_t offset;
    // ★ 释放空指针无操作（安全做法）
    if (ptr == NULL) return;

    // ★ 计算偏移地址 = 指针地址 - 内存池基地址
    offset = (uint32_t)ptr - (uint32_t)mallco_dev.arr_mem_base_addr[memx];

    // ★ 调用内部释放函数
    my_mem_free(memx, offset);
}

/* ====================================================================
   ⑧ 内存分配（外部函数，类似 malloc）
   ==================================================================== */
/**
 * @brief       分配内存(外部调用)
 * @param       memx : 所属内存块
 * @param       size : 要分配的内存大小(字节)
 * @retval      分配到的内存首地址（失败返回 NULL）
 * @note        ① 从指定的内存池中分配内存
 *              ② 分配大小会自动对齐到块大小的整数倍
 *              ③ 如果内存不足，返回 NULL
 */
void *mymalloc(uint8_t memx, uint32_t size)
{
    uint32_t offset;

    // ★ 调用内部分配函数获取偏移地址
    offset = my_mem_malloc(memx, size);
    // 分配失败
    if (offset == 0xFFFFFFFF)
    {
        // 返回空指针
        return NULL;
    }
    // 分配成功
    else
    {
        // ★ 返回实际内存地址 = 基地址 + 偏移
        return (void *)((uint32_t)mallco_dev.arr_mem_base_addr[memx] + offset);
    }
}

/* ====================================================================
   ⑨ 重新分配内存（外部函数，类似 realloc）
   ==================================================================== */
/**
 * @brief       重新分配内存(外部调用)
 * @param       memx : 所属内存块
 * @param       *ptr : 旧内存首地址
 * @param       size : 要分配的内存大小(字节)
 * @retval      新分配到的内存首地址
 * @note        ① 如果 ptr 为 NULL，等价于 mymalloc
 *              ② 如果 size 为 0，等价于 myfree
 *              ③ 先分配新内存，拷贝原数据，再释放旧内存
 */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size)
{
    uint32_t offset;

    // ★ 调用内部分配函数获取新内存
    offset = my_mem_malloc(memx, size);
    
    // 分配失败
    if (offset == 0xFFFFFFFF)
    {
        return NULL;
    }
    
    // 分配成功
    else   
    {
        // ★ 拷贝旧内存的内容到新内存
        my_mem_copy(
            // 新地址
            (void *)((uint32_t)mallco_dev.arr_mem_base_addr[memx] + offset),
            // 旧地址
            ptr,
            // 大小
            size
        );

        // ★ 释放旧内存
        myfree(memx, ptr);

        // ★ 返回新内存地址
        return (void *)((uint32_t)mallco_dev.arr_mem_base_addr[memx] + offset);
    }
}

