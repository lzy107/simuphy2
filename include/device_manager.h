/**
 * @file device_manager.h
 * @brief 设备管理器模块头文件
 */

#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* 设备句柄类型 */
typedef struct device_struct* device_handle_t;

/* 设备配置结构体 */
typedef struct {
    void *user_data;  /* 用户自定义数据 */
    /* 可以根据需要添加更多配置项 */
} device_config_t;

/* 设备操作函数集 */
typedef struct {
    /* 创建设备实例 */
    int (*create)(device_handle_t device, const char *name, const device_config_t *config);
    
    /* 销毁设备实例 */
    void (*destroy)(device_handle_t device);
    
    /* 重置设备 */
    int (*reset)(device_handle_t device);
    
    /* 保存设备状态 */
    int (*save_state)(device_handle_t device, void *buffer, size_t *size);
    
    /* 加载设备状态 */
    int (*load_state)(device_handle_t device, const void *buffer, size_t size);
    
    /* 设备特定操作 */
    int (*ioctl)(device_handle_t device, int cmd, void *arg);
} device_ops_t;

/**
 * @brief 初始化设备管理器
 * 
 * @return int 成功返回0，失败返回错误码
 */
int device_manager_init(void);

/**
 * @brief 清理设备管理器资源
 * 
 * @return int 成功返回0，失败返回错误码
 */
int device_manager_cleanup(void);

/**
 * @brief 注册设备类型
 * 
 * @param type_name 设备类型名称
 * @param ops 设备操作函数集
 * @param user_data 用户自定义数据
 * @return int 成功返回0，失败返回错误码
 */
int device_type_register(const char *type_name, const device_ops_t *ops, void *user_data);

/**
 * @brief 注销设备类型
 * 
 * @param type_name 设备类型名称
 * @return int 成功返回0，失败返回错误码
 */
int device_type_unregister(const char *type_name);

/**
 * @brief 创建设备实例
 * 
 * @param type_name 设备类型名称
 * @param name 设备实例名称
 * @param config 设备配置
 * @return device_handle_t 成功返回设备句柄，失败返回NULL
 */
device_handle_t device_create(const char *type_name, const char *name, const device_config_t *config);

/**
 * @brief 销毁设备实例
 * 
 * @param device 设备句柄
 * @return int 成功返回0，失败返回错误码
 */
int device_destroy(device_handle_t device);

/**
 * @brief 根据名称查找设备
 * 
 * @param name 设备名称
 * @return device_handle_t 成功返回设备句柄，失败返回NULL
 */
device_handle_t device_find_by_name(const char *name);

/**
 * @brief 重置设备
 * 
 * @param device 设备句柄
 * @return int 成功返回0，失败返回错误码
 */
int device_reset(device_handle_t device);

/**
 * @brief 保存设备状态
 * 
 * @param device 设备句柄
 * @param buffer 状态缓冲区
 * @param size 缓冲区大小
 * @return int 成功返回0，失败返回错误码
 */
int device_save_state(device_handle_t device, void *buffer, size_t *size);

/**
 * @brief 加载设备状态
 * 
 * @param device 设备句柄
 * @param buffer 状态缓冲区
 * @param size 缓冲区大小
 * @return int 成功返回0，失败返回错误码
 */
int device_load_state(device_handle_t device, const void *buffer, size_t size);

/**
 * @brief 设备特定操作
 * 
 * @param device 设备句柄
 * @param cmd 命令
 * @param arg 参数
 * @return int 成功返回0，失败返回错误码
 */
int device_ioctl(device_handle_t device, int cmd, void *arg);

/**
 * @brief 获取设备名称
 * 
 * @param device 设备句柄
 * @return const char* 设备名称
 */
const char* device_get_name(device_handle_t device);

/**
 * @brief 获取设备类型名称
 * 
 * @param device 设备句柄
 * @return const char* 设备类型名称
 */
const char* device_get_type_name(device_handle_t device);

/**
 * @brief 获取设备用户数据
 * 
 * @param device 设备句柄
 * @return void* 用户数据指针
 */
void* device_get_user_data(device_handle_t device);

/**
 * @brief 设置设备用户数据
 * 
 * @param device 设备句柄
 * @param user_data 用户数据指针
 * @return int 成功返回0，失败返回错误码
 */
int device_set_user_data(device_handle_t device, void *user_data);

#endif /* DEVICE_MANAGER_H */ 