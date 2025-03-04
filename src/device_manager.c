/**
 * @file device_manager.c
 * @brief 设备管理器模块实现
 */

#include "device_manager.h"
#include "phymuti_error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 设备类型结构体 */
typedef struct device_type_struct {
    char *name;                  /* 设备类型名称 */
    device_ops_t ops;            /* 设备操作函数集 */
    void *user_data;             /* 用户自定义数据 */
    struct device_type_struct *next;  /* 下一个设备类型 */
} device_type_t;

/* 设备结构体 */
struct device_struct {
    char *name;                  /* 设备名称 */
    device_type_t *type;         /* 设备类型 */
    void *user_data;             /* 用户自定义数据 */
    struct device_struct *next;  /* 下一个设备 */
};

/* 设备类型链表头 */
static device_type_t *device_type_list = NULL;

/* 设备链表头 */
static device_handle_t device_list = NULL;

/* 设备和设备类型链表的递归互斥锁 */
static pthread_mutex_t device_mutex;
static pthread_mutexattr_t device_mutex_attr;

/**
 * @brief 初始化设备管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int device_manager_init(void) {
    int ret;
    
    /* 初始化递归互斥锁 */
    ret = pthread_mutexattr_init(&device_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutexattr_settype(&device_mutex_attr, PTHREAD_MUTEX_RECURSIVE);
    if (ret != 0) {
        pthread_mutexattr_destroy(&device_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    ret = pthread_mutex_init(&device_mutex, &device_mutex_attr);
    if (ret != 0) {
        pthread_mutexattr_destroy(&device_mutex_attr);
        return PHYMUTI_ERROR_MUTEX_INIT_FAILED;
    }
    
    /* 初始化设备类型链表和设备链表 */
    device_type_list = NULL;
    device_list = NULL;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理设备管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int device_manager_cleanup(void) {
    device_type_t *type, *next_type;
    device_handle_t device, next_device;
    int ret;
    
    /* 清理所有设备 */
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    device = device_list;
    while (device) {
        next_device = device->next;
        
        /* 释放设备名称 */
        if (device->name) {
            free(device->name);
        }
        
        /* 释放设备结构体 */
        free(device);
        
        device = next_device;
    }
    
    /* 清理所有设备类型 */
    type = device_type_list;
    while (type) {
        next_type = type->next;
        
        /* 释放设备类型名称 */
        if (type->name) {
            free(type->name);
        }
        
        /* 释放设备类型结构体 */
        free(type);
        
        type = next_type;
    }
    
    device_list = NULL;
    device_type_list = NULL;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    /* 销毁互斥锁 */
    ret = pthread_mutex_destroy(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    ret = pthread_mutexattr_destroy(&device_mutex_attr);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_DESTROY_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 注册设备类型
 * 
 * @param type_name 设备类型名称
 * @param ops 设备操作函数集
 * @param user_data 用户自定义数据
 * @return int 成功返回0，失败返回错误码
 */
int device_type_register(const char *type_name, const device_ops_t *ops, void *user_data) {
    device_type_t *type;
    int ret;
    
    /* 检查参数 */
    if (!type_name || !ops) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查是否已注册 */
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    type = device_type_list;
    while (type) {
        if (strcmp(type->name, type_name) == 0) {
            ret = pthread_mutex_unlock(&device_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            return PHYMUTI_ERROR_ALREADY_EXISTS;
        }
        type = type->next;
    }
    
    /* 创建新的设备类型 */
    type = (device_type_t *)malloc(sizeof(device_type_t));
    if (!type) {
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_OUT_OF_MEMORY;
    }
    
    type->name = strdup(type_name);
    if (!type->name) {
        free(type);
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        return PHYMUTI_ERROR_OUT_OF_MEMORY;
    }
    
    type->ops = *ops;
    type->user_data = user_data;
    
    /* 添加到设备类型链表 */
    type->next = device_type_list;
    device_type_list = type;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 注销设备类型
 * 
 * @param type_name 设备类型名称
 * @return int 成功返回0，失败返回错误码
 */
int device_type_unregister(const char *type_name) {
    int ret;
    
    if (!type_name) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    device_type_t *prev = NULL;
    device_type_t *type = device_type_list;
    
    /* 查找设备类型 */
    while (type) {
        if (strcmp(type->name, type_name) == 0) {
            /* 检查是否有该类型的设备实例 */
            device_handle_t device = device_list;
            while (device) {
                if (device->type == type) {
                    ret = pthread_mutex_unlock(&device_mutex);
                    if (ret != 0) {
                        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
                    }
                    return PHYMUTI_ERROR_BUSY;  /* 有设备实例，不能注销 */
                }
                device = device->next;
            }
            
            /* 从链表中移除 */
            if (prev) {
                prev->next = type->next;
            } else {
                device_type_list = type->next;
            }
            
            /* 释放资源 */
            free(type->name);
            free(type);
            
            ret = pthread_mutex_unlock(&device_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            return PHYMUTI_SUCCESS;
        }
        
        prev = type;
        type = type->next;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    return PHYMUTI_ERROR_DEVICE_TYPE_NOT_FOUND;
}

/**
 * @brief 查找设备类型（不解锁版本，供内部使用）
 * 
 * @param type_name 设备类型名称
 * @return device_type_t* 成功返回设备类型指针，失败返回NULL
 */
static device_type_t* find_device_type_locked(const char *type_name) {
    device_type_t *type = device_type_list;
    
    while (type) {
        if (strcmp(type->name, type_name) == 0) {
            return type;
        }
        type = type->next;
    }
    
    return NULL;
}

/**
 * @brief 查找设备类型
 * 
 * @param type_name 设备类型名称
 * @return device_type_t* 成功返回设备类型指针，失败返回NULL
 */
static device_type_t* find_device_type(const char *type_name) {
    device_type_t *type;
    int ret;
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        /* 无法处理错误，直接返回NULL */
        return NULL;
    }
    
    type = find_device_type_locked(type_name);
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，记录错误或日志 */
        /* 这里我们无法返回错误码，仅返回找到的结果 */
    }
    
    return type;
}

/**
 * @brief 查找设备（不解锁版本，供内部使用）
 * 
 * @param name 设备名称
 * @return device_handle_t 成功返回设备句柄，失败返回NULL
 */
static device_handle_t find_device_by_name_locked(const char *name) {
    device_handle_t device = device_list;
    
    while (device) {
        if (strcmp(device->name, name) == 0) {
            return device;
        }
        device = device->next;
    }
    
    return NULL;
}

/**
 * @brief 创建设备实例
 * 
 * @param type_name 设备类型名称
 * @param name 设备实例名称
 * @param config 设备配置
 * @return device_handle_t 成功返回设备句柄，失败返回NULL
 */
device_handle_t device_create(const char *type_name, const char *name, const device_config_t *config) {
    int ret;
    
    if (!type_name || !name) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    /* 查找设备类型 */
    device_type_t *type = find_device_type_locked(type_name);
    if (!type) {
        pthread_mutex_unlock(&device_mutex);
        return NULL;
    }
    
    /* 检查设备名称是否已存在 */
    device_handle_t existing_device = find_device_by_name_locked(name);
    if (existing_device) {
        pthread_mutex_unlock(&device_mutex);
        return NULL;  /* 设备名称已存在 */
    }
    
    /* 创建新的设备实例 */
    device_handle_t device = (device_handle_t)malloc(sizeof(struct device_struct));
    if (!device) {
        pthread_mutex_unlock(&device_mutex);
        return NULL;
    }
    
    device->name = strdup(name);
    if (!device->name) {
        free(device);
        pthread_mutex_unlock(&device_mutex);
        return NULL;
    }
    
    device->type = type;
    device->user_data = config ? config->user_data : NULL;
    
    /* 调用设备类型的create函数 */
    if (type->ops.create) {
        /* 临时解锁以避免在回调中发生死锁 */
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            free(device->name);
            free(device);
            return NULL;
        }
        
        int create_ret = type->ops.create(device, name, config);
        
        ret = pthread_mutex_lock(&device_mutex);
        if (ret != 0) {
            /* 锁失败，需要清理资源 */
            if (create_ret == PHYMUTI_SUCCESS && type->ops.destroy) {
                type->ops.destroy(device);
            }
            free(device->name);
            free(device);
            return NULL;
        }
        
        if (create_ret != PHYMUTI_SUCCESS) {
            free(device->name);
            free(device);
            pthread_mutex_unlock(&device_mutex);
            return NULL;
        }
    }
    
    /* 添加到设备链表 */
    device->next = device_list;
    device_list = device;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，但设备已创建成功，这里直接返回设备 */
        /* 在实际应用中可以考虑记录错误日志 */
    }
    
    return device;
}

/**
 * @brief 销毁设备实例
 * 
 * @param device 设备句柄
 * @return int 成功返回0，失败返回错误码
 */
int device_destroy(device_handle_t device) {
    int ret;
    
    if (!device) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    /* 从链表中移除 */
    device_handle_t prev = NULL;
    device_handle_t curr = device_list;
    
    while (curr) {
        if (curr == device) {
            if (prev) {
                prev->next = curr->next;
            } else {
                device_list = curr->next;
            }
            
            /* 调用设备类型的destroy函数 */
            if (device->type && device->type->ops.destroy) {
                /* 临时解锁以避免在回调中发生死锁 */
                ret = pthread_mutex_unlock(&device_mutex);
                if (ret != 0) {
                    /* 锁释放失败，但仍需要继续销毁设备 */
                    /* 记录错误日志 */
                }
                
                device->type->ops.destroy(device);
                
                ret = pthread_mutex_lock(&device_mutex);
                if (ret != 0) {
                    /* 锁获取失败，设备已被销毁但无法更新链表状态 */
                    /* 这是一个严重的错误，可能导致内存泄漏或状态不一致 */
                    /* 在实际应用中应该记录错误日志 */
                    free(device->name);
                    free(device);
                    return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
                }
            }
            
            /* 释放资源 */
            free(device->name);
            free(device);
            
            ret = pthread_mutex_unlock(&device_mutex);
            if (ret != 0) {
                return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
            }
            
            return PHYMUTI_SUCCESS;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
}

/**
 * @brief 根据名称查找设备
 * 
 * @param name 设备名称
 * @return device_handle_t 成功返回设备句柄，失败返回NULL
 */
device_handle_t device_find_by_name(const char *name) {
    device_handle_t device;
    int ret;
    
    if (!name) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    device = find_device_by_name_locked(name);
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，记录错误或日志 */
        /* 这里我们无法返回错误码，仅返回找到的结果 */
    }
    
    return device;
}

/**
 * @brief 重置设备
 * 
 * @param device 设备句柄
 * @return int 成功返回0，失败返回错误码
 */
int device_reset(device_handle_t device) {
    int ret;
    
    if (!device) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    if (device->type && device->type->ops.reset) {
        /* 临时解锁以避免在回调中发生死锁 */
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        int reset_ret = device->type->ops.reset(device);
        
        ret = pthread_mutex_lock(&device_mutex);
        if (ret != 0) {
            /* 锁获取失败，但设备重置操作已完成 */
            return reset_ret; /* 返回重置结果 */
        }
        
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        return reset_ret;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 保存设备状态
 * 
 * @param device 设备句柄
 * @param buffer 状态缓冲区
 * @param size 缓冲区大小
 * @return int 成功返回0，失败返回错误码
 */
int device_save_state(device_handle_t device, void *buffer, size_t *size) {
    int ret;
    
    if (!device || !size) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    if (device->type && device->type->ops.save_state) {
        /* 临时解锁以避免在回调中发生死锁 */
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        int save_ret = device->type->ops.save_state(device, buffer, size);
        
        ret = pthread_mutex_lock(&device_mutex);
        if (ret != 0) {
            /* 锁获取失败，但保存状态操作已完成 */
            return save_ret; /* 返回保存结果 */
        }
        
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        return save_ret;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_NOT_SUPPORTED;
}

/**
 * @brief 加载设备状态
 * 
 * @param device 设备句柄
 * @param buffer 状态缓冲区
 * @param size 缓冲区大小
 * @return int 成功返回0，失败返回错误码
 */
int device_load_state(device_handle_t device, const void *buffer, size_t size) {
    int ret;
    
    if (!device || !buffer) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    if (device->type && device->type->ops.load_state) {
        /* 临时解锁以避免在回调中发生死锁 */
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        int load_ret = device->type->ops.load_state(device, buffer, size);
        
        ret = pthread_mutex_lock(&device_mutex);
        if (ret != 0) {
            /* 锁获取失败，但加载状态操作已完成 */
            return load_ret; /* 返回加载结果 */
        }
        
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        return load_ret;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_NOT_SUPPORTED;
}

/**
 * @brief 设备特定操作
 * 
 * @param device 设备句柄
 * @param cmd 命令
 * @param arg 参数
 * @return int 成功返回0，失败返回错误码
 */
int device_ioctl(device_handle_t device, int cmd, void *arg) {
    int ret;
    
    if (!device) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    if (device->type && device->type->ops.ioctl) {
        /* 临时解锁以避免在回调中发生死锁 */
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        int ioctl_ret = device->type->ops.ioctl(device, cmd, arg);
        
        ret = pthread_mutex_lock(&device_mutex);
        if (ret != 0) {
            /* 锁获取失败，但ioctl操作已完成 */
            return ioctl_ret; /* 返回ioctl结果 */
        }
        
        ret = pthread_mutex_unlock(&device_mutex);
        if (ret != 0) {
            return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
        }
        
        return ioctl_ret;
    }
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_ERROR_NOT_SUPPORTED;
}

/**
 * @brief 获取设备名称
 * 
 * @param device 设备句柄
 * @return const char* 设备名称
 */
const char* device_get_name(device_handle_t device) {
    int ret;
    const char *name;
    
    if (!device) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    name = device->name;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，记录错误或日志 */
    }
    
    return name;
}

/**
 * @brief 获取设备类型名称
 * 
 * @param device 设备句柄
 * @return const char* 设备类型名称
 */
const char* device_get_type_name(device_handle_t device) {
    int ret;
    const char *type_name;
    
    if (!device || !device->type) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    type_name = device->type->name;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，记录错误或日志 */
    }
    
    return type_name;
}

/**
 * @brief 获取设备用户数据
 * 
 * @param device 设备句柄
 * @return void* 用户数据指针
 */
void* device_get_user_data(device_handle_t device) {
    int ret;
    void *user_data;
    
    if (!device) {
        return NULL;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return NULL;
    }
    
    user_data = device->user_data;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        /* 锁释放失败，记录错误或日志 */
    }
    
    return user_data;
}

/**
 * @brief 设置设备用户数据
 * 
 * @param device 设备句柄
 * @param user_data 用户数据指针
 * @return int 成功返回0，失败返回错误码
 */
int device_set_user_data(device_handle_t device, void *user_data) {
    int ret;
    
    if (!device) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    ret = pthread_mutex_lock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_LOCK_FAILED;
    }
    
    device->user_data = user_data;
    
    ret = pthread_mutex_unlock(&device_mutex);
    if (ret != 0) {
        return PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED;
    }
    
    return PHYMUTI_SUCCESS;
} 