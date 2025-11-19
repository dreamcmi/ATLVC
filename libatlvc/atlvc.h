//
// Created by darren on 2025/11/18.
//

#ifndef ATLVC_ATLVC_H
#define ATLVC_ATLVC_H

#include "stdint.h"
#include <string.h>
#include "atlvc_err.h"

#define ATLVC_BUFF_LEN (255)

// 帧结构（地址+指令+长度+数据+校验位）
// 总长度 = 1(地址) + 1(指令) + 1(长度) + N(数据) + 1(校验位) = N+4
typedef struct {
    uint8_t address;    // 地址（1字节）
    uint8_t cmd;        // 指令（1字节）
    uint8_t length;     // 数据长度（1字节，N的大小）
    uint8_t *data;      // 数据（N字节）
    // 校验位紧跟在data后（1字节，根据cs_type定义）
} atlvc_frame_t;

// 校验方式
typedef enum {
    ATLVC_CHECKSUM_NONE = 0,    // 无校验（不需要校验位）
    ATLVC_CHECKSUM_XOR,         // XOR校验（字节异或，1字节校验位）
    ATLVC_CHECKSUM_CRC8,        // CRC8校验（1字节校验位）
} atlvc_checksum_type_t;

// 前向声明：指令处理回调函数类型（解析后触发的业务逻辑）
typedef atlvc_err_t (*atlvc_cmd_handler_t)(const atlvc_frame_t* frame, void* user_data);

// ATLVC预定义指令表项（存储指令的协议规则）
typedef struct {
    uint8_t address;                // 指令对应的地址（支持多地址设备）
    uint8_t cmd;                    // 指令码（唯一标识）
    uint8_t min_len;                // 最小数据长度（N的最小值）
    uint8_t max_len;                // 最大数据长度（N的最大值，固定长度时min=max）
    atlvc_checksum_type_t cs_type;  // 校验方式
    atlvc_cmd_handler_t handler;    // 指令处理回调（解析成功后执行）
    const char* desc;               // 指令描述（调试用）
    void* user_data;                // 回调函数的用户数据（可选）
} atlvc_cmd_rule_t;

// 上下文
typedef struct {
    const atlvc_cmd_rule_t* rule_table;  // 指令规则表
    uint16_t rule_table_size;            // 规则表长度
} atlvc_context_t;

// 函数声明
atlvc_err_t atlvc_init(atlvc_context_t *ctx, const atlvc_cmd_rule_t* rule_table, uint16_t length);
atlvc_err_t atlvc_deinit(atlvc_context_t *ctx);
atlvc_err_t atlvc_process(atlvc_context_t *ctx, uint8_t *p, uint16_t len);

#endif //ATLVC_ATLVC_H