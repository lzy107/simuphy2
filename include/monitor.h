/**
 * @file monitor.h
 * @brief 监视器模块头文件
 */

#ifndef MONITOR_H
#define MONITOR_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "memory_manager.h"

/* 监视点ID类型 */
typedef uint32_t monitor_id_t;

/* 无效的监视点ID */
#define MONITOR_INVALID_ID 0

/* 监视点类型 */
typedef enum {
    WATCHPOINT_READ = 1,    /* 读监视点 */
    WATCHPOINT_WRITE = 2,   /* 写监视点 */
    WATCHPOINT_ACCESS = 3,  /* 访问监视点（读或写） */
    WATCHPOINT_VALUE_WRITE = 4, /* 特定值写入监视点 */
} watchpoint_type_t;

/* 监视点上下文 */
typedef struct {
    memory_region_t *region;  /* 内存区域 */
    uint64_t address;         /* 地址 */
    uint32_t size;            /* 大小 */
    uint64_t value;           /* 值 */
    memory_access_type_t access_type;  /* 访问类型 */
} monitor_context_t;

/**
 * @brief 初始化监视器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int monitor_init(void);

/**
 * @brief 清理监视器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int monitor_cleanup(void);

/**
 * @brief 添加监视点
 * 
 * @param region 内存区域
 * @param addr 地址
 * @param size 大小（字节）
 * @param type 监视点类型
 * @param wpvalue 要监视的值（仅当type为WATCHPOINT_VALUE_WRITE时使用）
 * @return monitor_id_t 成功返回监视点ID，失败返回MONITOR_INVALID_ID
 */
monitor_id_t monitor_add_watchpoint(memory_region_t *region, uint64_t addr, 
                                   uint32_t size, watchpoint_type_t type, uint64_t wpvalue);

/**
 * @brief 删除监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_remove_watchpoint(monitor_id_t id);

/**
 * @brief 启用监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_enable_watchpoint(monitor_id_t id);

/**
 * @brief 禁用监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_disable_watchpoint(monitor_id_t id);

/**
 * @brief 绑定动作到监视点
 * 
 * @param id 监视点ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_bind_action(monitor_id_t id, uint32_t action_id);

/**
 * @brief 解绑动作
 * 
 * @param id 监视点ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_unbind_action(monitor_id_t id, uint32_t action_id);

/**
 * @brief 获取监视点信息
 * 
 * @param id 监视点ID
 * @param region 内存区域指针的指针
 * @param addr 地址指针
 * @param size 大小指针
 * @param type 类型指针
 * @return int 成功返回0，失败返回错误码
 */
int monitor_get_watchpoint_info(monitor_id_t id, memory_region_t **region, 
                               uint64_t *addr, uint32_t *size, watchpoint_type_t *type);

/**
 * @brief 通知内存访问
 * 
 * @param region 内存区域
 * @param addr 地址
 * @param size 大小（字节）
 * @param value 值
 * @param access_type 访问类型
 * @return int 成功返回0，失败返回错误码
 */
int monitor_notify_memory_access(memory_region_t *region, uint64_t addr, 
                                uint32_t size, uint64_t value, 
                                memory_access_type_t access_type);

#endif /* MONITOR_H */ 