/**
 * @file action_manager.h
 * @brief 动作管理器模块头文件
 */

#ifndef ACTION_MANAGER_H
#define ACTION_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "monitor.h"

/* 动作ID类型 */
typedef uint32_t action_id_t;

/* 无效的动作ID */
#define ACTION_INVALID_ID 0

/* 动作类型 */
typedef enum {
    ACTION_TYPE_CALLBACK,  /* 回调函数 */
    ACTION_TYPE_SCRIPT,    /* 脚本 */
    ACTION_TYPE_COMMAND,   /* 命令 */
} action_type_t;

/* 动作回调函数类型 */
typedef int (*action_callback_t)(const monitor_context_t *context, void *user_data);

/**
 * @brief 初始化动作管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int action_manager_init(void);

/**
 * @brief 清理动作管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int action_manager_cleanup(void);

/**
 * @brief 创建回调函数动作
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return action_id_t 成功返回动作ID，失败返回ACTION_INVALID_ID
 */
action_id_t action_create_callback(action_callback_t callback, void *user_data);

/**
 * @brief 创建脚本动作
 * 
 * @param script_path 脚本路径
 * @return action_id_t 成功返回动作ID，失败返回ACTION_INVALID_ID
 */
action_id_t action_create_script(const char *script_path);

/**
 * @brief 创建命令动作
 * 
 * @param command 命令字符串
 * @return action_id_t 成功返回动作ID，失败返回ACTION_INVALID_ID
 */
action_id_t action_create_command(const char *command);

/**
 * @brief 销毁动作
 * 
 * @param id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int action_destroy(action_id_t id);

/**
 * @brief 执行动作
 * 
 * @param id 动作ID
 * @param context 监视点上下文
 * @return int 成功返回0，失败返回错误码
 */
int action_execute(action_id_t id, const monitor_context_t *context);

/**
 * @brief 获取动作类型
 * 
 * @param id 动作ID
 * @param type 类型指针
 * @return int 成功返回0，失败返回错误码
 */
int action_get_type(action_id_t id, action_type_t *type);

/**
 * @brief 设置动作用户数据
 * 
 * @param id 动作ID
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int action_set_user_data(action_id_t id, void *user_data);

/**
 * @brief 获取动作用户数据
 * 
 * @param id 动作ID
 * @param user_data 用户数据指针的指针
 * @return int 成功返回0，失败返回错误码
 */
int action_get_user_data(action_id_t id, void **user_data);

#endif /* ACTION_MANAGER_H */ 