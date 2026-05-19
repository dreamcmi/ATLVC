#include <stdbool.h>
#include <stdio.h>
#include "atlvc.h"

// 全局上下文指针（供回调函数使用）
static atlvc_context_t* g_callback_ctx = NULL;
// 定义协议栈上下文（带帧头帧尾）
atlvc_context_t g_atlvc_context_with_boundary = {0};
// 定义协议栈上下文（无帧头帧尾）
atlvc_context_t g_atlvc_context_no_boundary = {0};

// 心跳包回调
atlvc_err_t heart_callback(const atlvc_frame_t* frame, void* user_data) {
    if (frame == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }

    printf("\n=== heart_callback triggered ===\n");
    printf("address: 0x%02X\n", frame->address);
    printf("cmd:    0x%02X\n", frame->cmd);
    printf("data len: %d\n", frame->length);
    printf("data:   0x%02X\n", frame->data[0]);  // 心跳包数据固定1字节
    printf("user data: %s\n", (char*)user_data);
    printf("===============================\n\n");

    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t heart_data[] = {0xBB, 0xDD};
    atlvc_frame_t send_frame = {
        .address = frame->address,
        .cmd = frame->cmd,
        .length = sizeof(heart_data),
        .data = heart_data,
    };
    atlvc_err_t err = atlvc_pack(g_callback_ctx, &send_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_pack() failed: %d\n", err);
    }else {
        printf("atlvc_pack() success\n");
        printf("atlvc pack len is %d\n", send_len);
        for (uint16_t i = 0; i < send_len; i++) {
            printf("0x%02X ", send_buff[i]);
        }
        printf("\n");
    }
    return ATLVC_ERR_SUCCESS;
}

// 版本查询回调
atlvc_err_t version_callback(const atlvc_frame_t* frame, void* user_data) {
    if (frame == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }

    printf("\n=== version_callback triggered ===\n");
    printf("address: 0x%02X\n", frame->address);
    printf("cmd:    0x%02X\n", frame->cmd);
    printf("data len: %d\n", frame->length);
    printf("version data: ");
    for (uint8_t i = 0; i < frame->length; i++) {
        printf("0x%02X ", frame->data[i]);
    }
    printf("\nuser data: %d\n", *(int*)user_data);
    printf("=================================\n\n");

    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t version_data[] = {0x01, 0x02, 0x01, 0x01};
    atlvc_frame_t send_frame = {
        .address = frame->address,
        .cmd = frame->cmd,
        .length = sizeof(version_data),
        .data = version_data,
    };
    atlvc_err_t err = atlvc_pack(g_callback_ctx, &send_frame, ATLVC_CHECKSUM_CRC8, send_buff, 20, &send_len);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_pack() failed: %d\n", err);
    }else {
        printf("atlvc_pack() success\n");
        printf("atlvc pack len is %d\n", send_len);
        for (uint16_t i = 0; i < send_len; i++) {
            printf("0x%02X ", send_buff[i]);
        }
        printf("\n");
    }
    return ATLVC_ERR_SUCCESS;
}

// 开关回调
atlvc_err_t turn_on_callback(const atlvc_frame_t* frame, void* user_data) {
    if (frame == NULL) {
        return ATLVC_ERR_INPUT_NULL;
    }

    printf("\n=== turn_on_callback triggered ===\n");
    printf("address: 0x%02X\n", frame->address);
    printf("cmd:    0x%02X\n", frame->cmd);
    printf("data len: %d\n", frame->length);
    printf("turn on data: ");
    for (uint8_t i = 0; i < frame->length; i++) {
        printf("0x%02X ", frame->data[i]);
    }
    printf("\nuser data: %d\n", *(int*)user_data);
    printf("=================================\n\n");

    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t turn_data[] = {0x02};
    atlvc_frame_t send_frame = {
        .address = frame->address,
        .cmd = frame->cmd,
        .length = sizeof(turn_data),
        .data = turn_data,
    };
    atlvc_err_t err = atlvc_pack(g_callback_ctx, &send_frame, ATLVC_CHECKSUM_SUM, send_buff, 20, &send_len);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_pack() failed: %d\n", err);
    }else {
        printf("atlvc_pack() success\n");
        printf("atlvc pack len is %d\n", send_len);
        for (uint16_t i = 0; i < send_len; i++) {
            printf("0x%02X ", send_buff[i]);
        }
        printf("\n");
    }
    return ATLVC_ERR_SUCCESS;
}


// 用户自定义数据（传递给回调函数）
static char g_heart_user_data[] = "heartbeat user data";
static int g_version_user_data = 12345;
static bool g_turn_on_user_data = true;

// 定义指令表
static const atlvc_cmd_rule_t g_user_cmd_table[] = {
    // 地址0x10-0x1F范围，指令0x00：心跳包（支持多地址设备，固定长度1字节，XOR校验）
    {
        .addr_match_type = ATLVC_ADDR_RANGE,
        .addr_pattern = 0x10,
        .addr_end = 0x1F,
        .cmd = 0x00,
        .min_len = 1,
        .max_len = 1,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = heart_callback,
        .desc = "Heartbeat",
        .user_data = g_heart_user_data
    },
    // 地址0x01，指令0x01：版本查询（可变长度2-8字节，CRC8校验）
    {
        .addr_match_type = ATLVC_ADDR_SINGLE,
        .addr_pattern = 0x01,
        .addr_end = 0,
        .cmd = 0x01,
        .min_len = 2,
        .max_len = 8,
        .cs_type = ATLVC_CHECKSUM_CRC8,
        .handler = version_callback,
        .desc = "Get Version",
        .user_data = &g_version_user_data
    },
    // 地址掩码0xF0（匹配0xF0-0xFF），指令0x02：开关控制（支持多地址设备，可变长度1-4字节，SUM校验）
    {
        .addr_match_type = ATLVC_ADDR_MASK,
        .addr_pattern = 0xF0,
        .addr_end = 0xF0,
        .cmd = 0x02,
        .min_len = 1,
        .max_len = 4,
        .cs_type = ATLVC_CHECKSUM_SUM,
        .handler = turn_on_callback,
        .desc = "Turn On",
        .user_data = &g_turn_on_user_data
    }
};

// 计算指令表长度
#define USER_CMD_TABLE_SIZE (sizeof(g_user_cmd_table) / sizeof(atlvc_cmd_rule_t))


// 定义帧头帧尾配置
static const atlvc_frame_boundary_t g_frame_boundary = {
    .header_len = 2,
    .header = {0xAA, 0xBB},      // 自定义帧头：0xAA 0xBB
    .trailer_len = 2,
    .trailer = {0xCC, 0xDD}       // 自定义帧尾：0xCC 0xDD
};


int main(void) {
    atlvc_err_t err;

    // 测试1：无帧头帧尾模式
    printf("===== 测试1：无帧头帧尾模式 =====\n");
    // 初始化上下文（无帧头帧尾）
    err = atlvc_init(&g_atlvc_context_no_boundary, g_user_cmd_table, USER_CMD_TABLE_SIZE);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_init() failed: %d\n", err);
        return err;
    }
    printf("atlvc_init() success (无帧头帧尾)\n");
    
    // 设置全局上下文指针供回调使用
    g_callback_ctx = &g_atlvc_context_no_boundary;

    // 测试：心跳包（地址0x10，指令0x00，数据长度1，数据0xAA，XOR校验位）
    // XOR校验计算：0x10 ^ 0x00 ^ 0x01 ^ 0xAA = 0xBB
    printf("\n=== Test 1.1: 心跳包 (无帧头帧尾) ===\n");
    uint8_t heart_data1[] = {0x10, 0x00, 0x01, 0xAA, 0xBB};
    err = atlvc_process(&g_atlvc_context_no_boundary, heart_data1, sizeof(heart_data1));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("heart_process failed: %d\n", err);
    } else {
        printf("heart_process success\n");
    }

    // 反初始化
    err = atlvc_deinit(&g_atlvc_context_no_boundary);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_deinit() (no boundary) failed: %d\n", err);
    }

    
    // 测试2：带帧头帧尾模式
    printf("\n===== 测试2：带帧头帧尾模式 =====\n");
    // 初始化上下文（带帧头帧尾）
    err = atlvc_init_with_boundary(&g_atlvc_context_with_boundary, g_user_cmd_table, USER_CMD_TABLE_SIZE, &g_frame_boundary);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_init_with_boundary() failed: %d\n", err);
        return err;
    }
    printf("atlvc_init_with_boundary() success (帧头: 0xAA 0xBB, 帧尾: 0xCC 0xDD)\n");
    
    // 设置全局上下文指针供回调使用
    g_callback_ctx = &g_atlvc_context_with_boundary;

    // 测试：带帧头帧尾的心跳包
    // 完整帧：帧头(0xAA 0xBB) + 地址(0x10) + 指令(0x00) + 长度(0x01) + 数据(0xAA) + 校验(0xBB) + 帧尾(0xCC 0xDD)
    printf("\n=== Test 2.1: 心跳包 (带帧头帧尾 - 正确) ===\n");
    uint8_t heart_with_boundary[] = {0xAA, 0xBB, 0x10, 0x00, 0x01, 0xAA, 0xBB, 0xCC, 0xDD};
    err = atlvc_process(&g_atlvc_context_with_boundary, heart_with_boundary, sizeof(heart_with_boundary));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("heart_process (with boundary) failed: %d\n", err);
    } else {
        printf("heart_process (with boundary) success\n");
    }

    // 测试：帧头错误
    printf("\n=== Test 2.2: 心跳包 (帧头错误 - 预期失败) ===\n");
    uint8_t heart_bad_header[] = {0xAB, 0xBB, 0x10, 0x00, 0x01, 0xAA, 0xBB, 0xCC, 0xDD};
    err = atlvc_process(&g_atlvc_context_with_boundary, heart_bad_header, sizeof(heart_bad_header));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("heart_process (bad header) failed (expected): %d\n", err);
    } else {
        printf("heart_process (bad header) success (unexpected!)\n");
    }

    // 测试：帧尾错误
    printf("\n=== Test 2.3: 心跳包 (帧尾错误 - 预期失败) ===\n");
    uint8_t heart_bad_trailer[] = {0xAA, 0xBB, 0x10, 0x00, 0x01, 0xAA, 0xBB, 0xCD, 0xDD};
    err = atlvc_process(&g_atlvc_context_with_boundary, heart_bad_trailer, sizeof(heart_bad_trailer));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("heart_process (bad trailer) failed (expected): %d\n", err);
    } else {
        printf("heart_process (bad trailer) success (unexpected!)\n");
    }

    // 反初始化
    err = atlvc_deinit(&g_atlvc_context_with_boundary);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_deinit() (with boundary) failed: %d\n", err);
    }
    printf("\natlvc_deinit() success\n");
    return 0;
}