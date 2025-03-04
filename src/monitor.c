/**
 * @file monitor.c
 * @brief 监视器模块实现
 */

#include "monitor.h"
#include "phymuti_error.h"
#include "action_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 监视点结构体 */
typedef struct watchpoint_struct {
    monitor_id_t id;              /* 监视点ID */
    memory_region_t *region;      /* 内存区域 */
    uint64_t addr;                /* 地址 */
    uint32_t size;                /* 大小 */
    watchpoint_type_t type;       /* 类型 */
    bool enabled;                 /* 是否启用 */
    uint64_t wpvalue;             /* 要监视的值 */
    uint32_t *action_ids;         /* 动作ID数组 */
    uint32_t action_count;        /* 动作数量 */
    uint32_t action_capacity;     /* 动作容量 */
    struct watchpoint_struct *next;  /* 下一个监视点 */
} watchpoint_t;

/* 监视点链表头 */
static watchpoint_t *watchpoint_list = NULL;

/* 下一个可用的监视点ID */
static monitor_id_t next_watchpoint_id = 1;

/* 监视点链表的互斥锁 */
static pthread_mutex_t watchpoint_mutex;
static pthread_mutexattr_t watchpoint_mutex_attr;

/**
 * @brief 初始化监视器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int monitor_init(void) {
    int ret;
    
    /* 初始化互斥锁为递归锁 */
    ret = pthread_mutexattr_init(&watchpoint_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutexattr_settype(&watchpoint_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0) {
        pthread_mutexattr_destroy(&watchpoint_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutex_init(&watchpoint_mutex, &watchpoint_mutex_attr);
    if (ret != 0) {
        pthread_mutexattr_destroy(&watchpoint_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 初始化监视点链表 */
    watchpoint_list = NULL;
    next_watchpoint_id = 1;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理监视器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int monitor_cleanup(void) {
    int ret;
    
    /* 清理所有监视点 */
    ret = pthread_mutex_lock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    watchpoint_t *wp = watchpoint_list;
    watchpoint_t *next_wp;
    
    while (wp) {
        next_wp = wp->next;
        
        /* 释放动作ID数组 */
        if (wp->action_ids) {
            free(wp->action_ids);
        }
        
        /* 释放监视点结构体 */
        free(wp);
        
        wp = next_wp;
    }
    
    watchpoint_list = NULL;
    next_watchpoint_id = 1;
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    /* 销毁互斥锁 */
    ret = pthread_mutex_destroy(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    ret = pthread_mutexattr_destroy(&watchpoint_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 查找监视点（不会释放锁，调用者必须释放锁）
 * 
 * @param id 监视点ID
 * @return watchpoint_t* 成功返回监视点指针，失败返回NULL
 */
static watchpoint_t* find_watchpoint(monitor_id_t id) {
    watchpoint_t *wp;
    
    /* 遍历监视点链表 */
    wp = watchpoint_list;
    while (wp) {
        if (wp->id == id) {
            return wp;
        }
        
        wp = wp->next;
    }
    
    return NULL;
}

/**
 * @brief 查找监视点（内部加锁版本）
 * 
 * @param id 监视点ID
 * @return watchpoint_t* 成功返回监视点指针，失败返回NULL
 */
static watchpoint_t* find_watchpoint_locked(monitor_id_t id) {
    int ret;
    
    ret = pthread_mutex_lock(&watchpoint_mutex);
    if (ret != 0) {
        /* 锁获取失败，无法访问共享数据 */
        return NULL;
    }
    
    watchpoint_t *wp = find_watchpoint(id);
    if (!wp) {
        ret = pthread_mutex_unlock(&watchpoint_mutex);
        if (ret != 0) {
            /* 锁释放失败，但已经确定没有找到监视点 */
            /* 在实际应用中可以考虑记录错误日志 */
        }
    }
    
    return wp;
}

/**
 * @brief 添加监视点
 * 
 * @param region 内存区域
 * @param addr 地址
 * @param size 大小
 * @param type 类型
 * @param wpvalue 要监视的值
 * @return monitor_id_t 成功返回监视点ID，失败返回0
 */
monitor_id_t monitor_add_watchpoint(memory_region_t *region, uint64_t addr, 
                                   uint32_t size, watchpoint_type_t type, uint64_t wpvalue) {
    watchpoint_t *wp;
    monitor_id_t id;
    int ret;
    
    /* 检查参数 */
    if (!region || size == 0 || size > 8) {
        return 0;
    }
    
    /* 创建监视点 */
    wp = (watchpoint_t *)malloc(sizeof(watchpoint_t));
    if (!wp) {
        return 0;
    }
    
    /* 初始化监视点 */
    ret = pthread_mutex_lock(&watchpoint_mutex);
    if (ret != 0) {
        /* 锁获取失败，释放已分配的资源 */
        free(wp);
        return 0;
    }
    
    id = next_watchpoint_id++;
    wp->id = id;
    wp->region = region;
    wp->addr = addr;
    wp->size = size;
    wp->type = type;
    wp->enabled = true;
    wp->wpvalue = wpvalue;
    wp->action_ids = NULL;
    wp->action_count = 0;
    wp->action_capacity = 0;
    
    /* 添加到监视点链表 */
    wp->next = watchpoint_list;
    watchpoint_list = wp;
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        /* 锁释放失败，但监视点已创建，
           记录错误但返回创建的ID */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    return id;
}

/**
 * @brief 删除监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_remove_watchpoint(monitor_id_t id) {
    int ret;
    
    if (id == MONITOR_INVALID_ID) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 查找监视点 */
    ret = pthread_mutex_lock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    watchpoint_t *prev = NULL;
    watchpoint_t *wp = watchpoint_list;
    
    while (wp) {
        if (wp->id == id) {
            /* 从链表中移除 */
            if (prev) {
                prev->next = wp->next;
            } else {
                watchpoint_list = wp->next;
            }
            
            /* 释放动作ID数组 */
            if (wp->action_ids) {
                free(wp->action_ids);
            }
            
            /* 释放监视点 */
            free(wp);
            
            ret = pthread_mutex_unlock(&watchpoint_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            
            return PHYMUTI_SUCCESS;
        }
        
        prev = wp;
        wp = wp->next;
    }
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
}

/**
 * @brief 启用监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_enable_watchpoint(monitor_id_t id) {
    int ret;
    
    /* 查找监视点 */
    watchpoint_t *wp = find_watchpoint_locked(id);
    if (!wp) {
        return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
    }
    
    wp->enabled = true;
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 禁用监视点
 * 
 * @param id 监视点ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_disable_watchpoint(monitor_id_t id) {
    int ret;
    
    /* 查找监视点 */
    watchpoint_t *wp = find_watchpoint_locked(id);
    if (!wp) {
        return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
    }
    
    wp->enabled = false;
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 绑定动作到监视点
 * 
 * @param id 监视点ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_bind_action(monitor_id_t id, uint32_t action_id) {
    int ret;
    
    /* 查找监视点 */
    watchpoint_t *wp = find_watchpoint_locked(id);
    if (!wp) {
        return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
    }
    
    /* 检查动作是否已绑定 */
    for (uint32_t i = 0; i < wp->action_count; i++) {
        if (wp->action_ids[i] == action_id) {
            ret = pthread_mutex_unlock(&watchpoint_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            return PHYMUTI_SUCCESS;  /* 已绑定 */
        }
    }
    
    /* 检查是否需要扩容 */
    if (wp->action_count >= wp->action_capacity) {
        uint32_t new_capacity = wp->action_capacity == 0 ? 4 : wp->action_capacity * 2;
        uint32_t *new_action_ids = (uint32_t *)realloc(wp->action_ids, new_capacity * sizeof(uint32_t));
        if (!new_action_ids) {
            ret = pthread_mutex_unlock(&watchpoint_mutex);
            if (ret != 0) {
                /* 锁释放失败，但内存分配已经失败 */
                /* 在实际应用中可以考虑记录错误日志 */
            }
            return PHYMUTI_ERROR_OUT_OF_MEMORY;
        }
        
        wp->action_ids = new_action_ids;
        wp->action_capacity = new_capacity;
    }
    
    /* 添加动作ID */
    wp->action_ids[wp->action_count++] = action_id;
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 解绑动作
 * 
 * @param id 监视点ID
 * @param action_id 动作ID
 * @return int 成功返回0，失败返回错误码
 */
int monitor_unbind_action(monitor_id_t id, uint32_t action_id) {
    int ret;
    
    /* 查找监视点 */
    watchpoint_t *wp = find_watchpoint_locked(id);
    if (!wp) {
        return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
    }
    
    /* 查找动作ID */
    for (uint32_t i = 0; i < wp->action_count; i++) {
        if (wp->action_ids[i] == action_id) {
            /* 移除动作ID */
            if (i < wp->action_count - 1) {
                memmove(&wp->action_ids[i], &wp->action_ids[i + 1], 
                       (wp->action_count - i - 1) * sizeof(uint32_t));
            }
            wp->action_count--;
            
            ret = pthread_mutex_unlock(&watchpoint_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            
            return PHYMUTI_SUCCESS;
        }
    }
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_NOT_FOUND;
}

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
                               uint64_t *addr, uint32_t *size, watchpoint_type_t *type) {
    int ret;
    
    /* 查找监视点 */
    watchpoint_t *wp = find_watchpoint_locked(id);
    if (!wp) {
        return PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND;
    }
    
    /* 返回信息 */
    if (region) {
        *region = wp->region;
    }
    if (addr) {
        *addr = wp->addr;
    }
    if (size) {
        *size = wp->size;
    }
    if (type) {
        *type = wp->type;
    }
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

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
                                memory_access_type_t access_type) {
    int ret;
    
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 遍历所有监视点 */
    ret = pthread_mutex_lock(&watchpoint_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    watchpoint_t *wp = watchpoint_list;
    
    /* 收集需要执行的动作，避免在持有锁时调用外部函数 */
    #define MAX_MATCHES 32
    struct {
        uint32_t action_id;
        monitor_context_t context;
    } matched_actions[MAX_MATCHES];
    int match_count = 0;
    
    while (wp) {
        /* 检查监视点是否启用 */
        if (!wp->enabled) {
            wp = wp->next;
            continue;
        }
        
        /* 检查内存区域是否匹配 */
        if (wp->region != region) {
            wp = wp->next;
            continue;
        }
        
        /* 检查地址范围是否重叠 */
        if (addr + size <= wp->addr || addr >= wp->addr + wp->size) {
            wp = wp->next;
            continue;
        }
        
        /* 检查访问类型是否匹配 */
        bool match = false;
        switch (wp->type) {
            case WATCHPOINT_READ:
                match = (access_type == MEMORY_ACCESS_READ);
                break;
                
            case WATCHPOINT_WRITE:
                match = (access_type == MEMORY_ACCESS_WRITE);
                break;
                
            case WATCHPOINT_ACCESS:
                match = (access_type == MEMORY_ACCESS_READ || 
                         access_type == MEMORY_ACCESS_WRITE);
                break;
                
            case WATCHPOINT_VALUE_WRITE:
                /* 只有在写入操作且值等于wpvalue时才匹配 */
                match = (access_type == MEMORY_ACCESS_WRITE && value == wp->wpvalue);
                break;
        }
        
        if (!match) {
            wp = wp->next;
            continue;
        }
        
        /* 创建监视点上下文 */
        monitor_context_t context;
        context.region = region;
        context.address = addr;
        context.size = size;
        context.value = value;
        context.access_type = access_type;
        
        /* 添加所有绑定的动作到待执行列表 */
        for (uint32_t i = 0; i < wp->action_count && match_count < MAX_MATCHES; i++) {
            matched_actions[match_count].action_id = wp->action_ids[i];
            matched_actions[match_count].context = context;
            match_count++;
        }
        
        wp = wp->next;
    }
    
    ret = pthread_mutex_unlock(&watchpoint_mutex);
    if (ret != 0) {
        /* 锁释放失败，但仍需要执行匹配的动作 */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    /* 执行所有匹配的动作 */
    for (int i = 0; i < match_count; i++) {
        action_execute(matched_actions[i].action_id, &matched_actions[i].context);
    }
    
    return PHYMUTI_SUCCESS;
} 