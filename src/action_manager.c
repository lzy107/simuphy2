/**
 * @file action_manager.c
 * @brief 动作管理器模块实现
 */

#include "action_manager.h"
#include "phymuti_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

/* 动作结构体 */
typedef struct action_struct {
    action_id_t id;              /* 动作ID */
    action_type_t type;          /* 类型 */
    union {
        struct {
            action_callback_t callback;  /* 回调函数 */
        } callback_data;
        struct {
            char *path;                  /* 脚本路径 */
        } script_data;
        struct {
            char *command;               /* 命令字符串 */
        } command_data;
    } data;
    void *user_data;             /* 用户数据 */
    struct action_struct *next;  /* 下一个动作 */
} action_t;

/* 动作链表头 */
static action_t *action_list = NULL;

/* 下一个可用的动作ID */
static action_id_t next_action_id = 1;

/* 动作链表的递归互斥锁 */
static pthread_mutex_t action_mutex;
static pthread_mutexattr_t action_mutex_attr;

/**
 * @brief 初始化动作管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int action_manager_init(void) {
    int ret;
    
    /* 初始化递归互斥锁 */
    ret = pthread_mutexattr_init(&action_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutexattr_settype(&action_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0) {
        pthread_mutexattr_destroy(&action_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutex_init(&action_mutex, &action_mutex_attr);
    if (ret != 0) {
        pthread_mutexattr_destroy(&action_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 初始化动作链表 */
    action_list = NULL;
    next_action_id = 1;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理动作管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int action_manager_cleanup(void) {
    int ret;
    
    /* 清理所有动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    action_t *action = action_list;
    action_t *next_action;
    
    while (action) {
        next_action = action->next;
        
        /* 根据动作类型释放资源 */
        if (action->type == ACTION_TYPE_SCRIPT) {
            free(action->data.script_data.path);
        } else if (action->type == ACTION_TYPE_COMMAND) {
            free(action->data.command_data.command);
        }
        
        /* 释放动作结构体 */
        free(action);
        
        action = next_action;
    }
    
    action_list = NULL;
    next_action_id = 1;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    /* 销毁互斥锁 */
    ret = pthread_mutex_destroy(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    ret = pthread_mutexattr_destroy(&action_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 根据ID查找动作
 * 
 * @param id 动作ID
 * @return action_t* 成功返回动作指针，失败返回NULL
 */
static action_t* find_action(action_id_t id) {
    action_t *action;
    int ret;
    
    /* 遍历动作链表 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return NULL; /* 加锁失败返回NULL */
    }
    
    action = action_list;
    while (action) {
        if (action->id == id) {
            ret = pthread_mutex_unlock(&action_mutex);
            if (ret != 0) {
                return NULL; /* 败返回NULL */
            }
            return action;
        }
        
        action = action->next;
    }
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return NULL; /* 败返回NULL */
    }
    return NULL;
}

/**
 * @brief 根据ID查找动作(不解锁版本，供内部使用)
 * 
 * @param id 动作ID
 * @return action_t* 成功返回动作指针，失败返回NULL
 */
static action_t* find_action_locked(action_id_t id) {
    action_t *action = action_list;
    
    while (action) {
        if (action->id == id) {
            return action;
        }
        
        action = action->next;
    }
    
    return NULL;
}

/**
 * @brief 创建回调函数类型的动作
 * 
 * @param callback 回调函数
 * @param user_data 用户数据
 * @return action_id_t 成功返回动作ID，失败返回0
 */
action_id_t action_create_callback(action_callback_t callback, void *user_data) {
    action_t *action;
    action_id_t id;
    int ret;
    
    /* 检查参数 */
    if (!callback) {
        return 0;
    }
    
    /* 创建动作 */
    action = (action_t *)malloc(sizeof(action_t));
    if (!action) {
        return 0;
    }
    
    /* 初始化动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        free(action);
        return 0;
    }
    
    id = next_action_id++;
    action->id = id;
    action->type = ACTION_TYPE_CALLBACK;
    action->data.callback_data.callback = callback;
    action->user_data = user_data;
    
    /* 添加到动作链表 */
    action->next = action_list;
    action_list = action;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        /* 解锁失败，但动作已创建，返回ID */
        /* 实际应用中可能需要额外处理，这里简化处理 */
    }
    
    return id;
}

/**
 * @brief 创建脚本动作
 * 
 * @param script_path 脚本路径
 * @return action_id_t 成功返回动作ID，失败返回ACTION_INVALID_ID
 */
action_id_t action_create_script(const char *script_path) {
    int ret;
    
    if (!script_path) {
        return ACTION_INVALID_ID;
    }
    
    /* 创建新的动作 */
    action_t *action = (action_t *)malloc(sizeof(action_t));
    if (!action) {
        return ACTION_INVALID_ID;
    }
    
    /* 复制脚本路径 */
    char *path_copy = strdup(script_path);
    if (!path_copy) {
        free(action);
        return ACTION_INVALID_ID;
    }
    
    /* 初始化动作并添加到链表 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        free(path_copy);
        free(action);
        return ACTION_INVALID_ID;
    }
    
    action->id = next_action_id++;
    action->type = ACTION_TYPE_SCRIPT;
    action->data.script_data.path = path_copy;
    action->user_data = NULL;
    
    /* 添加到动作链表 */
    action->next = action_list;
    action_list = action;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        /* 解锁失败，但动作已创建，返回ID */
        /* 实际应用中可能需要额外处理，这里简化处理 */
    }
    
    return action->id;
}

/**
 * @brief 创建命令动作
 * 
 * @param command 命令字符串
 * @return action_id_t 成功返回动作ID，失败返回ACTION_INVALID_ID
 */
action_id_t action_create_command(const char *command) {
    int ret;
    
    if (!command) {
        return ACTION_INVALID_ID;
    }
    
    /* 创建新的动作 */
    action_t *action = (action_t *)malloc(sizeof(action_t));
    if (!action) {
        return ACTION_INVALID_ID;
    }
    
    /* 复制命令字符串 */
    char *command_copy = strdup(command);
    if (!command_copy) {
        free(action);
        return ACTION_INVALID_ID;
    }
    
    /* 初始化动作并添加到链表 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        free(command_copy);
        free(action);
        return ACTION_INVALID_ID;
    }
    
    action->id = next_action_id++;
    action->type = ACTION_TYPE_COMMAND;
    action->data.command_data.command = command_copy;
    action->user_data = NULL;
    
    /* 添加到动作链表 */
    action->next = action_list;
    action_list = action;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        /* 解锁失败，但动作已创建，返回ID */
        /* 实际应用中可能需要额外处理，这里简化处理 */
    }
    
    return action->id;
}

/**
 * @brief 销毁动作
 * 
 * @param id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int action_destroy(action_id_t id) {
    int ret;
    
    if (id == ACTION_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 查找动作 */
    action_t *prev = NULL;
    action_t *action = action_list;
    
    while (action) {
        if (action->id == id) {
            /* 从链表中移除 */
            if (prev) {
                prev->next = action->next;
            } else {
                action_list = action->next;
            }
            
            /* 根据类型释放资源 */
            switch (action->type) {
                case ACTION_TYPE_SCRIPT:
                    if (action->data.script_data.path) {
                        free(action->data.script_data.path);
                    }
                    break;
                    
                case ACTION_TYPE_COMMAND:
                    if (action->data.command_data.command) {
                        free(action->data.command_data.command);
                    }
                    break;
                    
                default:
                    break;
            }
            
            /* 释放动作 */
            free(action);
            
            ret = pthread_mutex_unlock(&action_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            return PHYMUTI_SUCCESS;
        }
        
        prev = action;
        action = action->next;
    }
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return PHYMUTI_ERROR_ACTION_NOT_FOUND;
}

/**
 * @brief 执行动作
 * 
 * @param id 动作ID
 * @param context 监视点上下文
 * @return int 成功返回0，失败返回错误码
 */
int action_execute(action_id_t id, const monitor_context_t *context) {
    int ret;
    
    if (id == ACTION_INVALID_ID || !context) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    action_t *action = find_action_locked(id);
    if (!action) {
        ret = pthread_mutex_unlock(&action_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_ACTION_NOT_FOUND;
    }
    
    /* 根据类型执行动作 */
    int result = PHYMUTI_SUCCESS;
    
    switch (action->type) {
        case ACTION_TYPE_CALLBACK:
            if (action->data.callback_data.callback) {
                /* 回调可能需要调用管理器其他函数, 所以在执行前解锁 */
                ret = pthread_mutex_unlock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
                }
                result = action->data.callback_data.callback(context, action->user_data);
                /* 再次加锁以保持一致性 */
                ret = pthread_mutex_lock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
                }
            }
            break;
            
        case ACTION_TYPE_SCRIPT:
            if (action->data.script_data.path) {
                /* 构建命令行 */
                char cmd[1024];
                snprintf(cmd, sizeof(cmd), "%s %lu %u %lu %d",
                        action->data.script_data.path,
                        context->address,
                        context->size,
                        context->value,
                        (int)context->access_type);
                
                /* 执行脚本 - 先解锁再执行外部命令 */
                ret = pthread_mutex_unlock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
                }
                result = system(cmd);
                ret = pthread_mutex_lock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
                }
                
                if (result != 0) {
                    result = PHYMUTI_ERROR_ACTION_EXECUTE_FAILED;
                }
            }
            break;
            
        case ACTION_TYPE_COMMAND:
            if (action->data.command_data.command) {
                /* 执行命令 - 先解锁再执行外部命令 */
                ret = pthread_mutex_unlock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
                }
                result = system(action->data.command_data.command);
                ret = pthread_mutex_lock(&action_mutex);
                if (ret != 0) {
                    return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
                }
                
                if (result != 0) {
                    result = PHYMUTI_ERROR_ACTION_EXECUTE_FAILED;
                }
            }
            break;
            
        default:
            result = PHYMUTI_ERROR_ACTION_INVALID_TYPE;
            break;
    }
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return result;
}

/**
 * @brief 获取动作类型
 * 
 * @param id 动作ID
 * @param type 类型指针
 * @return int 成功返回0，失败返回错误码
 */
int action_get_type(action_id_t id, action_type_t *type) {
    int ret;
    
    if (id == ACTION_INVALID_ID || !type) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    action_t *action = find_action_locked(id);
    if (!action) {
        ret = pthread_mutex_unlock(&action_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_ACTION_NOT_FOUND;
    }
    
    *type = action->type;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 设置动作用户数据
 * 
 * @param id 动作ID
 * @param user_data 用户数据
 * @return int 成功返回0，失败返回错误码
 */
int action_set_user_data(action_id_t id, void *user_data) {
    int ret;
    
    if (id == ACTION_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    action_t *action = find_action_locked(id);
    if (!action) {
        ret = pthread_mutex_unlock(&action_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_ACTION_NOT_FOUND;
    }
    
    action->user_data = user_data;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 获取动作用户数据
 * 
 * @param id 动作ID
 * @param user_data 用户数据指针的指针
 * @return int 成功返回0，失败返回错误码
 */
int action_get_user_data(action_id_t id, void **user_data) {
    int ret;
    
    if (id == ACTION_INVALID_ID || !user_data) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找动作 */
    ret = pthread_mutex_lock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    action_t *action = find_action_locked(id);
    if (!action) {
        ret = pthread_mutex_unlock(&action_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_ACTION_NOT_FOUND;
    }
    
    *user_data = action->user_data;
    
    ret = pthread_mutex_unlock(&action_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return PHYMUTI_SUCCESS;
} 