//
// Created by darren on 2025/11/18.
//

#include "atlvc.h"
#include "atlvc_check.h"
// #include <stdio.h>

// 统一校验入口函数（调用独立的XOR/CRC8校验函数）
atlvc_err_t atlvc_check_sum(uint8_t *p, size_t data_len, atlvc_checksum_type_t cs_type, uint8_t checksum) {
    if (p == NULL || data_len == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    uint8_t calculated_checksum = 0;
    switch (cs_type) {
        case ATLVC_CHECKSUM_NONE:
            return ATLVC_ERR_SUCCESS;  // 无校验，直接通过

        case ATLVC_CHECKSUM_XOR:
            calculated_checksum = atlvc_xor_checksum(p, data_len);
            break;

        case ATLVC_CHECKSUM_CRC8:
            calculated_checksum = atlvc_crc8_checksum(p, data_len);
            break;

        default:
            return ATLVC_ERR_FAILURE;  // 未知校验类型
    }

    // 对比计算结果与传入的校验位
    return (calculated_checksum == checksum) ? ATLVC_ERR_SUCCESS : ATLVC_ERR_CHECKSUM_ERR;
}

// 初始化上下文
atlvc_err_t atlvc_init(atlvc_context_t *ctx, const atlvc_cmd_rule_t* rule_table, uint16_t length) {
    if (ctx == NULL || rule_table == NULL || length == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    // printf("atlvc_init: rule_table=%p length=%d\n", rule_table, length);
    // printf("===========begin check==========\n");
    // for (uint16_t i = 0; i < length; i++) {
    //     printf("=====No.%d=====\n", i);
    //     printf("addr:  0x%02X\n", rule_table[i].address);
    //     printf("cmd:   0x%02X\n", rule_table[i].cmd);
    //     printf("min length: %d\n", rule_table[i].min_len);
    //     printf("max length: %d\n", rule_table[i].max_len);
    //     printf("check type: %d\n", rule_table[i].cs_type);
    //     printf("callback:   %p\n", rule_table[i].handler);
    //     printf("user_data:  %p\n", rule_table[i].user_data);
    //     printf("desc:       %s\n", rule_table[i].desc);
    // }
    // printf("===========end check==========\n");

    ctx->rule_table = rule_table;
    ctx->rule_table_size = length;
    return ATLVC_ERR_SUCCESS;
}

// 反初始化（空实现，如需释放资源可在此扩展）
atlvc_err_t atlvc_deinit(atlvc_context_t *ctx) {
    if (ctx == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }
    ctx->rule_table = NULL;
    ctx->rule_table_size = 0;
    return ATLVC_ERR_SUCCESS;
}

// 核心处理流程：解析数据->合法性校验->触发回调
atlvc_err_t atlvc_process(atlvc_context_t *ctx, uint8_t *p, uint16_t len) {
    // 1. 输入参数校验
    if (ctx == NULL || p == NULL || len == 0) {
        // printf("process err: input null\n");
        return ATLVC_ERR_INPUT_NULL;
    }

    // 2. 帧头合法性校验（最小帧长度：地址+指令+长度 = 3字节）
    if (len < 3) {
        // printf("process err: invalid frame (len=%zu < 3)\n", len);
        return ATLVC_ERR_INVALID_FRAME;
    }

    uint8_t address = p[0];
    uint8_t cmd = p[1];
    uint8_t data_len = p[2];  // 数据段长度（N）
    atlvc_frame_t frame = {
        .address = address,
        .cmd = cmd,
        .length = data_len,
        .data = p + 3  // 数据段起始地址
    };

    // 3. 查找匹配的指令规则（地址+指令匹配）
    const atlvc_cmd_rule_t* matched_rule = NULL;
    for (uint16_t i = 0; i < ctx->rule_table_size; i++) {
        if (ctx->rule_table[i].address == address && ctx->rule_table[i].cmd == cmd) {
            matched_rule = &ctx->rule_table[i];
            break;
        }
    }

    if (matched_rule == NULL) {
        // printf("process err: cmd not found (addr=0x%02X, cmd=0x%02X)\n", address, cmd);
        return ATLVC_ERR_CMD_NOT_FOUND;
    }

    // 4. 数据长度校验（匹配规则表的min/max）
    if (data_len < matched_rule->min_len || data_len > matched_rule->max_len) {
        // printf("process err: data len err (actual=%d, min=%d, max=%d)\n", data_len, matched_rule->min_len, matched_rule->max_len);
        return ATLVC_ERR_DATA_LEN_ERR;
    }

    // 5. 总长度校验（包含校验位）
    size_t expected_total_len = 3 + data_len;  // 地址+指令+长度+数据
    if (matched_rule->cs_type != ATLVC_CHECKSUM_NONE) {
        expected_total_len += 1;  // 校验位占1字节
    }

    if (len != expected_total_len) {
        // printf("process err: total len err (actual=%zu, expected=%zu)\n", len, expected_total_len);
        return ATLVC_ERR_INVALID_FRAME;
    }

    // 6. 校验位验证（如果需要）
    if (matched_rule->cs_type != ATLVC_CHECKSUM_NONE) {
        uint8_t checksum = p[3 + data_len];  // 校验位在数据段后
        atlvc_err_t cs_err = atlvc_check_sum(
            p,                  // 校验范围：从地址开始到数据段结束
            3 + data_len,       // 校验长度：地址+指令+长度+数据
            matched_rule->cs_type,
            checksum
        );

        if (cs_err != ATLVC_ERR_SUCCESS) {
            // printf("process err: checksum failed (type=%d, actual=0x%02X)\n", matched_rule->cs_type, checksum);
            return ATLVC_ERR_CHECKSUM_ERR;
        }
    }

    // 7. 触发回调函数（传递帧数据和用户数据）
    if (matched_rule->handler == NULL) {
        // printf("process err: handler null (addr=0x%02X, cmd=0x%02X)\n", address, cmd);
        return ATLVC_ERR_HANDLER_NULL;
    }

    // printf("process success: match cmd [%s] (addr=0x%02X, cmd=0x%02X, data_len=%d)\n", matched_rule->desc, address, cmd, data_len);
    atlvc_err_t handler_err = matched_rule->handler(&frame, matched_rule->user_data);
    if (handler_err != ATLVC_ERR_SUCCESS) {
        // printf("process err: handler failed (err=%d)\n", handler_err);
        return handler_err;
    }

    return ATLVC_ERR_SUCCESS;
}