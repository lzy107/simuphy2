# PhyMuTi - 物理设备模拟工具包

PhyMuTi (Physical Muti-device Toolkit) 是一个用于模拟物理设备的C语言工具包，提供了设备管理、内存管理、监视器、动作管理和规则引擎等功能。

## 功能特点

- **设备管理**：注册设备类型，创建设备实例，管理设备生命周期
- **内存管理**：创建和管理内存区域，支持读写操作
- **监视器**：设置监视点，监控内存区域变化
- **动作管理**：创建和执行动作，响应监视点触发
- **规则引擎**：创建规则，设置条件，绑定动作

## 项目结构

```
phymuti/
├── include/         # 头文件
├── src/             # 源代码
├── examples/        # 示例程序
├── tests/           # 测试程序
├── build/           # 构建输出目录
├── Makefile         # 构建脚本
└── README.md        # 项目说明
```

## 构建与使用

### 构建库

```bash
make
```

### 构建并运行示例

```bash
make examples
./build/examples/temperature_sensor
```

### 运行测试

```bash
make run_tests
```

## 使用示例

```c
#include "phymuti.h"

int main() {
    // 初始化系统
    phymuti_init();
    
    // 注册设备类型
    device_type_register("temperature_sensor", &temp_sensor_ops, NULL);
    
    // 创建设备实例
    device_config_t config = {0};
    device_handle_t device = device_create("temperature_sensor", "room_temp", &config);
    
    // 创建内存区域
    memory_region_t *region = memory_region_create(device, "reg", 0x1000, 256, MEMORY_FLAG_RW);
    
    // 写入内存
    memory_write_word(region, 0x1000, 25);
    
    // 添加监视点
    monitor_id_t wp_id = monitor_add_watchpoint(region, 0x1000, 2, WATCHPOINT_WRITE, 0);
    
    // 添加特定值监视点（监视写入值为30的操作）
    monitor_id_t value_wp_id = monitor_add_watchpoint(region, 0x1000, 2, WATCHPOINT_VALUE_WRITE, 30);
    
    // 创建动作
    action_id_t action_id = action_create_callback(temperature_alarm_callback, NULL);
    
    // 绑定动作到监视点
    monitor_bind_action(wp_id, action_id);
    
    // 处理事件
    phymuti_process_events();
    
    // 清理系统
    phymuti_cleanup();
    
    return 0;
}
```


SimuPhy2 模拟器使用指南
1. 系统架构概述
SimuPhy2 是一个灵活的硬件模拟器框架，采用分层架构设计，主要由以下几个核心组件构成：

设备类型管理：定义各种设备的类型和行为
设备实例管理：创建和管理具体的设备实例
内存管理：创建和管理内存区域，处理内存读写操作
监视器：监控内存区域的访问活动
动作管理：定义和执行当监视点触发时的行为
这些组件共同工作，使用户能够模拟各种硬件系统，追踪内存访问，并执行自定义操作。

2. 核心概念和关系
2.1 三层组织结构
系统使用三个链表组织不同层次的组件：

设备类型链表 (device_type_list)：
定义设备的行为和特性
相当于设备的"类"或"模板"

设备实例链表 (device_list)：
基于设备类型创建的具体实例
每个设备实例关联到一个特定的设备类型

内存区域链表 (memory_region_list)：
由设备实例创建和管理的内存区域
一个设备可以关联多个内存区域，各自具有不同的特性

2.2 层次关系示意图
CopyInsert
设备类型层 (device_type_list)
    │
    ├──► 设备类型1 (例如: "CPU")
    │     └──► 操作函数集 (创建、销毁、复位等)
    │
    └──► 设备类型2 (例如: "内存控制器")
          └──► 操作函数集
                 │
                 ▼
设备实例层 (device_list)
    │
    ├──► 设备实例1 (例如: "CPU0")
    │     └──► 关联到 "CPU" 类型
    │
    └──► 设备实例2 (例如: "主内存控制器")
          └──► 关联到 "内存控制器" 类型
                 │
                 ▼
内存区域层 (memory_region_list)
    │
    ├──► 内存区域1 (例如: "寄存器组")
    │     ├──► 关联到 "CPU0" 设备
    │     ├──► 基址: 0x0000
    │     └──► 大小: 1KB
    │
    └──► 内存区域2 (例如: "RAM")
          ├──► 关联到 "主内存控制器" 设备
          ├──► 基址: 0x10000
          └──► 大小: 64MB
3. 基本使用流程
3.1 系统初始化
c
CopyInsert
// 初始化系统各组件
phymuti_init();
3.2 设备类型注册
c
CopyInsert
// 定义设备操作函数集
device_ops_t cpu_ops = {
    .create = cpu_create,
    .destroy = cpu_destroy,
    .reset = cpu_reset,
    .save_state = cpu_save_state,
    .load_state = cpu_load_state,
    .ioctl = cpu_ioctl
};

// 注册设备类型
int ret = device_type_register("cpu", &cpu_ops, NULL);
if (ret != PHYMUTI_SUCCESS) {
    fprintf(stderr, "注册CPU设备类型失败: %s\n", phymuti_error_string(ret));
    return 1;
}
3.3 创建设备实例
c
CopyInsert
// 创建设备配置
device_config_t config = {
    .user_data = NULL
    // 可添加其他配置参数
};

// 创建设备实例
device_handle_t cpu = device_create("cpu", "cpu0", &config);
if (!cpu) {
    fprintf(stderr, "创建CPU设备实例失败\n");
    return 1;
}
3.4 创建内存区域
c
CopyInsert
// 为设备创建内存区域
memory_region_t *reg_region = memory_region_create(
    cpu,           // 关联的设备
    "registers",   // 区域名称
    0x0,           // 基地址
    1024,          // 大小 (1KB)
    MEMORY_FLAG_RW // 权限 (可读写)
);

if (!reg_region) {
    fprintf(stderr, "创建寄存器内存区域失败\n");
    return 1;
}

// 同一设备可以创建多个内存区域
memory_region_t *cache_region = memory_region_create(
    cpu,               // 同一设备
    "cache",           // 不同名称
    0x1000,            // 不同基地址
    4096,              // 大小 (4KB)
    MEMORY_FLAG_RWX    // 权限 (可读写执行)
);
3.5 内存访问操作
c
CopyInsert
// 写入内存
uint32_t value = 0x12345678;
int ret = memory_write_word(reg_region, 0x0, value);
if (ret != PHYMUTI_SUCCESS) {
    fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
    return 1;
}

// 读取内存
uint32_t read_value;
ret = memory_read_word(reg_region, 0x0, &read_value);
if (ret != PHYMUTI_SUCCESS) {
    fprintf(stderr, "读取内存失败: %s\n", phymuti_error_string(ret));
    return 1;
}

printf("读取到的值: 0x%08x\n", read_value);
4. 监视器和动作触发系统
监视器和动作系统是 SimuPhy2 的核心特性，它们允许用户监控内存访问并执行自定义操作。

4.1 创建监视点
c
CopyInsert
// 创建写入监视点
monitor_id_t write_wp = monitor_add_watchpoint(
    reg_region,         // 监视的内存区域
    0x0,                // 起始地址
    4,                  // 大小 (4字节)
    WATCHPOINT_WRITE,   // 监视类型 (写入)
    0                   // 监视值 (0表示监视任何值)
);

if (write_wp == MONITOR_INVALID_ID) {
    fprintf(stderr, "创建写入监视点失败\n");
    return 1;
}

// 创建特定值监视点 (只监视值为0x12345678的写入)
monitor_id_t value_wp = monitor_add_watchpoint(
    reg_region,            // 监视的内存区域
    0x0,                   // 起始地址
    4,                     // 大小 (4字节)
    WATCHPOINT_VALUE_WRITE,// 监视类型 (特定值写入)
    0x12345678            // 监视值
);
4.2 定义动作回调函数
c
CopyInsert
// 监视点触发的回调函数
static int watchpoint_callback(const monitor_context_t *context, void *user_data) {
    if (!context) {
        return PHYMUTI_ERROR_INVALID_PARAM;
    }
    
    // 获取内存区域关联的设备
    device_handle_t device = memory_region_get_device(context->region);
    const char *device_name = device_get_name(device);
    
    printf("监视点触发: 设备 %s 的内存地址 0x%llx 被写入值 0x%llx\n", 
           device_name, 
           (unsigned long long)context->address, 
           (unsigned long long)context->value);
    
    return PHYMUTI_SUCCESS;
}
4.3 创建并绑定动作
c
CopyInsert
// 创建回调函数动作
action_id_t action_id = action_create_callback(watchpoint_callback, NULL);
if (action_id == ACTION_INVALID_ID) {
    fprintf(stderr, "创建动作失败\n");
    return 1;
}

// 将动作绑定到监视点
int ret = monitor_bind_action(write_wp, action_id);
if (ret != PHYMUTI_SUCCESS) {
    fprintf(stderr, "绑定动作到监视点失败: %s\n", phymuti_error_string(ret));
    return 1;
}
4.4 其他类型的动作
除了回调函数外，SimuPhy2 还支持其他类型的动作：

c
CopyInsert
// 创建脚本动作
action_id_t script_action = action_create_script("/path/to/script.sh");

// 创建命令动作
action_id_t cmd_action = action_create_command("echo 'Memory accessed!' >> log.txt");
5. 高级特性：跨设备操作
SimuPhy2 允许一个设备的内存访问触发对另一个设备内存的操作，实现设备间的交互。

5.1 定义跨设备动作数据结构
c
CopyInsert
// 定义跨设备操作的数据结构
typedef struct {
    char target_device_name[64];  // 目标设备名称
    char target_region_name[64];  // 目标内存区域名称
    uint64_t target_address;      // 目标地址
    int operation_type;           // 操作类型
    uint32_t fixed_value;         // 固定值
} cross_device_action_data_t;

// 操作类型常量
#define OP_COPY  1  // 复制源值到目标
#define OP_FIXED 2  // 写入固定值到目标
5.2 实现跨设备动作回调函数
c
CopyInsert
// 跨设备动作回调函数
static int cross_device_action_callback(const monitor_context_t *context, void *user_data) {
    cross_device_action_data_t *data = (cross_device_action_data_t *)user_data;
    
    // 查找目标设备
    device_handle_t target_device = device_find_by_name(data->target_device_name);
    if (!target_device) {
        printf("目标设备未找到: %s\n", data->target_device_name);
        return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    }
    
    // 查找目标内存区域
    memory_region_t *target_region = memory_region_find(target_device, data->target_region_name);
    if (!target_region) {
        printf("目标内存区域未找到: %s\n", data->target_region_name);
        return PHYMUTI_ERROR_MEMORY_REGION_NOT_FOUND;
    }
    
    // 确定要写入的值
    uint32_t value_to_write;
    if (data->operation_type == OP_COPY) {
        value_to_write = (uint32_t)context->value;  // 复制源值
    } else {
        value_to_write = data->fixed_value;         // 使用固定值
    }
    
    // 写入目标内存
    int ret = memory_write_word(target_region, data->target_address, value_to_write);
    
    printf("跨设备操作: 设备 %s:%s 地址 0x%llx 写入值 0x%x\n",
           data->target_device_name, data->target_region_name,
           (unsigned long long)data->target_address, value_to_write);
    
    return ret;
}
5.3 设置跨设备动作
c
CopyInsert
// 创建跨设备动作数据
cross_device_action_data_t *action_data = malloc(sizeof(cross_device_action_data_t));
strncpy(action_data->target_device_name, "peripheral0", sizeof(action_data->target_device_name));
strncpy(action_data->target_region_name, "registers", sizeof(action_data->target_region_name));
action_data->target_address = 0x100;
action_data->operation_type = OP_COPY;
action_data->fixed_value = 0;

// 创建动作
action_id_t cross_device_action = action_create_callback(cross_device_action_callback, action_data);

// 绑定到监视点
monitor_bind_action(write_wp, cross_device_action);
6. 完整示例：实现两个设备间的交互
下面是一个完整的示例，展示如何创建两个设备并在它们之间建立交互关系：

c
CopyInsert
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "phymuti.h"

// 跨设备操作的数据结构
typedef struct {
    char target_device_name[64];
    char target_region_name[64];
    uint64_t target_address;
    int operation_type;
    uint32_t fixed_value;
} cross_device_action_data_t;

#define OP_COPY  1
#define OP_FIXED 2

// 监视点回调函数
static int watchpoint_callback(const monitor_context_t *context, void *user_data) {
    if (!context) return PHYMUTI_ERROR_INVALID_PARAM;
    
    device_handle_t device = memory_region_get_device(context->region);
    const char *device_name = device_get_name(device);
    
    printf("监视点触发: 设备 %s 地址 0x%llx 值 0x%llx\n", 
           device_name, (unsigned long long)context->address, 
           (unsigned long long)context->value);
    
    return PHYMUTI_SUCCESS;
}

// 跨设备动作回调函数
static int cross_device_callback(const monitor_context_t *context, void *user_data) {
    cross_device_action_data_t *data = (cross_device_action_data_t *)user_data;
    
    device_handle_t target_device = device_find_by_name(data->target_device_name);
    if (!target_device) return PHYMUTI_ERROR_DEVICE_NOT_FOUND;
    
    memory_region_t *target_region = memory_region_find(target_device, data->target_region_name);
    if (!target_region) return PHYMUTI_ERROR_MEMORY_REGION_NOT_FOUND;
    
    uint32_t value = data->operation_type == OP_COPY ? 
                     (uint32_t)context->value : data->fixed_value;
    
    printf("跨设备操作: 从 %s 复制值 0x%x 到 %s:%s 地址 0x%llx\n",
           device_get_name(memory_region_get_device(context->region)),
           value, data->target_device_name, data->target_region_name,
           (unsigned long long)data->target_address);
    
    return memory_write_word(target_region, data->target_address, value);
}

int main(void) {
    // 初始化系统
    int ret = phymuti_init();
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "系统初始化失败: %s\n", phymuti_error_string(ret));
        return 1;
    }
    
    // 注册设备类型
    device_ops_t ops = { /* ... 设置操作函数 ... */ };
    ret = device_type_register("test_device", &ops, NULL);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "注册设备类型失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    // 创建设备配置
    device_config_t config = { .user_data = NULL };
    
    // 创建两个设备实例
    device_handle_t device1 = device_create("test_device", "device1", &config);
    device_handle_t device2 = device_create("test_device", "device2", &config);
    
    if (!device1 || !device2) {
        fprintf(stderr, "创建设备实例失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    // 为两个设备创建内存区域
    memory_region_t *region1 = memory_region_create(device1, "regs", 0x0, 1024, MEMORY_FLAG_RW);
    memory_region_t *region2 = memory_region_create(device2, "regs", 0x0, 1024, MEMORY_FLAG_RW);
    
    if (!region1 || !region2) {
        fprintf(stderr, "创建内存区域失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    // 创建监视点
    monitor_id_t wp1 = monitor_add_watchpoint(region1, 0x0, 4, WATCHPOINT_WRITE, 0);
    if (wp1 == MONITOR_INVALID_ID) {
        fprintf(stderr, "创建监视点失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    // 创建基本动作
    action_id_t action1 = action_create_callback(watchpoint_callback, NULL);
    if (action1 == ACTION_INVALID_ID) {
        fprintf(stderr, "创建动作失败\n");
        phymuti_cleanup();
        return 1;
    }
    
    // 绑定基本动作到监视点
    ret = monitor_bind_action(wp1, action1);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "绑定动作失败: %s\n", phymuti_error_string(ret));
        phymuti_cleanup();
        return 1;
    }
    
    // 创建跨设备动作数据
    cross_device_action_data_t *cross_data = malloc(sizeof(cross_device_action_data_t));
    strncpy(cross_data->target_device_name, "device2", sizeof(cross_data->target_device_name));
    strncpy(cross_data->target_region_name, "regs", sizeof(cross_data->target_region_name));
    cross_data->target_address = 0x4;
    cross_data->operation_type = OP_COPY;
    cross_data->fixed_value = 0;
    
    // 创建跨设备动作
    action_id_t action2 = action_create_callback(cross_device_callback, cross_data);
    if (action2 == ACTION_INVALID_ID) {
        fprintf(stderr, "创建跨设备动作失败\n");
        free(cross_data);
        phymuti_cleanup();
        return 1;
    }
    
    // 绑定跨设备动作到监视点
    ret = monitor_bind_action(wp1, action2);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "绑定跨设备动作失败: %s\n", phymuti_error_string(ret));
        free(cross_data);
        phymuti_cleanup();
        return 1;
    }
    
    // 测试内存访问和监视点触发
    uint32_t test_value = 0x12345678;
    printf("写入设备1地址0x0的值: 0x%x\n", test_value);
    ret = memory_write_word(region1, 0x0, test_value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "写入内存失败: %s\n", phymuti_error_string(ret));
        free(cross_data);
        phymuti_cleanup();
        return 1;
    }
    
    // 验证跨设备复制是否成功
    uint32_t verify_value;
    ret = memory_read_word(region2, 0x4, &verify_value);
    if (ret != PHYMUTI_SUCCESS) {
        fprintf(stderr, "读取内存失败: %s\n", phymuti_error_string(ret));
        free(cross_data);
        phymuti_cleanup();
        return 1;
    }
    
    printf("设备2地址0x4的值: 0x%x\n", verify_value);
    printf("跨设备操作%s\n", (verify_value == test_value) ? "成功" : "失败");
    
    // 清理资源
    free(cross_data);
    phymuti_cleanup();
    
    return 0;
}
7. 常见应用场景
SimuPhy2 可用于以下应用场景：

硬件模拟：模拟各种硬件设备和系统
程序调试：设置监视点跟踪关键内存区域的访问
性能分析：跟踪内存访问模式和频率
传感器模拟：一个设备的变化触发另一个设备的响应
中断和信号处理：模拟硬件中断机制
并行系统：模拟多个设备同时工作并交互
8. 最佳实践
合理组织内存区域：为每个设备创建适当的内存区域，反映其实际功能
使用有意义的名称：为设备和内存区域使用描述性名称
正确管理资源：在不需要时释放设备、内存区域和动作
错误处理：始终检查函数返回值，处理可能的错误
监视点放置：只在必要的位置设置监视点，避免过多监视点影响性能
9. 总结
SimuPhy2 模拟器提供了一个灵活而强大的框架，用于模拟各种硬件系统和设备之间的交互。通过其分层架构，用户可以定义设备类型，创建设备实例，管理内存区域，并设置监视点和动作来监控和响应系统状态变化。

特别是监视器和动作触发系统，使得模拟器能够实现复杂的设备间交互和事件驱动的行为，为硬件模拟和调试提供了强大的工具。




## 许可证

本项目采用MIT许可证。 