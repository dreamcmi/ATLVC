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

void test_xor_checksum(void) {
    printf("\n===== 测试XOR校验 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_cmd_rule_t xor_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_SINGLE,
            .addr_pattern = 0x01,
            .addr_end = 0,
            .cmd = 0x01,
            .min_len = 1,
            .max_len = 1,
            .cs_type = ATLVC_CHECKSUM_XOR,
            .handler = simple_callback,
            .desc = "XOR Checksum",
            .user_data = NULL
        }
    };
    
    err = atlvc_init(&ctx, xor_table, 1);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试正确的XOR校验
    // XOR: 0x01 ^ 0x01 ^ 0x01 ^ 0x01 = 0x00
    uint8_t valid_xor[] = {0x01, 0x01, 0x01, 0x01, 0x00};
    err = atlvc_process(&ctx, valid_xor, sizeof(valid_xor));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正确的XOR校验应成功");
    
    // 测试错误的XOR校验
    uint8_t invalid_xor[] = {0x01, 0x01, 0x01, 0x01, 0xFF};
    err = atlvc_process(&ctx, invalid_xor, sizeof(invalid_xor));
    TEST_ASSERT(err == ATLVC_ERR_CHECKSUM_ERR, "错误的XOR校验应失败");
    
    // 测试不同数据的XOR校验
    // XOR: 0x02 ^ 0x02 ^ 0x02 ^ 0x02 = 0x00
    uint8_t xor2[] = {0x02, 0x02, 0x02, 0x02, 0x00};
    err = atlvc_process(&ctx, xor2, sizeof(xor2));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "不匹配的指令应失败");
    
    atlvc_deinit(&ctx);
}

void test_crc8_checksum(void) {
    printf("\n===== 测试CRC8校验 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_cmd_rule_t crc8_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_SINGLE,
            .addr_pattern = 0x01,
            .addr_end = 0,
            .cmd = 0x02,
            .min_len = 3,
            .max_len = 3,
            .cs_type = ATLVC_CHECKSUM_CRC8,
            .handler = simple_callback,
            .desc = "CRC8 Checksum",
            .user_data = NULL
        }
    };
    
    err = atlvc_init(&ctx, crc8_table, 1);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试正确的CRC8校验
    // 数据长度为3：{0x00, 0x00, 0x00}
    // CRC8 of {0x01, 0x02, 0x03, 0x00, 0x00, 0x00} = 0x49
    uint8_t valid_crc8[] = {0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x49};
    err = atlvc_process(&ctx, valid_crc8, sizeof(valid_crc8));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正确的CRC8校验应成功");
    
    // 测试错误的CRC8校验
    uint8_t invalid_crc8[] = {0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0xFF};
    err = atlvc_process(&ctx, invalid_crc8, sizeof(invalid_crc8));
    TEST_ASSERT(err == ATLVC_ERR_CHECKSUM_ERR, "错误的CRC8校验应失败");
    
    atlvc_deinit(&ctx);
}

void test_sum_checksum(void) {
    printf("\n===== 测试SUM校验 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_cmd_rule_t sum_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_SINGLE,
            .addr_pattern = 0xF2,
            .addr_end = 0,
            .cmd = 0x03,
            .min_len = 1,
            .max_len = 1,
            .cs_type = ATLVC_CHECKSUM_SUM,
            .handler = simple_callback,
            .desc = "SUM Checksum",
            .user_data = NULL
        }
    };
    
    err = atlvc_init(&ctx, sum_table, 1);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试正确的SUM校验
    // SUM: (0xF2 + 0x03 + 0x01 + 0x01) & 0xFF = 0xF7
    uint8_t valid_sum[] = {0xF2, 0x03, 0x01, 0x01, 0xF7};
    err = atlvc_process(&ctx, valid_sum, sizeof(valid_sum));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正确的SUM校验应成功");
    
    // 测试错误的SUM校验
    uint8_t invalid_sum[] = {0xF2, 0x03, 0x01, 0x01, 0xFF};
    err = atlvc_process(&ctx, invalid_sum, sizeof(invalid_sum));
    TEST_ASSERT(err == ATLVC_ERR_CHECKSUM_ERR, "错误的SUM校验应失败");
    
    atlvc_deinit(&ctx);
}

void test_no_checksum(void) {
    printf("\n===== 测试无校验模式 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_cmd_rule_t no_checksum_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_SINGLE,
            .addr_pattern = 0x01,
            .addr_end = 0,
            .cmd = 0x04,
            .min_len = 1,
            .max_len = 1,
            .cs_type = ATLVC_CHECKSUM_NONE,
            .handler = simple_callback,
            .desc = "No Checksum",
            .user_data = NULL
        }
    };
    
    err = atlvc_init(&ctx, no_checksum_table, 1);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试无校验模式（无校验位）
    uint8_t no_checksum_frame[] = {0x01, 0x04, 0x01, 0x01};
    err = atlvc_process(&ctx, no_checksum_frame, sizeof(no_checksum_frame));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "无校验模式应成功");
    
    // 测试无校验模式（有校验位应失败）
    uint8_t with_checksum_frame[] = {0x01, 0x04, 0x01, 0x01, 0x00};
    err = atlvc_process(&ctx, with_checksum_frame, sizeof(with_checksum_frame));
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "无校验模式不应有校验位");
    
    atlvc_deinit(&ctx);
}

void test_checksum_with_boundary(void) {
    printf("\n===== 测试带帧头帧尾的校验 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    const atlvc_cmd_rule_t checksum_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_SINGLE,
            .addr_pattern = 0x01,
            .addr_end = 0,
            .cmd = 0x05,
            .min_len = 1,
            .max_len = 1,
            .cs_type = ATLVC_CHECKSUM_XOR,
            .handler = simple_callback,
            .desc = "Checksum with Boundary",
            .user_data = NULL
        }
    };
    
    const atlvc_frame_boundary_t boundary = {
        .header_len = 2,
        .header = {0xAA, 0xBB},
        .trailer_len = 2,
        .trailer = {0xCC, 0xDD}
    };
    
    err = atlvc_init_with_boundary(&ctx, checksum_table, 1, &boundary);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试带帧头帧尾的XOR校验
    // XOR: 0x01 ^ 0x05 ^ 0x01 ^ 0x01 = 0x04
    uint8_t frame_with_boundary[] = {0xAA, 0xBB, 0x01, 0x05, 0x01, 0x01, 0x04, 0xCC, 0xDD};
    err = atlvc_process(&ctx, frame_with_boundary, sizeof(frame_with_boundary));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "带帧头帧尾的校验应成功");
    
    // 测试带帧头帧尾的错误校验
    uint8_t bad_checksum_frame[] = {0xAA, 0xBB, 0x01, 0x05, 0x01, 0x01, 0xFF, 0xCC, 0xDD};
    err = atlvc_process(&ctx, bad_checksum_frame, sizeof(bad_checksum_frame));
    TEST_ASSERT(err == ATLVC_ERR_CHECKSUM_ERR, "带帧头帧尾的错误校验应失败");
    
    atlvc_deinit(&ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  ATLVC 校验功能测试\n");
    printf("========================================\n");
    
    test_xor_checksum();
    test_crc8_checksum();
    test_sum_checksum();
    test_no_checksum();
    test_checksum_with_boundary();
    
    printf("\n========================================\n");
    printf("  测试结果统计\n");
    printf("========================================\n");
    printf("  通过: %d\n", test_passed);
    printf("  失败: %d\n", test_failed);
    printf("  总计: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}