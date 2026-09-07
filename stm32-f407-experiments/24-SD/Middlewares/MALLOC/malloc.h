#ifndef __MALLOC_H
#define __MALLOC_H

#include "./SYSTEM/sys/sys.h"   // 包含系统基础头文件（类型定义、位操作等）

/* ====================================================================
   内存池编号定义
   ★ 定义了三个独立的内存池，每个内存池独立管理，互不干扰
   ==================================================================== */

// ★ ① 内部 SRAM 内存池（SRAMIN）
// 位于芯片内部，CPU 和 DMA 都可以访问
// 是最常用的内存池
#define SRAMIN                  0

// ★ ② CCM 内存池（SRAMCCM）
// 位于芯片内部，但只有 CPU 可以访问（DMA 无法访问！）
// 速度比普通 SRAM 快，适合存放 CPU 频繁访问的数据
// ★ 注意：CCM 内存不能被 DMA 使用！
#define SRAMCCM                 1

// ★ ③ 外部 SRAM 内存池（SRAMEX）
// 位于芯片外部（通过 FSMC 外扩的 SRAM）
// 容量大，但访问速度比内部 SRAM 慢
#define SRAMEX                  2

// ★ ④ 支持的 SRAM 块数（3 个内存池）
#define SRAMBANK                3

/* ====================================================================
   内存管理表类型定义
   ==================================================================== */

// ★ ⑤ 内存管理表的数据类型
// 使用 uint16_t 可以节省内存（2 字节/项）
// 如果外扩 SDRAM（容量很大），需要改为 uint32_t
// 本实验用 uint16_t 即可（管理表每项只存 0~65535 的块编号）
#define MT_TYPE     uint16_t

/* ====================================================================
   内存池参数计算说明
   ★ 单块内存管理所占用的全部空间大小计算公式：
   size = 内存池大小 + (内存池大小 / 内存块大小) × sizeof(MT_TYPE)
   
   例如 SRAMEX：size = 963KB + (963KB / 32B) × 2B ≈ 1023KB
   
   ★ 已知总容量 size，最大内存池的计算公式：
   MEM_MAX_SIZE = (MEM_BLOCK_SIZE × size) / (MEM_BLOCK_SIZE + sizeof(MT_TYPE))
   例如 CCM：MEM2_MAX_SIZE = (32 × 64KB) / (32 + 2) ≈ 60KB
   ==================================================================== */

/* ====================================================================
   mem1：内部 SRAM 内存池参数
   ==================================================================== */

// ★ ⑥ 内存块大小：32 字节
// 分配内存时，按 32 字节的整数倍分配
// 例如：申请 10 字节，实际分配 32 字节（内存块是最小分配单位）
// 块大小越小，内存利用率越高，但管理表越大
// 块大小越大，管理表越小，但内存利用率越低
#define MEM1_BLOCK_SIZE         32

// ★ ⑦ 最大管理内存：100KB
// 从内部 SRAM 中划出 100KB 用于动态内存管理
// 剩余部分留给其他用途（如堆栈、全局变量等）
#define MEM1_MAX_SIZE           100*1024

// ★ ⑧ 内存管理表大小
// 表项数 = 总内存 / 块大小 = 100KB / 32B = 3200 项
// 每项用 uint16_t 存储，占用 3200 × 2 = 6400 字节
#define MEM1_ALLOC_TABLE_SIZE   MEM1_MAX_SIZE/MEM1_BLOCK_SIZE

/* ====================================================================
   mem2：CCM 内存池参数
   ==================================================================== */

// ★ ⑨ CCM 内存块大小：32 字节
#define MEM2_BLOCK_SIZE         32

// ★ ⑩ CCM 最大管理内存：60KB
// CCM 总容量约 64KB，预留 4KB 给其他用途
#define MEM2_MAX_SIZE           60 *1024

// ★ ? CCM 内存管理表大小
#define MEM2_ALLOC_TABLE_SIZE   MEM2_MAX_SIZE/MEM2_BLOCK_SIZE

/* ====================================================================
   mem3：外部 SRAM 内存池参数
   ==================================================================== */

// ★ ? 外部 SRAM 内存块大小：32 字节
#define MEM3_BLOCK_SIZE         32

// ★ ? 外部 SRAM 最大管理内存：963KB
// 外部 SRAM 总容量约 1MB，预留一部分给其他用途
#define MEM3_MAX_SIZE           963 *1024

// ★ ? 外部 SRAM 内存管理表大小
#define MEM3_ALLOC_TABLE_SIZE   MEM3_MAX_SIZE/MEM3_BLOCK_SIZE

/* ====================================================================
   NULL 定义
   ==================================================================== */

// ★ ? 如果未定义 NULL，则定义为 0
// 标准 C 库的 NULL 通常在 stddef.h 中定义
// 此处作为后备定义，确保代码兼容
#ifndef NULL
#define NULL 0
#endif

/* ====================================================================
   内存管理控制器结构体
   ★ 管理三个独立的内存池
   ==================================================================== */
struct _m_mallco_dev
{
    void (*init)(uint8_t);              // ★ 初始化函数指针（指向 my_mem_init）
    uint16_t (*perused)(uint8_t);       // ★ 内存使用率函数指针（指向 my_mem_perused）
    uint8_t *membase[SRAMBANK];         // ★ 内存池基地址数组（指向三个内存池的起始地址）
    MT_TYPE *memmap[SRAMBANK];          // ★ 内存管理状态表数组（记录每个块的空闲/占用状态）
    uint8_t  memrdy[SRAMBANK];          // ★ 内存池就绪标志（1=已初始化，0=未初始化）
};

// ★ 外部声明全局内存管理控制器
// 在 malloc.c 中定义，其他文件通过 extern 引用
extern struct _m_mallco_dev mallco_dev;

/* ====================================================================
   用户调用函数声明
   ==================================================================== */

/**
 * @brief   内存管理初始化函数
 * @param   memx: 内存池编号（SRAMIN / SRAMCCM / SRAMEX）
 * @retval  无
 * @note    初始化指定内存池的管理表，将所有块标记为空闲
 */
void my_mem_init(uint8_t memx);

/**
 * @brief   获取内存使用率
 * @param   memx: 内存池编号
 * @retval  使用率（0~100）
 * @note    计算已分配块数 / 总块数 × 100
 */
uint16_t my_mem_perused(uint8_t memx);

/**
 * @brief   内存设置函数（类似 memset）
 * @param   s:     目标地址
 * @param   c:     要填充的值
 * @param   count: 填充的字节数
 * @retval  无
 */
void my_mem_set(void *s, uint8_t c, uint32_t count);

/**
 * @brief   内存拷贝函数（类似 memcpy）
 * @param   des: 目标地址
 * @param   src: 源地址
 * @param   n:   拷贝的字节数
 * @retval  无
 */
void my_mem_copy(void *des, void *src, uint32_t n);

/**
 * @brief   内存释放函数（类似 free）
 * @param   memx: 内存池编号
 * @param   ptr:  要释放的内存指针
 * @retval  无
 * @note    将 ptr 对应的块标记为空闲
 *          ★ 必须释放由 mymalloc 分配的内存！
 */
void myfree(uint8_t memx, void *ptr);

/**
 * @brief   内存分配函数（类似 malloc）
 * @param   memx: 内存池编号
 * @param   size: 要分配的字节数
 * @retval  分配成功返回内存指针，失败返回 NULL
 * @note    ① 从指定的内存池中分配内存
 *          ② 分配大小会自动对齐到块大小的整数倍
 *          ③ 如果内存不足，返回 NULL
 */
void *mymalloc(uint8_t memx, uint32_t size);

/**
 * @brief   重新分配内存（类似 realloc）
 * @param   memx: 内存池编号
 * @param   ptr:  原有内存指针（可为 NULL）
 * @param   size: 新的内存大小
 * @retval  新内存指针
 * @note    ① 如果 ptr 为 NULL，等价于 mymalloc
 *          ② 如果 size 为 0，等价于 myfree
 *          ③ 先分配新内存，拷贝原数据，再释放旧内存
 */
void *myrealloc(uint8_t memx, void *ptr, uint32_t size);

#endif

