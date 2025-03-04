/**
 * @file rule_engine.c
 * @brief 规则引擎模块实现
 */

#include "rule_engine.h"
#include "phymuti_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

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

/* 规则链表的递归互斥锁 */
static pthread_mutex_t rule_mutex;
static pthread_mutexattr_t rule_mutex_attr;

/**
 * @brief 初始化规则引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_init(void) {
    int ret;
    pthread_mutexattr_t mutex_attr;
    
    /* 初始化互斥锁属性 */
    ret = pthread_mutexattr_init(&mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 设置互斥锁属性 */
    ret = pthread_mutexattr_settype(&mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0) {
        pthread_mutexattr_destroy(&mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 初始化互斥锁 */
    ret = pthread_mutex_init(&rule_mutex, &mutex_attr);
    if (ret != 0) {
        pthread_mutexattr_destroy(&mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 销毁互斥锁属性 */
    ret = pthread_mutexattr_destroy(&mutex_attr);
    if (ret != 0) {
        pthread_mutex_destroy(&rule_mutex);
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    /* 初始化规则列表 */
    rule_list = NULL;
    next_rule_id = 1;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理规则引擎
 * 
 * @return int 成功返回0，失败返回错误码
 */
int rule_engine_cleanup(void) {
    int ret;
    
    /* 加锁 */
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 释放所有规则 */
    rule_t *rule = rule_list;
    rule_t *next_rule;
    
    while (rule) {
        next_rule = rule->next;
        
        /* 释放规则名称 */
        if (rule->name) {
            free(rule->name);
        }
        
        /* 释放动作ID数组 */
        if (rule->action_ids) {
            free(rule->action_ids);
        }
        
        /* 释放规则结构体 */
        free(rule);
        
        rule = next_rule;
    }
    
    rule_list = NULL;
    next_rule_id = 1;
    
    /* 解锁 */
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    /* 销毁互斥锁 */
    ret = pthread_mutex_destroy(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 根据ID查找规则
 * 
 * @param id 规则ID
 * @return rule_t* 成功返回规则指针，失败返回NULL
 */
static rule_t* find_rule_by_id(rule_id_t id) {
    rule_t *rule;
    
    /* 遍历规则链表 */
    pthread_mutex_lock(&rule_mutex);
    
    rule = rule_list;
    while (rule) {
        if (rule->id == id) {
            pthread_mutex_unlock(&rule_mutex);
            return rule;
        }
        
        rule = rule->next;
    }
    
    pthread_mutex_unlock(&rule_mutex);
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
    
    pthread_mutex_lock(&rule_mutex);
    
    rule_t *rule = rule_list;
    
    while (rule) {
        if (rule->name && strcmp(rule->name, name) == 0) {
            pthread_mutex_unlock(&rule_mutex);
            return rule;
        }
        rule = rule->next;
    }
    
    pthread_mutex_unlock(&rule_mutex);
    return NULL;
}

/**
 * @brief 创建规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回0
 */
rule_id_t rule_create(const char *name) {
    rule_t *rule;
    rule_id_t id;
    int ret;
    
    /* 检查参数 */
    if (!name || strlen(name) == 0) {
        return RULE_INVALID_ID;
    }
    
    /* 分配规则结构体 */
    rule = (rule_t *)malloc(sizeof(rule_t));
    if (!rule) {
        return RULE_INVALID_ID;
    }
    
    /* 复制规则名称 */
    rule->name = strdup(name);
    if (!rule->name) {
        free(rule);
        return RULE_INVALID_ID;
    }
    
    /* 初始化其他属性 */
    rule->condition = NULL;
    rule->condition_user_data = NULL;
    rule->action_ids = NULL;
    rule->action_count = 0;
    rule->action_capacity = 0;
    rule->enabled = true;
    rule->user_data = NULL;
    
    /* 为规则分配ID并添加到链表 */
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        free(rule->name);
        free(rule);
        return RULE_INVALID_ID;
    }
    
    id = next_rule_id++;
    rule->id = id;
    
    /* 添加到规则链表 */
    rule->next = rule_list;
    rule_list = rule;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        /* 锁释放失败，但规则已创建
           记录错误但返回创建的ID */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    return id;
}

/**
 * @brief 销毁规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_destroy(rule_id_t id) {
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
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
            
            /* 释放规则名称 */
            if (rule->name) {
                free(rule->name);
            }
            
            /* 释放动作ID数组 */
            if (rule->action_ids) {
                free(rule->action_ids);
            }
            
            /* 释放规则结构体 */
            free(rule);
            
            ret = pthread_mutex_unlock(&rule_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            
            return PHYMUTI_SUCCESS;
        }
        
        prev = rule;
        rule = rule->next;
    }
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
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
    int ret;
    
    if (id == RULE_INVALID_ID || !condition) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->condition = condition;
    rule->condition_user_data = user_data;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
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
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    /* 检查动作是否已添加 */
    for (uint32_t i = 0; i < rule->action_count; i++) {
        if (rule->action_ids[i] == action_id) {
            ret = pthread_mutex_unlock(&rule_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            return PHYMUTI_SUCCESS;  /* 动作已添加 */
        }
    }
    
    /* 检查是否需要扩容 */
    if (rule->action_count >= rule->action_capacity) {
        uint32_t new_capacity = rule->action_capacity == 0 ? 4 : rule->action_capacity * 2;
        action_id_t *new_action_ids = (action_id_t *)realloc(rule->action_ids, 
                                                          new_capacity * sizeof(action_id_t));
        if (!new_action_ids) {
            ret = pthread_mutex_unlock(&rule_mutex);
            if (ret != 0) {
                /* 锁释放失败，但内存分配已经失败 */
                /* 在实际应用中可以考虑记录错误日志 */
            }
            return PHYMUTI_ERROR_OUT_OF_MEMORY;
        }
        
        rule->action_ids = new_action_ids;
        rule->action_capacity = new_capacity;
    }
    
    /* 添加动作ID */
    rule->action_ids[rule->action_count++] = action_id;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
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
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
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
            
            ret = pthread_mutex_unlock(&rule_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            
            return PHYMUTI_SUCCESS;
        }
    }
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
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
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->enabled = true;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 禁用规则
 * 
 * @param id 规则ID
 * @return int 成功返回0，失败返回错误码
 */
int rule_disable(rule_id_t id) {
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->enabled = false;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 评估规则
 * 
 * @param id 规则ID
 * @param context 上下文
 * @return int 成功返回0，失败返回错误码
 */
int rule_evaluate(rule_id_t id, const monitor_context_t *context) {
    int ret;
    rule_t *rule;
    bool is_match = false;
    
    if (id == RULE_INVALID_ID || !context) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找规则 */
    rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    /* 检查规则是否启用 */
    if (!rule->enabled) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_DISABLED;
    }
    
    /* 检查是否有条件函数 */
    if (!rule->condition) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NO_CONDITION;
    }
    
    /* 收集需要执行的动作，避免在持有锁时调用外部函数 */
    uint32_t action_count = rule->action_count;
    action_id_t action_ids[32];  /* 最多支持32个动作 */
    
    if (action_count > 32) {
        action_count = 32;
    }
    
    /* 复制动作ID */
    for (uint32_t i = 0; i < action_count; i++) {
        action_ids[i] = rule->action_ids[i];
    }
    
    /* 执行条件函数 */
    is_match = rule->condition(context, rule->condition_user_data);
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    /* 如果条件匹配，执行所有动作 */
    if (is_match) {
        for (uint32_t i = 0; i < action_count; i++) {
            action_execute(action_ids[i], context);
        }
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 通过名称查找规则
 * 
 * @param name 规则名称
 * @return rule_id_t 成功返回规则ID，失败返回RULE_INVALID_ID
 */
rule_id_t rule_find_by_name(const char *name) {
    int ret;
    rule_id_t id = RULE_INVALID_ID;
    
    if (!name) {
        return RULE_INVALID_ID;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return RULE_INVALID_ID;
    }
    
    rule_t *rule = rule_list;
    
    while (rule) {
        if (rule->name && strcmp(rule->name, name) == 0) {
            id = rule->id;
            break;
        }
        rule = rule->next;
    }
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        /* 锁释放失败，但已找到规则ID */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    return id;
}

/**
 * @brief 获取规则名称
 * 
 * @param id 规则ID
 * @return const char* 成功返回规则名称，失败返回NULL
 */
const char* rule_get_name(rule_id_t id) {
    int ret;
    const char *name = NULL;
    
    if (id == RULE_INVALID_ID) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    rule_t *rule = find_rule_by_id(id);
    if (rule) {
        name = rule->name;
    }
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        /* 锁释放失败，但已获取规则名称 */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    return name;
}

/**
 * @brief 设置规则用户数据
 * 
 * @param id 规则ID
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int rule_set_user_data(rule_id_t id, void *user_data) {
    int ret;
    
    if (id == RULE_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    rule->user_data = user_data;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
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
    int ret;
    
    if (id == RULE_INVALID_ID || !user_data) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    rule_t *rule = find_rule_by_id(id);
    if (!rule) {
        ret = pthread_mutex_unlock(&rule_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_RULE_NOT_FOUND;
    }
    
    *user_data = rule->user_data;
    
    ret = pthread_mutex_unlock(&rule_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
} 