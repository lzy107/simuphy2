/**
 * @file memory_manager.c
 * @brief 内存管理器模块实现
 */

#include "memory_manager.h"
#include "phymuti_error.h"
#include "monitor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/* 内存区域结构体 */
struct memory_region_struct {
    char *name;                  /* 内存区域名称 */
    device_handle_t device;      /* 关联的设备 */
    uint64_t base_addr;          /* 基地址 */
    size_t size;                 /* 大小（字节） */
    uint32_t flags;              /* 标志 */
    uint8_t *data;               /* 内存数据 */
    struct memory_region_struct *next;  /* 下一个内存区域 */
};

/* 内存区域链表头 */
static memory_region_t *memory_region_list = NULL;

/* 内存区域链表的互斥锁 */
static pthread_mutex_t memory_region_mutex = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief 初始化内存管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int memory_manager_init(void) {
    /* 初始化内存区域链表 */
    memory_region_list = NULL;
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 清理内存管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int memory_manager_cleanup(void) {
    /* 清理所有内存区域 */
    pthread_mutex_lock(&memory_region_mutex);
    memory_region_t *region = memory_region_list;
    memory_region_t *next_region;
    
    while (region) {
        next_region = region->next;
        
        /* 释放内存区域数据 */
        if (region->data) {
            free(region->data);
        }
        
        /* 释放名称 */
        if (region->name) {
            free(region->name);
        }
        
        /* 释放内存区域结构体 */
        free(region);
        
        region = next_region;
    }
    
    memory_region_list = NULL;
    pthread_mutex_unlock(&memory_region_mutex);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 创建内存区域
 * 
 * @param device 关联的设备
 * @param name 内存区域名称
 * @param base_addr 基地址
 * @param size 大小（字节）
 * @param flags 标志
 * @return memory_region_t* 成功返回内存区域指针，失败返回NULL
 */
memory_region_t* memory_region_create(device_handle_t device, const char *name, 
                                      uint64_t base_addr, size_t size, uint32_t flags) {
    memory_region_t *region;
    
    /* 检查参数 */
    if (!name || size == 0) {
        return NULL;
    }
    
    /* 分配内存区域结构体 */
    region = (memory_region_t *)malloc(sizeof(memory_region_t));
    if (!region) {
        return NULL;
    }
    
    /* 初始化内存区域结构体 */
    region->name = strdup(name);
    if (!region->name) {
        free(region);
        return NULL;
    }
    
    region->device = device;
    region->base_addr = base_addr;
    region->size = size;
    region->flags = flags;
    
    /* 分配内存数据 */
    region->data = (uint8_t *)calloc(size, 1);
    if (!region->data) {
        free(region->name);
        free(region);
        return NULL;
    }
    
    /* 添加到内存区域链表 */
    pthread_mutex_lock(&memory_region_mutex);
    region->next = memory_region_list;
    memory_region_list = region;
    pthread_mutex_unlock(&memory_region_mutex);
    
    return region;
}

/**
 * @brief 销毁内存区域
 * 
 * @param region 内存区域指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_region_destroy(memory_region_t *region) {
    memory_region_t *prev, *curr;
    
    /* 检查参数 */
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 从内存区域链表中移除 */
    pthread_mutex_lock(&memory_region_mutex);
    
    prev = NULL;
    curr = memory_region_list;
    
    while (curr) {
        if (curr == region) {
            if (prev) {
                prev->next = curr->next;
            } else {
                memory_region_list = curr->next;
            }
            
            /* 释放内存区域数据 */
            if (region->data) {
                free(region->data);
            }
            
            /* 释放名称 */
            if (region->name) {
                free(region->name);
            }
            
            /* 释放内存区域结构体 */
            free(region);
            
            pthread_mutex_unlock(&memory_region_mutex);
            return PHYMUTI_SUCCESS;
        }
        
        prev = curr;
        curr = curr->next;
    }
    
    pthread_mutex_unlock(&memory_region_mutex);
    return PHYMUTI_ERROR_NOT_FOUND;
}

/**
 * @brief 通过名称查找内存区域
 * 
 * @param device 关联的设备（可选，如果为NULL则不匹配设备）
 * @param name 内存区域名称
 * @return memory_region_t* 成功返回内存区域指针，失败返回NULL
 */
memory_region_t* memory_region_find(device_handle_t device, const char *name) {
    memory_region_t *region;
    
    /* 检查参数 */
    if (!name) {
        return NULL;
    }
    
    /* 遍历内存区域链表 */
    pthread_mutex_lock(&memory_region_mutex);
    
    region = memory_region_list;
    while (region) {
        if ((!device || region->device == device) && 
            strcmp(region->name, name) == 0) {
            pthread_mutex_unlock(&memory_region_mutex);
            return region;
        }
        
        region = region->next;
    }
    
    pthread_mutex_unlock(&memory_region_mutex);
    return NULL;
}

/**
 * @brief 获取内存区域名称
 * 
 * @param region 内存区域指针
 * @return const char* 内存区域名称
 */
const char* memory_region_get_name(const memory_region_t *region) {
    return region ? region->name : NULL;
}

/**
 * @brief 获取内存区域基地址
 * 
 * @param region 内存区域指针
 * @return uint64_t 基地址
 */
uint64_t memory_region_get_base_addr(const memory_region_t *region) {
    return region ? region->base_addr : 0;
}

/**
 * @brief 获取内存区域大小
 * 
 * @param region 内存区域指针
 * @return size_t 大小（字节）
 */
size_t memory_region_get_size(const memory_region_t *region) {
    return region ? region->size : 0;
}

/**
 * @brief 获取内存区域标志
 * 
 * @param region 内存区域指针
 * @return uint32_t 标志
 */
uint32_t memory_region_get_flags(const memory_region_t *region) {
    return region ? region->flags : 0;
}

/**
 * @brief 获取内存区域关联的设备
 * 
 * @param region 内存区域指针
 * @return device_handle_t 设备句柄
 */
device_handle_t memory_region_get_device(const memory_region_t *region) {
    return region ? region->device : NULL;
}

/**
 * @brief 检查内存访问权限
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param size 大小（字节）
 * @param access_type 访问类型
 * @return int 成功返回0，失败返回错误码
 */
static int check_memory_access(memory_region_t *region, uint64_t addr, 
                              size_t size, memory_access_type_t access_type) {
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址范围 */
    if (addr < region->base_addr || 
        addr + size > region->base_addr + region->size) {
        return PHYMUTI_ERROR_MEMORY_OUT_OF_RANGE;
    }
    
    /* 检查访问权限 */
    switch (access_type) {
        case MEMORY_ACCESS_READ:
            if (!(region->flags & MEMORY_FLAG_READ)) {
                return PHYMUTI_ERROR_MEMORY_PERMISSION;
            }
            break;
            
        case MEMORY_ACCESS_WRITE:
            if (!(region->flags & MEMORY_FLAG_WRITE)) {
                return PHYMUTI_ERROR_MEMORY_PERMISSION;
            }
            break;
            
        case MEMORY_ACCESS_EXEC:
            if (!(region->flags & MEMORY_FLAG_EXEC)) {
                return PHYMUTI_ERROR_MEMORY_PERMISSION;
            }
            break;
            
        default:
            return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 读取内存字节
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_byte(memory_region_t *region, uint64_t addr, uint8_t *value) {
    if (!region || !value) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 1, MEMORY_ACCESS_READ);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 读取数据 */
    *value = region->data[offset];
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 1, *value, MEMORY_ACCESS_READ);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 写入内存字节
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_byte(memory_region_t *region, uint64_t addr, uint8_t value) {
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 1, MEMORY_ACCESS_WRITE);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 写入数据 */
    region->data[offset] = value;
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 1, value, MEMORY_ACCESS_WRITE);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 读取内存半字（16位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_halfword(memory_region_t *region, uint64_t addr, uint16_t *value) {
    if (!region || !value) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x1) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 2, MEMORY_ACCESS_READ);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 读取数据 */
    *value = *(uint16_t *)(region->data + offset);
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 2, *value, MEMORY_ACCESS_READ);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 写入内存半字（16位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_halfword(memory_region_t *region, uint64_t addr, uint16_t value) {
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x1) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 2, MEMORY_ACCESS_WRITE);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 写入数据 */
    *(uint16_t *)(region->data + offset) = value;
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 2, value, MEMORY_ACCESS_WRITE);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 读取内存字（32位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_word(memory_region_t *region, uint64_t addr, uint32_t *value) {
    if (!region || !value) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x3) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 4, MEMORY_ACCESS_READ);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 读取数据 */
    *value = *(uint32_t *)(region->data + offset);
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 4, *value, MEMORY_ACCESS_READ);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 写入内存字（32位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_word(memory_region_t *region, uint64_t addr, uint32_t value) {
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x3) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 4, MEMORY_ACCESS_WRITE);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 写入数据 */
    *(uint32_t *)(region->data + offset) = value;
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 4, value, MEMORY_ACCESS_WRITE);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 读取内存双字（64位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值指针
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_doubleword(memory_region_t *region, uint64_t addr, uint64_t *value) {
    if (!region || !value) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x7) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 8, MEMORY_ACCESS_READ);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 读取数据 */
    *value = *(uint64_t *)(region->data + offset);
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 8, *value, MEMORY_ACCESS_READ);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 写入内存双字（64位）
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param value 值
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_doubleword(memory_region_t *region, uint64_t addr, uint64_t value) {
    if (!region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查地址对齐 */
    if (addr & 0x7) {
        return PHYMUTI_ERROR_MEMORY_ALIGNMENT;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, 8, MEMORY_ACCESS_WRITE);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 写入数据 */
    *(uint64_t *)(region->data + offset) = value;
    
    /* 通知监视器 */
    monitor_notify_memory_access(region, addr, 8, value, MEMORY_ACCESS_WRITE);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 读取内存块
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param buffer 缓冲区
 * @param size 大小（字节）
 * @return int 成功返回0，失败返回错误码
 */
int memory_read_buffer(memory_region_t *region, uint64_t addr, void *buffer, size_t size) {
    if (!region || !buffer || size == 0) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, size, MEMORY_ACCESS_READ);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 读取数据 */
    memcpy(buffer, region->data + offset, size);
    
    /* 通知监视器（简化处理，只通知一次） */
    monitor_notify_memory_access(region, addr, size, 0, MEMORY_ACCESS_READ);
    
    return PHYMUTI_SUCCESS;
}

/**
 * @brief 写入内存块
 * 
 * @param region 内存区域指针
 * @param addr 地址
 * @param buffer 缓冲区
 * @param size 大小（字节）
 * @return int 成功返回0，失败返回错误码
 */
int memory_write_buffer(memory_region_t *region, uint64_t addr, const void *buffer, size_t size) {
    if (!region || !buffer || size == 0) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查访问权限 */
    int ret = check_memory_access(region, addr, size, MEMORY_ACCESS_WRITE);
    if (ret != PHYMUTI_SUCCESS) {
        return ret;
    }
    
    /* 计算偏移量 */
    size_t offset = addr - region->base_addr;
    
    /* 写入数据 */
    memcpy(region->data + offset, buffer, size);
    
    /* 通知监视器（简化处理，只通知一次） */
    monitor_notify_memory_access(region, addr, size, 0, MEMORY_ACCESS_WRITE);
    
    return PHYMUTI_SUCCESS;
} 