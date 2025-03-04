/**
 * @file phymuti_error.h
 * @brief PhyMuTi错误码定义
 */

#ifndef PHYMUTI_ERROR_H
#define PHYMUTI_ERROR_H

/* 通用错误码 */
#define PHYMUTI_SUCCESS                0   /* 成功 */
#define PHYMUTI_ERROR_INVALID_PARAM   -1   /* 无效参数 */
#define PHYMUTI_ERROR_OUT_OF_MEMORY   -2   /* 内存不足 */
#define PHYMUTI_ERROR_NOT_FOUND       -3   /* 未找到 */
#define PHYMUTI_ERROR_ALREADY_EXISTS  -4   /* 已存在 */
#define PHYMUTI_ERROR_NOT_SUPPORTED   -5   /* 不支持 */
#define PHYMUTI_ERROR_PERMISSION      -6   /* 权限错误 */
#define PHYMUTI_ERROR_TIMEOUT         -7   /* 超时 */
#define PHYMUTI_ERROR_BUSY            -8   /* 忙 */
#define PHYMUTI_ERROR_IO              -9   /* IO错误 */
#define PHYMUTI_ERROR_INTERNAL        -10  /* 内部错误 */
#define PHYMUTI_ERROR_MUTEX_INIT_FAILED    -11  /* 互斥锁初始化失败 */
#define PHYMUTI_ERROR_MUTEX_DESTROY_FAILED -12  /* 互斥锁销毁失败 */
#define PHYMUTI_ERROR_MUTEX_LOCK_FAILED    -13  /* 互斥锁加锁失败 */
#define PHYMUTI_ERROR_MUTEX_UNLOCK_FAILED  -14  /* 互斥锁解锁失败 */

/* 设备管理器错误码 */
#define PHYMUTI_ERROR_DEVICE_TYPE_NOT_FOUND    -100  /* 设备类型未找到 */
#define PHYMUTI_ERROR_DEVICE_NOT_FOUND         -101  /* 设备未找到 */
#define PHYMUTI_ERROR_DEVICE_CREATE_FAILED     -102  /* 设备创建失败 */
#define PHYMUTI_ERROR_DEVICE_DESTROY_FAILED    -103  /* 设备销毁失败 */
#define PHYMUTI_ERROR_DEVICE_RESET_FAILED      -104  /* 设备重置失败 */
#define PHYMUTI_ERROR_DEVICE_SAVE_STATE_FAILED -105  /* 设备状态保存失败 */
#define PHYMUTI_ERROR_DEVICE_LOAD_STATE_FAILED -106  /* 设备状态加载失败 */

/* 内存管理器错误码 */
#define PHYMUTI_ERROR_MEMORY_REGION_NOT_FOUND  -200  /* 内存区域未找到 */
#define PHYMUTI_ERROR_MEMORY_OUT_OF_RANGE      -201  /* 内存访问越界 */
#define PHYMUTI_ERROR_MEMORY_PERMISSION        -202  /* 内存访问权限错误 */
#define PHYMUTI_ERROR_MEMORY_ALIGNMENT         -203  /* 内存对齐错误 */

/* 监视器错误码 */
#define PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND     -300  /* 监视点未找到 */
#define PHYMUTI_ERROR_WATCHPOINT_LIMIT         -301  /* 监视点数量超限 */
#define PHYMUTI_ERROR_WATCHPOINT_INVALID_TYPE  -302  /* 无效的监视点类型 */

/* 动作管理器错误码 */
#define PHYMUTI_ERROR_ACTION_NOT_FOUND         -400  /* 动作未找到 */
#define PHYMUTI_ERROR_ACTION_EXECUTE_FAILED    -401  /* 动作执行失败 */
#define PHYMUTI_ERROR_ACTION_INVALID_TYPE      -402  /* 无效的动作类型 */

/* 规则引擎错误码 */
#define PHYMUTI_ERROR_RULE_NOT_FOUND           -500  /* 规则未找到 */
#define PHYMUTI_ERROR_RULE_CONDITION_FAILED    -501  /* 规则条件评估失败 */
#define PHYMUTI_ERROR_RULE_ACTION_FAILED       -502  /* 规则动作执行失败 */

/**
 * @brief 获取错误码对应的错误信息
 * 
 * @param error_code 错误码
 * @return const char* 错误信息
 */
const char* phymuti_error_string(int error_code);

#endif /* PHYMUTI_ERROR_H */ 