#include <stdio.h>
#include <stdbool.h>
#include "../libatlvc/atlvc.h"

static int test_passed = 0;
static int test_failed = 0;

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  [PASS] %s\n", message); \
            test_passed++; \
        } else { \
            printf("  [FAIL] %s\n", message); \
            test_failed++; \
        } \
    } while(0)

atlvc_err_t simple_callback(const atlvc_frame_t* frame, void* user_data) {
    (void)frame;
    (void)user_data;
    return ATLVC_ERR_SUCCESS;
}

static const atlvc_cmd_rule_t test_cmd_table[] = {
    {
        .addr_match_type = ATLVC_ADDR_SINGLE,
        .addr_pattern = 0x01,
        .addr_end = 0,
        .cmd = 0x01,
        .min_len = 0,
        .max_len = 10,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = simple_callback,
        .desc = "Pack Test",
        .user_data = NULL
    }
};

#define TEST_CMD_TABLE_SIZE (sizeof(test_cmd_table) / sizeof(atlvc_cmd_rule_t))

void test_pack_xor(void) {
    printf("\n===== 测试XOR打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试XOR打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "XOR打包应成功");
    TEST_ASSERT(send_len == 7, "XOR打包长度应为7（3+3+1）");
    
    // 验证数据
    TEST_ASSERT(send_buff[0] == 0x01, "地址应正确");
    TEST_ASSERT(send_buff[1] == 0x01, "指令应正确");
    TEST_ASSERT(send_buff[2] == 0x03, "长度应正确");
    TEST_ASSERT(send_buff[3] == 0x01 && send_buff[4] == 0x02 && send_buff[5] == 0x03, "数据应正确");
    
    // 验证校验位
    uint8_t expected_checksum = 0x01 ^ 0x01 ^ 0x03 ^ 0x01 ^ 0x02 ^ 0x03;
    TEST_ASSERT(send_buff[6] == expected_checksum, "XOR校验位应正确");
    
    atlvc_deinit(&ctx);
}

void test_pack_crc8(void) {
    printf("\n===== 测试CRC8打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x02,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试CRC8打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_CRC8, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "CRC8打包应成功");
    TEST_ASSERT(send_len == 7, "CRC8打包长度应为7（3+3+1）");
    
    // 验证数据
    TEST_ASSERT(send_buff[0] == 0x01, "地址应正确");
    TEST_ASSERT(send_buff[1] == 0x02, "指令应正确");
    TEST_ASSERT(send_buff[2] == 0x03, "长度应正确");
    TEST_ASSERT(send_buff[3] == 0x01 && send_buff[4] == 0x02 && send_buff[5] == 0x03, "数据应正确");
    
    atlvc_deinit(&ctx);
}

void test_pack_sum(void) {
    printf("\n===== 测试SUM打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01};
    atlvc_frame_t test_frame = {
        .address = 0xF2,
        .cmd = 0x03,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试SUM打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_SUM, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "SUM打包应成功");
    TEST_ASSERT(send_len == 5, "SUM打包长度应为5（3+1+1）");
    
    // 验证数据
    TEST_ASSERT(send_buff[0] == 0xF2, "地址应正确");
    TEST_ASSERT(send_buff[1] == 0x03, "指令应正确");
    TEST_ASSERT(send_buff[2] == 0x01, "长度应正确");
    TEST_ASSERT(send_buff[3] == 0x01, "数据应正确");
    
    // 验证校验位
    uint8_t expected_checksum = (0xF2 + 0x03 + 0x01 + 0x01) & 0xFF;
    TEST_ASSERT(send_buff[4] == expected_checksum, "SUM校验位应正确");
    
    atlvc_deinit(&ctx);
}

void test_pack_no_checksum(void) {
    printf("\n===== 测试无校验打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01, 0x02};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x04,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试无校验打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_NONE, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "无校验打包应成功");
    TEST_ASSERT(send_len == 5, "无校验打包长度应为5（3+2）");
    
    // 验证数据
    TEST_ASSERT(send_buff[0] == 0x01, "地址应正确");
    TEST_ASSERT(send_buff[1] == 0x04, "指令应正确");
    TEST_ASSERT(send_buff[2] == 0x02, "长度应正确");
    TEST_ASSERT(send_buff[3] == 0x01 && send_buff[4] == 0x02, "数据应正确");
    
    atlvc_deinit(&ctx);
}

void test_pack_with_boundary(void) {
    printf("\n===== 测试带帧头帧尾打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_frame_boundary_t boundary = {
        .header_len = 2,
        .header = {0xAA, 0xBB},
        .trailer_len = 2,
        .trailer = {0xCC, 0xDD}
    };
    
    err = atlvc_init_with_boundary(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE, &boundary);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x05,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试带帧头帧尾的XOR打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "带帧头帧尾打包应成功");
    TEST_ASSERT(send_len == 9, "带帧头帧尾打包长度应为9（2+4+1+2）");
    
    // 验证帧头
    TEST_ASSERT(send_buff[0] == 0xAA && send_buff[1] == 0xBB, "帧头应正确");
    
    // 验证核心数据
    TEST_ASSERT(send_buff[2] == 0x01 && send_buff[3] == 0x05 && 
                send_buff[4] == 0x01 && send_buff[5] == 0x01, "核心数据应正确");
    
    // 验证校验位
    uint8_t expected_checksum = 0x01 ^ 0x05 ^ 0x01 ^ 0x01;
    TEST_ASSERT(send_buff[6] == expected_checksum, "校验位应正确");
    
    // 验证帧尾
    TEST_ASSERT(send_buff[7] == 0xCC && send_buff[8] == 0xDD, "帧尾应正确");
    
    atlvc_deinit(&ctx);
}

void test_pack_buffer_overflow(void) {
    printf("\n===== 测试缓冲区溢出保护 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[5] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01, 0x02, 0x03};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试缓冲区不足
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 5, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_DATA_LEN_ERR, "缓冲区不足应失败");
    
    atlvc_deinit(&ctx);
}

void test_pack_null_parameters(void) {
    printf("\n===== 测试空参数保护 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    // 测试空帧指针
    err = atlvc_pack(&ctx, NULL, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空帧指针应失败");
    
    // 测试空缓冲区指针
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, NULL, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空缓冲区指针应失败");
    
    // 测试空长度指针
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, NULL);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空长度指针应失败");
    
    // 测试零缓冲区长度
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 0, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "零缓冲区长度应失败");
    
    atlvc_deinit(&ctx);
}

void test_pack_zero_data(void) {
    printf("\n===== 测试零数据打包 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = 0,
        .data = NULL,
    };
    
    // 测试零数据打包
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "零数据打包应成功");
    TEST_ASSERT(send_len == 4, "零数据打包长度应为4（3+0+1）");
    
    // 验证数据
    TEST_ASSERT(send_buff[0] == 0x01, "地址应正确");
    TEST_ASSERT(send_buff[1] == 0x01, "指令应正确");
    TEST_ASSERT(send_buff[2] == 0x00, "长度应正确");
    
    // 验证校验位
    uint8_t expected_checksum = 0x01 ^ 0x01 ^ 0x00;
    TEST_ASSERT(send_buff[3] == expected_checksum, "校验位应正确");
    
    atlvc_deinit(&ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  ATLVC 打包功能测试\n");
    printf("========================================\n");
    
    test_pack_xor();
    test_pack_crc8();
    test_pack_sum();
    test_pack_no_checksum();
    test_pack_with_boundary();
    test_pack_buffer_overflow();
    test_pack_null_parameters();
    test_pack_zero_data();
    
    printf("\n========================================\n");
    printf("  测试结果统计\n");
    printf("========================================\n");
    printf("  通过: %d\n", test_passed);
    printf("  失败: %d\n", test_failed);
    printf("  总计: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}