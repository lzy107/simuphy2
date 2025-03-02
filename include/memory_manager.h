/**
 * @file memory_manager.h
 * @brief 内存管理器模块头文件
 */

#ifndef MEMORY_MANAGER_H
#define MEMORY_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "device_manager.h"

/* 内存区域标志 */
#define MEMORY_FLAG_READ  (1 << 0)  /* 可读 */
#define MEMORY_FLAG_WRITE (1 << 1)  /* 可写 */
#define MEMORY_FLAG_EXEC  (1 << 2)  /* 可执行 */
#define MEMORY_FLAG_RW    (MEMORY_FLAG_READ | MEMORY_FLAG_WRITE)  /* 可读写 */
#define MEMORY_FLAG_RX    (MEMORY_FLAG_READ | MEMORY_FLAG_EXEC)   /* 可读执行 */
#define MEMORY_FLAG_RWX   (MEMORY_FLAG_READ | MEMORY_FLAG_WRITE | MEMORY_FLAG_EXEC)  /* 可读写执行 */

/* 内存访问类型 */
typedef enum {
    MEMORY_ACCESS_READ,   /* 读访问 */
    MEMORY_ACCESS_WRITE,  /* 写访问 */
    MEMORY_ACCESS_EXEC    /* 执行访问 */
} memory_access_type_t;

/* 内存区域结构体 */
typedef struct memory_region_struct memory_region_t;

/**
 * @brief 初始化内存管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int memory_manager_init(void);

/**
 * @brief 清理内存管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int memory_manager_cleanup(void);

/**
 * @brief 创建内存区域
 * 
 * @param device 关联的设备句柄
 * @param name 内存区域名称
 * @param base_addr 基地址
 * @param size 大小（字节）
 * @param flags 标志
 * @return memory_region_t* 成功返回内存区域指针，失败返回NULL
 */
memory_region_t* memory_region_create(device_handle_t device, const char *name, 
                                     uint64_t base_addr, size_t size, uint32_t flags);

/**
 * @brief 销毁内存区域
 * 
 * @param region 内存区域指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_region_destroy(memory_region_t *region);

/**
 * @brief 查找内存区域
 * 
 * @param device 设备句柄
 * @param name 内存区域名称
 * @return memory_region_t* 成功返回内存区域指针，失败返回NULL
 */
memory_region_t* memory_region_find(device_handle_t device, const char *name);

/**
 * @brief 获取内存区域名称
 * 
 * @param region 内存区域指针
 * @return const char* 内存区域名称
 */
const char* memory_region_get_name(const memory_region_t *region);

/**
 * @brief 获取内存区域基地址
 * 
 * @param region 内存区域指针
 * @return uint64_t 基地址
 */
uint64_t memory_region_get_base_addr(const memory_region_t *region);

/**
 * @brief 获取内存区域大小
 * 
 * @param region 内存区域指针
 * @return size_t 大小（字节）
 */
size_t memory_region_get_size(const memory_region_t *region);

/**
 * @brief 获取内存区域标志
 * 
 * @param region 内存区域指针
 * @return uint32_t 标志
 */
uint32_t memory_region_get_flags(const memory_region_t *region);

/**
 * @brief 获取内存区域关联的设备
 * 
 * @param region 内存区域指针
 * @return device_handle_t 设备句柄
 */
device_handle_t memory_region_get_device(const memory_region_t *region);

/**
 * @brief 读取内存字节
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_byte(memory_region_t *region, uint64_t addr, uint8_t *value);

/**
 * @brief 写入内存字节
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_byte(memory_region_t *region, uint64_t addr, uint8_t value);

/**
 * @brief 读取内存半字（16位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_halfword(memory_region_t *region, uint64_t addr, uint16_t *value);

/**
 * @brief 写入内存半字（16位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_halfword(memory_region_t *region, uint64_t addr, uint16_t value);

/**
 * @brief 读取内存字（32位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_word(memory_region_t *region, uint64_t addr, uint32_t *value);

/**
 * @brief 写入内存字（32位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_word(memory_region_t *region, uint64_t addr, uint32_t value);

/**
 * @brief 读取内存双字（64位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_doubleword(memory_region_t *region, uint64_t addr, uint64_t *value);

/**
 * @brief 写入内存双字（64位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_doubleword(memory_region_t *region, uint64_t addr, uint64_t value);

/**
 * @brief 读取内存块
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param buffer 缓冲区
 * @param size 大小（字节）
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_buffer(memory_region_t *region, uint64_t addr, void *buffer, size_t size);

/**
 * @brief 写入内存块
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param buffer 缓冲区
 * @param size 大小（字节）
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_buffer(memory_region_t *region, uint64_t addr, const void *buffer, size_t size);

#endif /* MEMORY_MANAGER_H */ 