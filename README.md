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
    monitor_id_t wp_id = monitor_add_watchpoint(region, 0x1000, 2, WATCHPOINT_WRITE);
    
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

## 许可证

本项目采用MIT许可证。 