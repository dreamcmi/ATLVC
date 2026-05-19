//
// Created by darren on 2025/11/18.
//

#include "atlvc.h"
#include "atlvc_check.h"
// #include <stdio.h>

// 地址匹配函数（支持多地址设备）
static int atlvc_addr_match(const atlvc_cmd_rule_t* rule, uint8_t address) {
    switch (rule->addr_match_type) {
        case ATLVC_ADDR_SINGLE:
            return (rule->addr_pattern == address);
        case ATLVC_ADDR_RANGE:
            return (address >= rule->addr_pattern && address <= rule->addr_end);
        case ATLVC_ADDR_MASK:
            return ((address & rule->addr_end) == (rule->addr_pattern & rule->addr_end));
        case ATLVC_ADDR_WILDCARD:
            return 1;
        default:
            return 0;
    }
}

// 统一校验入口函数
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
        case ATLVC_CHECKSUM_SUM:
            calculated_checksum = atlvc_sum_checksum(p, data_len);
            break;
        default:
            return ATLVC_ERR_FAILURE;  // 未知校验类型
    }
    return (calculated_checksum == checksum) ? ATLVC_ERR_SUCCESS : ATLVC_ERR_CHECKSUM_ERR;
}

// 初始化上下文（无帧头帧尾）
atlvc_err_t atlvc_init(atlvc_context_t *ctx, const atlvc_cmd_rule_t* rule_table, uint16_t length) {
    if (ctx == NULL || rule_table == NULL || length == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    ctx->rule_table = rule_table;
    ctx->rule_table_size = length;
    ctx->boundary = NULL;  // 无帧头帧尾
    return ATLVC_ERR_SUCCESS;
}

// 初始化上下文（带帧头帧尾配置）
atlvc_err_t atlvc_init_with_boundary(atlvc_context_t *ctx, const atlvc_cmd_rule_t* rule_table, uint16_t length, const atlvc_frame_boundary_t* boundary) {
    if (ctx == NULL || rule_table == NULL || length == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    ctx->rule_table = rule_table;
    ctx->rule_table_size = length;
    ctx->boundary = boundary;
    return ATLVC_ERR_SUCCESS;
}

// 反初始化
atlvc_err_t atlvc_deinit(atlvc_context_t *ctx) {
    if (ctx == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }
    ctx->rule_table = NULL;
    ctx->rule_table_size = 0;
    ctx->boundary = NULL;
    return ATLVC_ERR_SUCCESS;
}

// 核心处理流程：解析数据->合法性校验->触发回调
atlvc_err_t atlvc_process(atlvc_context_t *ctx, uint8_t *p, uint16_t len) {
    // 1. 输入参数校验
    if (ctx == NULL || p == NULL || len == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    if (ctx->rule_table == NULL || ctx->rule_table_size == 0) {
        return ATLVC_ERR_INPUT_NULL;
    }

    uint16_t offset = 0;
    uint16_t data_len = len;

    // 2. 帧头校验（如果配置了帧头）
    if (ctx->boundary != NULL && ctx->boundary->header_len > 0) {
        if (len < ctx->boundary->header_len) {
            return ATLVC_ERR_INVALID_FRAME;
        }
        // 检查帧头是否匹配
        if (memcmp(p, ctx->boundary->header, ctx->boundary->header_len) != 0) {
            return ATLVC_ERR_INVALID_FRAME;
        }
        offset += ctx->boundary->header_len;
        data_len -= ctx->boundary->header_len;
    }

    // 3. 帧尾校验（如果配置了帧尾）
    if (ctx->boundary != NULL && ctx->boundary->trailer_len > 0) {
        if (data_len < ctx->boundary->trailer_len) {
            return ATLVC_ERR_INVALID_FRAME;
        }
        // 检查帧尾是否匹配
        uint16_t trailer_offset = offset + data_len - ctx->boundary->trailer_len;
        if (memcmp(p + trailer_offset, ctx->boundary->trailer, ctx->boundary->trailer_len) != 0) {
            return ATLVC_ERR_INVALID_FRAME;
        }
        data_len -= ctx->boundary->trailer_len;
    }

    // 4. 帧头合法性校验（最小帧长度：地址+指令+长度 = 3字节）
    if (data_len < 3) {
        return ATLVC_ERR_INVALID_FRAME;
    }

    uint8_t address = p[offset];
    uint8_t cmd = p[offset + 1];
    uint8_t frame_data_len = p[offset + 2];

    // 5. 查找匹配的指令规则（地址+指令匹配，支持多地址设备）
    const atlvc_cmd_rule_t* matched_rule = NULL;
    for (uint16_t i = 0; i < ctx->rule_table_size; i++) {
        if (ctx->rule_table[i].cmd == cmd && atlvc_addr_match(&ctx->rule_table[i], address)) {
            matched_rule = &ctx->rule_table[i];
            break;
        }
    }

    if (matched_rule == NULL) {
        return ATLVC_ERR_CMD_NOT_FOUND;
    }

    // 6. 数据长度校验（匹配规则表的min/max）
    if (frame_data_len < matched_rule->min_len || frame_data_len > matched_rule->max_len) {
        return ATLVC_ERR_DATA_LEN_ERR;
    }

    // 7. 总长度校验（包含校验位）
    size_t expected_total_len = 3 + frame_data_len;  // 地址+指令+长度+数据
    if (matched_rule->cs_type != ATLVC_CHECKSUM_NONE) {
        expected_total_len += 1;  // 校验位占1字节
    }

    if (data_len != expected_total_len) {
        return ATLVC_ERR_INVALID_FRAME;
    }

    // 8. 校验位验证（如果需要）
    if (matched_rule->cs_type != ATLVC_CHECKSUM_NONE) {
        uint8_t checksum = p[offset + 3 + frame_data_len];  // 校验位在数据段后
        atlvc_err_t cs_err = atlvc_check_sum(
            p + offset,                  // 校验范围：从地址开始到数据段结束
            3 + frame_data_len,          // 校验长度：地址+指令+长度+数据
            matched_rule->cs_type,
            checksum
        );

        if (cs_err != ATLVC_ERR_SUCCESS) {
            return ATLVC_ERR_CHECKSUM_ERR;
        }
    }

    // 9. 赋值frame用于传递
    atlvc_frame_t frame = {
        .address = address,
        .cmd = cmd,
        .length = frame_data_len,
        .data = p + offset + 3  // 数据段起始地址
    };

    // 10. 触发回调函数（传递帧数据和用户数据）
    if (matched_rule->handler == NULL) {
        return ATLVC_ERR_HANDLER_NULL;
    }

    atlvc_err_t handler_err = matched_rule->handler(&frame, matched_rule->user_data);
    if (handler_err != ATLVC_ERR_SUCCESS) {
        return handler_err;
    }

    return ATLVC_ERR_SUCCESS;
}


atlvc_err_t atlvc_pack(atlvc_context_t *ctx, atlvc_frame_t *frame, atlvc_checksum_type_t check_type, uint8_t *buff, uint16_t buff_len, uint16_t *frame_len) {
    if (frame == NULL || buff == NULL || buff_len == 0 || frame_len == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }

    if (frame->length > 0 && frame->data == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }

    // 计算打包长度
    uint16_t pack_len = 0;
    
    // 帧头长度
    uint16_t header_len = 0;
    if (ctx != NULL && ctx->boundary != NULL) {
        header_len = ctx->boundary->header_len;
    }
    pack_len += header_len;
    
    // 核心数据长度：地址+指令+长度+数据
    pack_len += frame->length + 3;
    
    // 校验位长度
    if (check_type != ATLVC_CHECKSUM_NONE) {
        pack_len += 1;
    }
    
    // 帧尾长度
    uint16_t trailer_len = 0;
    if (ctx != NULL && ctx->boundary != NULL) {
        trailer_len = ctx->boundary->trailer_len;
    }
    pack_len += trailer_len;

    // 检查缓冲区是否足够
    if (pack_len > buff_len) {
        return ATLVC_ERR_DATA_LEN_ERR;
    }

    uint16_t offset = 0;

    // 写入帧头
    if (header_len > 0) {
        memcpy(buff + offset, ctx->boundary->header, header_len);
        offset += header_len;
    }

    // 写入核心数据
    buff[offset++] = frame->address;
    buff[offset++] = frame->cmd;
    buff[offset++] = frame->length;
    if (frame->length > 0) {
        memcpy(buff + offset, frame->data, frame->length);
        offset += frame->length;
    }

    // 写入校验位
    if (check_type != ATLVC_CHECKSUM_NONE) {
        uint16_t check_data_len = offset - header_len;  // 校验数据长度（地址+指令+长度+数据）
        uint8_t checksum = 0;

        switch (check_type) {
            case ATLVC_CHECKSUM_XOR:
                checksum = atlvc_xor_checksum(buff + header_len, check_data_len);
                break;
            case ATLVC_CHECKSUM_CRC8:
                checksum = atlvc_crc8_checksum(buff + header_len, check_data_len);
                break;
            case ATLVC_CHECKSUM_SUM:
                checksum = atlvc_sum_checksum(buff + header_len, check_data_len);
                break;
            default:
                break;
        }

        buff[offset++] = checksum;
    }

    // 写入帧尾
    if (trailer_len > 0) {
        memcpy(buff + offset, ctx->boundary->trailer, trailer_len);
        offset += trailer_len;
    }

    *frame_len = pack_len;
    return ATLVC_ERR_SUCCESS;
}