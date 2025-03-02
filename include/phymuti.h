/**
 * @file phymuti.h
 * @brief PhyMuTi - 物理设备模拟工具包主头文件
 */

#ifndef PHYMUTI_H
#define PHYMUTI_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 版本信息 */
#define PHYMUTI_VERSION_MAJOR 0
#define PHYMUTI_VERSION_MINOR 1
#define PHYMUTI_VERSION_PATCH 0

/* 包含各个模块的头文件 */
#include "phymuti_error.h"
#include "device_manager.h"
#include "memory_manager.h"
#include "monitor.h"
#include "action_manager.h"
#include "rule_engine.h"

/**
 * @brief 初始化PhyMuTi系统
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_init(void);

/**
 * @brief 处理PhyMuTi系统中的事件
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_process_events(void);

/**
 * @brief 清理PhyMuTi系统资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_cleanup(void);

/**
 * @brief 获取PhyMuTi版本信息
 * 
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁版本号
 */
void phymuti_get_version(int *major, int *minor, int *patch);

#endif /* PHYMUTI_H */ 