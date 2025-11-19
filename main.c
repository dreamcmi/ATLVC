#include <stdio.h>
#include "atlvc.h"

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
    return ATLVC_ERR_SUCCESS;
}

// 用户自定义数据（传递给回调函数）
static char g_heart_user_data[] = "heartbeat user data";
static int g_version_user_data = 12345;

// 定义指令表
static const atlvc_cmd_rule_t g_user_cmd_table[] = {
    // 地址0x01，指令0x00：心跳包（固定长度1字节，XOR校验）
    {
        .address = 0x01,
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
        .address = 0x01,
        .cmd = 0x01,
        .min_len = 2,
        .max_len = 8,
        .cs_type = ATLVC_CHECKSUM_CRC8,
        .handler = version_callback,
        .desc = "Get Version",
        .user_data = &g_version_user_data
    }
};

// 计算指令表长度
#define USER_CMD_TABLE_SIZE (sizeof(g_user_cmd_table) / sizeof(atlvc_cmd_rule_t))

// 定义协议栈上下文
atlvc_context_t g_atlvc_context = {0};

int main(void) {
    // 初始化上下文
    atlvc_err_t err = atlvc_init(&g_atlvc_context, g_user_cmd_table, USER_CMD_TABLE_SIZE);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_init() failed: %d\n", err);
        return err;
    }
    printf("atlvc_init() success\n");

    // 测试1：心跳包（地址0x01+指令0x00+数据长度1+数据0xAA+XOR校验位）
    // XOR校验计算：0x01 ^ 0x00 ^ 0x01 ^ 0xAA = 0xAA
    uint8_t heart_data[] = {0x01, 0x00, 0x01, 0xAA, 0xAA};
    err = atlvc_process(&g_atlvc_context, heart_data, sizeof(heart_data));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("heart_process failed: %d\n", err);
    } else {
        printf("heart_process success\n");
    }

    // 测试2：版本查询（地址0x01+指令0x01+数据长度3+数据0x01,0x02,0x03+CRC8校验位）
    uint8_t version_data[] = {0x01, 0x01, 0x03, 0x01, 0x02, 0x03, 0x39};
    err = atlvc_process(&g_atlvc_context, version_data, sizeof(version_data));
    if (err != ATLVC_ERR_SUCCESS) {
        printf("version_process failed: %d\n", err);
    } else {
        printf("version_process success\n");
    }

    // 反初始化
    err = atlvc_deinit(&g_atlvc_context);
    if (err != ATLVC_ERR_SUCCESS) {
        printf("atlvc_deinit() failed: %d\n", err);
    }
    printf("atlvc_deinit() success\n");
    return 0;
}