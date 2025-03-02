/**
 * @file phymuti_error.c
 * @brief PhyMuTi错误码实现
 */

#include "phymuti_error.h"

/**
 * @brief 获取错误码对应的错误信息
 * 
 * @param error_code 错误码
 * @return const char* 错误信息
 */
const char* phymuti_error_string(int error_code) {
    switch (error_code) {
        /* 通用错误码 */
        case PHYMUTI_SUCCESS:
            return "Success";
        case PHYMUTI_ERROR_INVALID_PARAM:
            return "Invalid parameter";
        case PHYMUTI_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case PHYMUTI_ERROR_NOT_FOUND:
            return "Not found";
        case PHYMUTI_ERROR_ALREADY_EXISTS:
            return "Already exists";
        case PHYMUTI_ERROR_NOT_SUPPORTED:
            return "Not supported";
        case PHYMUTI_ERROR_PERMISSION:
            return "Permission denied";
        case PHYMUTI_ERROR_TIMEOUT:
            return "Timeout";
        case PHYMUTI_ERROR_BUSY:
            return "Resource busy";
        case PHYMUTI_ERROR_IO:
            return "I/O error";
        case PHYMUTI_ERROR_INTERNAL:
            return "Internal error";
            
        /* 设备管理器错误码 */
        case PHYMUTI_ERROR_DEVICE_TYPE_NOT_FOUND:
            return "Device type not found";
        case PHYMUTI_ERROR_DEVICE_NOT_FOUND:
            return "Device not found";
        case PHYMUTI_ERROR_DEVICE_CREATE_FAILED:
            return "Device creation failed";
        case PHYMUTI_ERROR_DEVICE_DESTROY_FAILED:
            return "Device destruction failed";
        case PHYMUTI_ERROR_DEVICE_RESET_FAILED:
            return "Device reset failed";
        case PHYMUTI_ERROR_DEVICE_SAVE_STATE_FAILED:
            return "Device state save failed";
        case PHYMUTI_ERROR_DEVICE_LOAD_STATE_FAILED:
            return "Device state load failed";
            
        /* 内存管理器错误码 */
        case PHYMUTI_ERROR_MEMORY_REGION_NOT_FOUND:
            return "Memory region not found";
        case PHYMUTI_ERROR_MEMORY_OUT_OF_RANGE:
            return "Memory access out of range";
        case PHYMUTI_ERROR_MEMORY_PERMISSION:
            return "Memory access permission denied";
        case PHYMUTI_ERROR_MEMORY_ALIGNMENT:
            return "Memory alignment error";
            
        /* 监视器错误码 */
        case PHYMUTI_ERROR_WATCHPOINT_NOT_FOUND:
            return "Watchpoint not found";
        case PHYMUTI_ERROR_WATCHPOINT_LIMIT:
            return "Watchpoint limit exceeded";
        case PHYMUTI_ERROR_WATCHPOINT_INVALID_TYPE:
            return "Invalid watchpoint type";
            
        /* 动作管理器错误码 */
        case PHYMUTI_ERROR_ACTION_NOT_FOUND:
            return "Action not found";
        case PHYMUTI_ERROR_ACTION_EXECUTE_FAILED:
            return "Action execution failed";
        case PHYMUTI_ERROR_ACTION_INVALID_TYPE:
            return "Invalid action type";
            
        /* 规则引擎错误码 */
        case PHYMUTI_ERROR_RULE_NOT_FOUND:
            return "Rule not found";
        case PHYMUTI_ERROR_RULE_CONDITION_FAILED:
            return "Rule condition evaluation failed";
        case PHYMUTI_ERROR_RULE_ACTION_FAILED:
            return "Rule action execution failed";
            
        default:
            return "Unknown error";
    }
} 