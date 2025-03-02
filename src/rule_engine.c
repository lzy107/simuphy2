/**
 * @file rule_engine.c
 * @brief 规则引擎模块实现
 */

#include "rule_engine.h"
#include "phymuti_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 规则结构体 */
typedef struct rule_struct {
    rule_id_t id;                /* 规则ID */
    char *name;                  /* 规则名称 */
    rule_condition_t condition;  /* 条件函数 */
    void *condition_user_data;   /* 条件函数用户数据 */
    action_id_t *action_ids;     /* 动作ID数组 */
    uint32_t action_count;       /* 动作数量 */
    uint32_t action_capacity;    /* 动作容量 */
    bool enabled;                /* 是否启用 */
    void *user_data;             /* 用户数据 */
    struct rule_struct *next;    /* 下一个规则 */
} rule_t;

/* 规则链表头 */
static rule_t *rule_list = NULL;

/* 下一个可用的规则ID */
static rule_id_t next_rule_id = 1;

/**
 * @brief 初始化规则引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_init(void) {
    /* 初始化规则链表 */
    rule_list = NULL;
    next_rule_id = 1;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理规则引擎资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_cleanup(void) {
    /* 清理所有规则 */
    rule_t *rule = rule_list;
    rule_t *next_rule;
    
    while (rule) {
        next_rule = rule->next;
        
        /* 释放资源 */
        if (rule->name) {
            free(rule->name);
        }
        if (rule->action_ids) {
            free(rule->action_ids);
        }
        free(rule);
        
        rule = next_rule;
    }
    
    rule_list = NULL;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 查找规则
 * 
 * @param id 规则ID
 * @return rule_t* 成功返回规则指针，失败返回NULL
 */
static rule_t* find_rule_by_id(rule_id_t id) {
    rule_t *rule = rule_list;
    
    while (rule) {
        if (rule->id == id) {
            return rule;
        }
        rule = rule->next;
    }
    
    return NULL;
}

/**
 * @brief 根据名称查找规则
 * 
 * @param name 规则名称
 * @return rule_t* 成功返回规则指针，失败返回NULL
 */
static rule_t* find_rule_by_name(const char *name) {
    if (!name) {
        return NULL;
    }
    
    rule_t *rule = rule_list;
    
    while (rule) {
        if (rule->name && strcmp(rule->name, name) == 0) {
            return rule;
        }
        rule = rule->next;
    }
    
    return NULL;
}

/**
 * @brief 创建规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回RULE_INVALID_ID
 */
rule_id_t rule_create(const char *name) {
    if (!name) {
        return RULE_INVALID_ID;
    }
    
    /* 检查规则名称是否已存在 */
    if (find_rule_by_name(name)) {
        return RULE_INVALID_ID;  /* 规则名称已存在 */
    }
    
    /* 创建新的规则 */
    rule_t *rule = (rule_t *)malloc(sizeof(rule_t));
    if (!rule) {
        return RULE_INVALID_ID;
    }
    
    /* 初始化规则 */
    rule->id = next_rule_id++;
    rule->name = strdup(name);
    if (!rule->name) {
        free(rule);
        return RULE_INVALID_ID;
    }
    rule->condition = NULL;
    rule->condition_user_data = NULL;
    rule->action_ids = NULL;
    rule->action_count = 0;
    rule->action_capacity = 0;
    rule->enabled = false;
    rule->user_data = NULL;
    
    /* 添加到规则链表 */
    rule->next = rule_list;
    rule_list = rule;
    
    return rule->id;
}

/**
 * @brief 销毁规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_destroy(rule_id_t id) {
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *prev = NULL;
    rule_t *rule = rule_list;
    
    while (rule) {
        if (rule->id == id) {
            /* 从链表中移除 */
            if (prev) {
                prev->next = rule->next;
            } else {
                rule_list = rule->next;
            }
            
            /* 释放资源 */
            if (rule->name) {
                free(rule->name);
            }
            if (rule->action_ids) {
                free(rule->action_ids);
            }
            free(rule);
            
            return PHYMUTI_SUCCESS;
        }
        
        prev = rule;
        rule = rule->next;
    }
    
    return PHYMUTI_ERROR_RULE_NOT_FOUND;
}

/**
 * @brief 设置规则条件
 * 
 * @param id 规则ID
 * @param condition 条件函数
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int rule_set_condition(rule_id_t id, rule_condition_t condition, void *user_data) {
    if (id == RULE_INVALID_ID || !condition) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->condition = condition;
    rule->condition_user_data = user_data;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 添加规则动作
 * 
 * @param id 规则ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_add_action(rule_id_t id, action_id_t action_id) {
    if (id == RULE_INVALID_ID || action_id == ACTION_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    /* 检查动作是否已添加 */
    for (uint32_t i = 0; i < rule->action_count; i++) {
        if (rule->action_ids[i] == action_id) {
            return PHYMUTI_SUCCESS;  /* 已添加 */
        }
    }
    
    /* 检查是否需要扩容 */
    if (rule->action_count >= rule->action_capacity) {
        uint32_t new_capacity = rule->action_capacity == 0 ? 4 : rule->action_capacity * 2;
        action_id_t *new_action_ids = (action_id_t *)realloc(rule->action_ids, 
                                                           new_capacity * sizeof(action_id_t));
        if (!new_action_ids) {
            return PHYMUTI_ERROR_OUT_OF_MEMORY;
        }
        
        rule->action_ids = new_action_ids;
        rule->action_capacity = new_capacity;
    }
    
    /* 添加动作ID */
    rule->action_ids[rule->action_count++] = action_id;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 移除规则动作
 * 
 * @param id 规则ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_remove_action(rule_id_t id, action_id_t action_id) {
    if (id == RULE_INVALID_ID || action_id == ACTION_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    /* 查找动作ID */
    for (uint32_t i = 0; i < rule->action_count; i++) {
        if (rule->action_ids[i] == action_id) {
            /* 移除动作ID */
            if (i < rule->action_count - 1) {
                memmove(&rule->action_ids[i], &rule->action_ids[i + 1], 
                       (rule->action_count - i - 1) * sizeof(action_id_t));
            }
            rule->action_count--;
            
            return PHYMUTI_SUCCESS;
        }
    }
    
    return PHYMUTI_ERROR_NOT_FOUND;
}

/**
 * @brief 启用规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_enable(rule_id_t id) {
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->enabled = true;
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 禁用规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_disable(rule_id_t id) {
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->enabled = false;
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 评估规则
 * 
 * @param id 规则ID
 * @param context 监视点上下文
 * @return int 成功返回0，失败返回错误码
 */
int rule_evaluate(rule_id_t id, const monitor_context_t *context) {
    if (id == RULE_INVALID_ID || !context) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    /* 检查规则是否启用 */
    if (!rule->enabled) {
        return PHYMUTI_SUCCESS;  /* 规则未启用，不执行 */
    }
    
    /* 检查是否有条件函数 */
    if (!rule->condition) {
        return PHYMUTI_SUCCESS;  /* 没有条件函数，不执行 */
    }
    
    /* 评估条件 */
    bool result = false;
    
    /* 直接调用条件函数，在C语言中无法捕获异常 */
    result = rule->condition(context, rule->condition_user_data);
    
    /* 如果条件满足，执行所有动作 */
    if (result) {
        for (uint32_t i = 0; i < rule->action_count; i++) {
            int ret = action_execute(rule->action_ids[i], context);
            if (ret != PHYMUTI_SUCCESS) {
                return PHYMUTI_ERROR_RULE_ACTION_FAILED;
            }
        }
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 根据名称查找规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回RULE_INVALID_ID
 */
rule_id_t rule_find_by_name(const char *name) {
    if (!name) {
        return RULE_INVALID_ID;
    }
    
    rule_t *rule = find_rule_by_name(name);
    return rule ? rule->id : RULE_INVALID_ID;
}

/**
 * @brief 获取规则名称
 * 
 * @param id 规则ID
 * @return const char* 规则名称
 */
const char* rule_get_name(rule_id_t id) {
    rule_t *rule = find_rule_by_id(id);
    return rule ? rule->name : NULL;
}

/**
 * @brief 设置规则用户数据
 * 
 * @param id 规则ID
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int rule_set_user_data(rule_id_t id, void *user_data) {
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->user_data = user_data;
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 获取规则用户数据
 * 
 * @param id 规则ID
 * @param user_data 用户数据指针的指针
 * @return int 成功返回0，失败返回错误码
 */
int rule_get_user_data(rule_id_t id, void **user_data) {
    if (id == RULE_INVALID_ID || !user_data) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    *user_data = rule->user_data;
    return PHYMUTI_SUCCESS;
} 