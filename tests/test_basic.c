/**
 * @file test_basic.c
 * @brief PhyMuTi基本功能测试程序
 */

#include "phymuti.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 测试设备结构体 */
typedef struct {
    int value;  /* 设备值 */
} test_device_data_t;

/* 测试设备创建函数 */
static int test_device_create(device_handle_t device, const char *name, const device_config_t *config) {
    (void)name;
    (void)config;
    
    /* 分配设备数据 */
    test_device_data_t *data = (test_device_data_t *)malloc(sizeof(test_device_data_t));
    if (!data) {
        return PHYMUTI_ERROR_OUT_OF_MEMORY;
    }
    
    /* 初始化设备数据 */
    data->value = 0;
    
    /* 设置设备用户数据 */
    device_set_user_data(device, data);
    
    printf("测试设备创建成功\n");
    return PHYMUTI_SUCCESS;
}

/* 测试设备销毁函数 */
static void test_device_destroy(device_handle_t device) {
    /* 释放设备数据 */
    test_device_data_t *data = (test_device_data_t *)device_get_user_data(device);
    if (data) {
        free(data);
    }
    
    printf("测试设备销毁成功\n");
}

/* 测试设备重置函数 */
static int test_device_reset(device_handle_t device) {
    test_device_data_t *data = (test_device_data_t *)device_get_user_data(device);
    if (!data) {
        return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    }
    
    /* 重置设备数据 */
    data->value = 0;
    
    printf("测试设备重置成功\n");
    return PHYMUTI_SUCCESS;
}

/* 测试设备操作函数集 */
static device_ops_t test_device_ops = {
    .create = test_device_create,
    .destroy = test_device_destroy,
    .reset = test_device_reset,
    .save_state = NULL,
    .load_state = NULL,
    .ioctl = NULL
};

/* 测试监视点回调函数 */
static int test_watchpoint_callback(const monitor_context_t *context, void *user_data) {
    (void)user_data;
    
    if (!context || !context->region) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    device_handle_t device = memory_region_get_device(context->region);
    if (!device) {
        return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    }
    
    const char *device_name = device_get_name(device);
    uint32_t value = (uint32_t)context->value;
    
    printf("监视点触发: 设备 %s 的值变为 %u\n", device_name, value);
    
    return PHYMUTI_SUCCESS;
}

/* 测试规则条件函数 */
static bool test_rule_condition(const monitor_context_t *context, void *user_data) {
    (void)user_data;
    
    if (!context) {
        return false;
    }
    
    uint32_t value = (uint32_t)context->value;
    
    /* 值大于10触发规则 */
    return value > 10;
}

int main(void) {
    int ret;
    
    printf("PhyMuTi基本功能测试\n");
    
    /* 初始化PhyMuTi系统 */
    ret = phymuti_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "初始化PhyMuTi系统失败: %s\n", phymuti_error_string(ret));
        return 1;
    }
    
    /* 注册测试设备类型 */
    ret = device_type_register("test_device", &test_device_ops, NULL);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "注册测试设备类型失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建测试设备实例 */
    device_config_t config = {0};
    device_handle_t device = device_create("test_device", "test1", &config);
    if (!device) {
        fprintf(stderr, "创建测试设备实例失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建内存区域 */
    memory_region_t *region = memory_region_create(device, "reg", 0x1000, 16, MEMORY_FLAG_RW);
    if (!region) {
        fprintf(stderr, "创建内存区域失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 添加监视点 */
    monitor_id_t wp_id = monitor_add_watchpoint(region, 0x1000, 4, WATCHPOINT_WRITE);
    if (wp_id == MONITOR_INVALID_ID) {
        fprintf(stderr, "添加监视点失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建动作 */
    action_id_t action_id = action_create_callback(test_watchpoint_callback, NULL);
    if (action_id == ACTION_INVALID_ID) {
        fprintf(stderr, "创建动作失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 绑定动作到监视点 */
    ret = monitor_bind_action(wp_id, action_id);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "绑定动作到监视点失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建规则 */
    rule_id_t rule_id = rule_create("test_rule");
    if (rule_id == RULE_INVALID_ID) {
        fprintf(stderr, "创建规则失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 设置规则条件 */
    ret = rule_set_condition(rule_id, test_rule_condition, NULL);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "设置规则条件失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 添加规则动作 */
    ret = rule_add_action(rule_id, action_id);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "添加规则动作失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 启用规则 */
    ret = rule_enable(rule_id);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "启用规则失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    printf("系统初始化完成，开始测试...\n");
    
    /* 测试内存读写 */
    uint32_t value = 0;
    
    /* 写入值5 */
    printf("写入值5\n");
    ret = memory_write_word(region, 0x1000, 5);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 处理事件 */
    ret = phymuti_process_events();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "处理事件失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 读取值 */
    ret = memory_read_word(region, 0x1000, &value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "读取内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    printf("读取值: %u\n", value);
    
    /* 写入值15，触发规则 */
    printf("写入值15\n");
    ret = memory_write_word(region, 0x1000, 15);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 处理事件 */
    ret = phymuti_process_events();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "处理事件失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 读取值 */
    ret = memory_read_word(region, 0x1000, &value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "读取内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    printf("读取值: %u\n", value);
    
    /* 测试禁用监视点 */
    printf("禁用监视点\n");
    ret = monitor_disable_watchpoint(wp_id);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "禁用监视点失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 写入值20，不应触发监视点 */
    printf("写入值20\n");
    ret = memory_write_word(region, 0x1000, 20);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 处理事件 */
    ret = phymuti_process_events();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "处理事件失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 读取值 */
    ret = memory_read_word(region, 0x1000, &value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "读取内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    printf("读取值: %u\n", value);
    
    /* 测试启用监视点 */
    printf("启用监视点\n");
    ret = monitor_enable_watchpoint(wp_id);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "启用监视点失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 写入值25，应触发监视点 */
    printf("写入值25\n");
    ret = memory_write_word(region, 0x1000, 25);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 处理事件 */
    ret = phymuti_process_events();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "处理事件失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 清理PhyMuTi系统 */
    ret = phymuti_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "清理PhyMuTi系统失败: %s\n", phymuti_error_string(ret));
        return 1;
    }
    
    printf("测试完成\n");
    return 0;
} 