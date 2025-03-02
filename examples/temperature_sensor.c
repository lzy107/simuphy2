/**
 * @file temperature_sensor.c
 * @brief 温度传感器示例程序
 */

#include "phymuti.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 温度传感器设备结构体 */
typedef struct {
    float current_temp;  /* 当前温度 */
    float min_temp;      /* 最低温度 */
    float max_temp;      /* 最高温度 */
    bool alarm_enabled;  /* 报警使能 */
} temp_sensor_data_t;

/* 温度传感器命令 */
#define TEMP_SENSOR_CMD_SET_TEMP      1  /* 设置温度 */
#define TEMP_SENSOR_CMD_SET_MIN_TEMP  2  /* 设置最低温度 */
#define TEMP_SENSOR_CMD_SET_MAX_TEMP  3  /* 设置最高温度 */
#define TEMP_SENSOR_CMD_ENABLE_ALARM  4  /* 使能报警 */
#define TEMP_SENSOR_CMD_DISABLE_ALARM 5  /* 禁用报警 */

/* 温度传感器内存区域 */
#define TEMP_SENSOR_REG_BASE    0x1000
#define TEMP_SENSOR_REG_CURRENT 0x1000  /* 当前温度寄存器 */
#define TEMP_SENSOR_REG_MIN     0x1004  /* 最低温度寄存器 */
#define TEMP_SENSOR_REG_MAX     0x1008  /* 最高温度寄存器 */
#define TEMP_SENSOR_REG_CTRL    0x100C  /* 控制寄存器 */
#define TEMP_SENSOR_REG_SIZE    16      /* 寄存器区域大小 */

/* 温度报警回调函数 */
static int temperature_alarm_callback(const monitor_context_t *context, void *user_data) {
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
    float temp = *(float*)&value;
    
    printf("温度报警: 设备 %s 的温度为 %.1f°C\n", device_name, temp);
    
    return PHYMUTI_SUCCESS;
}

/* 温度传感器创建函数 */
static int temp_sensor_create(device_handle_t device, const char *name, const device_config_t *config) {
    (void)name;
    (void)config;
    
    /* 分配设备数据 */
    temp_sensor_data_t *data = (temp_sensor_data_t *)malloc(sizeof(temp_sensor_data_t));
    if (!data) {
        return PHYMUTI_ERROR_OUT_OF_MEMORY;
    }
    
    /* 初始化设备数据 */
    data->current_temp = 25.0f;
    data->min_temp = 0.0f;
    data->max_temp = 100.0f;
    data->alarm_enabled = false;
    
    /* 设置设备用户数据 */
    device_set_user_data(device, data);
    
    return PHYMUTI_SUCCESS;
}

/* 温度传感器销毁函数 */
static void temp_sensor_destroy(device_handle_t device) {
    /* 释放设备数据 */
    temp_sensor_data_t *data = (temp_sensor_data_t *)device_get_user_data(device);
    if (data) {
        free(data);
    }
}

/* 温度传感器重置函数 */
static int temp_sensor_reset(device_handle_t device) {
    temp_sensor_data_t *data = (temp_sensor_data_t *)device_get_user_data(device);
    if (!data) {
        return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    }
    
    /* 重置设备数据 */
    data->current_temp = 25.0f;
    data->min_temp = 0.0f;
    data->max_temp = 100.0f;
    data->alarm_enabled = false;
    
    return PHYMUTI_SUCCESS;
}

/* 温度传感器保存状态函数 */
static int temp_sensor_save_state(device_handle_t device, void *buffer, size_t *size) {
    temp_sensor_data_t *data = (temp_sensor_data_t *)device_get_user_data(device);
    if (!data || !size) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查缓冲区大小 */
    if (*size < sizeof(temp_sensor_data_t)) {
        *size = sizeof(temp_sensor_data_t);
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 保存状态 */
    if (buffer) {
        memcpy(buffer, data, sizeof(temp_sensor_data_t));
    }
    
    *size = sizeof(temp_sensor_data_t);
    return PHYMUTI_SUCCESS;
}

/* 温度传感器加载状态函数 */
static int temp_sensor_load_state(device_handle_t device, const void *buffer, size_t size) {
    temp_sensor_data_t *data = (temp_sensor_data_t *)device_get_user_data(device);
    if (!data || !buffer) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 检查缓冲区大小 */
    if (size < sizeof(temp_sensor_data_t)) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    /* 加载状态 */
    memcpy(data, buffer, sizeof(temp_sensor_data_t));
    return PHYMUTI_SUCCESS;
}

/* 温度传感器特定操作函数 */
static int temp_sensor_ioctl(device_handle_t device, int cmd, void *arg) {
    temp_sensor_data_t *data = (temp_sensor_data_t *)device_get_user_data(device);
    if (!data) {
        return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    }
    
    switch (cmd) {
        case TEMP_SENSOR_CMD_SET_TEMP:
            if (!arg) {
                return PHYMUTI_ERROR_INVALID_PARAM;
            }
            data->current_temp = *(float*)arg;
            break;
            
        case TEMP_SENSOR_CMD_SET_MIN_TEMP:
            if (!arg) {
                return PHYMUTI_ERROR_INVALID_PARAM;
            }
            data->min_temp = *(float*)arg;
            break;
            
        case TEMP_SENSOR_CMD_SET_MAX_TEMP:
            if (!arg) {
                return PHYMUTI_ERROR_INVALID_PARAM;
            }
            data->max_temp = *(float*)arg;
            break;
            
        case TEMP_SENSOR_CMD_ENABLE_ALARM:
            data->alarm_enabled = true;
            break;
            
        case TEMP_SENSOR_CMD_DISABLE_ALARM:
            data->alarm_enabled = false;
            break;
            
        default:
            return PHYMUTI_ERROR_NOT_SUPPORTED;
    }
    
    return PHYMUTI_SUCCESS;
}

/* 温度传感器操作函数集 */
static device_ops_t temp_sensor_ops = {
    .create = temp_sensor_create,
    .destroy = temp_sensor_destroy,
    .reset = temp_sensor_reset,
    .save_state = temp_sensor_save_state,
    .load_state = temp_sensor_load_state,
    .ioctl = temp_sensor_ioctl
};

/* 高温规则条件函数 */
static bool high_temp_rule_condition(const monitor_context_t *context, void *user_data) {
    (void)user_data;
    
    if (!context) {
        return false;
    }
    
    uint32_t value = (uint32_t)context->value;
    float temp = *(float*)&value;
    
    /* 温度超过30度触发规则 */
    return temp > 30.0f;
}

int main(void) {
    int ret;
    
    printf("PhyMuTi温度传感器示例\n");
    
    /* 初始化PhyMuTi系统 */
    ret = phymuti_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "初始化PhyMuTi系统失败: %s\n", phymuti_error_string(ret));
        return 1;
    }
    
    /* 注册温度传感器设备类型 */
    ret = device_type_register("temperature_sensor", &temp_sensor_ops, NULL);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "注册温度传感器设备类型失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建温度传感器设备实例 */
    device_config_t config = {0};
    device_handle_t device = device_create("temperature_sensor", "room_temp", &config);
    if (!device) {
        fprintf(stderr, "创建温度传感器设备实例失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建内存区域 */
    memory_region_t *region = memory_region_create(device, "reg", TEMP_SENSOR_REG_BASE, 
                                                  TEMP_SENSOR_REG_SIZE, MEMORY_FLAG_RW);
    if (!region) {
        fprintf(stderr, "创建内存区域失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 写入初始温度 */
    float temp = 25.0f;
    uint32_t temp_value = *(uint32_t*)&temp;
    ret = memory_write_word(region, TEMP_SENSOR_REG_CURRENT, temp_value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入初始温度失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    /* 添加监视点 */
    monitor_id_t wp_id = monitor_add_watchpoint(region, TEMP_SENSOR_REG_CURRENT, 
                                              sizeof(uint32_t), WATCHPOINT_WRITE);
    if (wp_id == MONITOR_INVALID_ID) {
        fprintf(stderr, "添加监视点失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 创建温度报警动作 */
    action_id_t action_id = action_create_callback(temperature_alarm_callback, NULL);
    if (action_id == ACTION_INVALID_ID) {
        fprintf(stderr, "创建温度报警动作失败\n");
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
    
    /* 创建高温规则 */
    rule_id_t rule_id = rule_create("high_temp_rule");
    if (rule_id == RULE_INVALID_ID) {
        fprintf(stderr, "创建高温规则失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    /* 设置规则条件 */
    ret = rule_set_condition(rule_id, high_temp_rule_condition, NULL);
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
    
    printf("系统初始化完成，开始模拟温度变化...\n");
    
    /* 模拟温度变化 */
    for (int i = 0; i < 10; i++) {
        /* 温度每次增加2度 */
        temp += 2.0f;
        temp_value = *(uint32_t*)&temp;
        
        printf("设置温度为 %.1f°C\n", temp);
        
        /* 写入新温度 */
        ret = memory_write_word(region, TEMP_SENSOR_REG_CURRENT, temp_value);
        if (ret != PHYMUTI_SUCCESS) {
            fprintf(stderr, "写入温度失败: %s\n", phymuti_error_string(ret));
            break;
        }
        
        /* 处理事件 */
        ret = phymuti_process_events();
        if (ret != PHYMUTI_SUCCESS) {
            fprintf(stderr, "处理事件失败: %s\n", phymuti_error_string(ret));
            break;
        }
        
        /* 等待1秒 */
        sleep(1);
    }
    
    /* 保存设备状态 */
    size_t state_size = 0;
    ret = device_save_state(device, NULL, &state_size);
    if (ret == PHYMUTI_ERROR_INVALID_PARAM && state_size > 0) {
        void *state_buffer = malloc(state_size);
        if (state_buffer) {
            ret = device_save_state(device, state_buffer, &state_size);
            if (ret == PHYMUTI_SUCCESS) {
                printf("设备状态已保存，大小为 %zu 字节\n", state_size);
            }
            free(state_buffer);
        }
    }
    
    /* 清理PhyMuTi系统 */
    ret = phymuti_cleanup();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "清理PhyMuTi系统失败: %s\n", phymuti_error_string(ret));
        return 1;
    }
    
    printf("示例程序结束\n");
    return 0;
}