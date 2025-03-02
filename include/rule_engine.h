/**
 * @file rule_engine.h
 * @brief 规则引擎模块头文件
 */

#ifndef RULE_ENGINE_H
#define RULE_ENGINE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "action_manager.h"

/* 规则ID类型 */
typedef uint32_t rule_id_t;

/* 无效的规则ID */
#define RULE_INVALID_ID 0

/* 规则条件函数类型 */
typedef bool (*rule_condition_t)(const monitor_context_t *context, void *user_data);

/**
 * @brief 初始化规则引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_init(void);

/**
 * @brief 清理规则引擎资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_cleanup(void);

/**
 * @brief 创建规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回RULE_INVALID_ID
 */
rule_id_t rule_create(const char *name);

/**
 * @brief 销毁规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_destroy(rule_id_t id);

/**
 * @brief 设置规则条件
 * 
 * @param id 规则ID
 * @param condition 条件函数
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int rule_set_condition(rule_id_t id, rule_condition_t condition, void *user_data);

/**
 * @brief 添加规则动作
 * 
 * @param id 规则ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_add_action(rule_id_t id, action_id_t action_id);

/**
 * @brief 移除规则动作
 * 
 * @param id 规则ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_remove_action(rule_id_t id, action_id_t action_id);

/**
 * @brief 启用规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_enable(rule_id_t id);

/**
 * @brief 禁用规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_disable(rule_id_t id);

/**
 * @brief 评估规则
 * 
 * @param id 规则ID
 * @param context 监视点上下文
 * @return int 成功返回0，失败返回错误码
 */
int rule_evaluate(rule_id_t id, const monitor_context_t *context);

/**
 * @brief 根据名称查找规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回RULE_INVALID_ID
 */
rule_id_t rule_find_by_name(const char *name);

/**
 * @brief 获取规则名称
 * 
 * @param id 规则ID
 * @return const char* 规则名称
 */
const char* rule_get_name(rule_id_t id);

/**
 * @brief 设置规则用户数据
 * 
 * @param id 规则ID
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int rule_set_user_data(rule_id_t id, void *user_data);

/**
 * @brief 获取规则用户数据
 * 
 * @param id 规则ID
 * @param user_data 用户数据指针的指针
 * @return int 成功返回0，失败返回错误码
 */
int rule_get_user_data(rule_id_t id, void **user_data);

#endif /* RULE_ENGINE_H */ 