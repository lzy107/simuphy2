/**
 * @file phymuti.c
 * @brief PhyMuTi主模块实现
 */

#include "phymuti.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/**
 * @brief 初始化PhyMuTi系统
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_init(void) {
    int ret;
    
    /* 初始化设备管理器 */
    ret = device_manager_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to initialize device manager: %s\n", phymuti_error_string(ret));
        return ret;
    }
    
    /* 初始化内存管理器 */
    ret = memory_manager_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to initialize memory manager: %s\n", phymuti_error_string(ret));
        device_manager_cleanup();
        return ret;
    }
    
    /* 初始化监视器 */
    ret = monitor_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to initialize monitor: %s\n", phymuti_error_string(ret));
        memory_manager_cleanup();
        device_manager_cleanup();
        return ret;
    }
    
    /* 初始化动作管理器 */
    ret = action_manager_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to initialize action manager: %s\n", phymuti_error_string(ret));
        monitor_cleanup();
        memory_manager_cleanup();
        device_manager_cleanup();
        return ret;
    }
    
    /* 初始化规则引擎 */
    ret = rule_engine_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to initialize rule engine: %s\n", phymuti_error_string(ret));
        action_manager_cleanup();
        monitor_cleanup();
        memory_manager_cleanup();
        device_manager_cleanup();
        return ret;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 处理PhyMuTi系统中的事件
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_process_events(void) {
    /* 这里可以实现事件处理循环 */
    /* 在实际实现中，可能需要从事件队列中获取事件并处理 */
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理PhyMuTi系统资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int phymuti_cleanup(void) {
    int ret;
    
    /* 清理规则引擎 */
    ret = rule_engine_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to cleanup rule engine: %s\n", phymuti_error_string(ret));
        /* 继续清理其他模块 */
    }
    
    /* 清理动作管理器 */
    ret = action_manager_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to cleanup action manager: %s\n", phymuti_error_string(ret));
        /* 继续清理其他模块 */
    }
    
    /* 清理监视器 */
    ret = monitor_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to cleanup monitor: %s\n", phymuti_error_string(ret));
        /* 继续清理其他模块 */
    }
    
    /* 清理内存管理器 */
    ret = memory_manager_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to cleanup memory manager: %s\n", phymuti_error_string(ret));
        /* 继续清理其他模块 */
    }
    
    /* 清理设备管理器 */
    ret = device_manager_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "Failed to cleanup device manager: %s\n", phymuti_error_string(ret));
        /* 返回最后一个错误 */
        return ret;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 获取PhyMuTi版本信息
 * 
 * @param major 主版本号
 * @param minor 次版本号
 * @param patch 补丁版本号
 */
void phymuti_get_version(int *major, int *minor, int *patch) {
    if (major) {
        *major = PHYMUTI_VERSION_MAJOR;
    }
    if (minor) {
        *minor = PHYMUTI_VERSION_MINOR;
    }
    if (patch) {
        *patch = PHYMUTI_VERSION_PATCH;
    }
} 