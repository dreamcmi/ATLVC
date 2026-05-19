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
    // 单一地址匹配
    {
        .addr_match_type = ATLVC_ADDR_SINGLE,
        .addr_pattern = 0x01,
        .addr_end = 0,
        .cmd = 0x01,
        .min_len = 1,
        .max_len = 1,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = simple_callback,
        .desc = "Single Address",
        .user_data = NULL
    },
    // 地址范围匹配
    {
        .addr_match_type = ATLVC_ADDR_RANGE,
        .addr_pattern = 0x10,
        .addr_end = 0x1F,
        .cmd = 0x02,
        .min_len = 1,
        .max_len = 1,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = simple_callback,
        .desc = "Address Range",
        .user_data = NULL
    },
    // 地址掩码匹配
    {
        .addr_match_type = ATLVC_ADDR_MASK,
        .addr_pattern = 0xF0,
        .addr_end = 0xF0,
        .cmd = 0x03,
        .min_len = 1,
        .max_len = 1,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = simple_callback,
        .desc = "Address Mask",
        .user_data = NULL
    }
};

#define TEST_CMD_TABLE_SIZE (sizeof(test_cmd_table) / sizeof(atlvc_cmd_rule_t))

void test_single_address_match(void) {
    printf("\n===== 测试单一地址匹配 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试匹配地址
    uint8_t match_frame[] = {0x01, 0x01, 0x01, 0x01, 0x00};
    err = atlvc_process(&ctx, match_frame, sizeof(match_frame));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "匹配地址应成功");
    
    // 测试不匹配地址
    uint8_t no_match_frame[] = {0x02, 0x01, 0x01, 0x02, 0x00};
    err = atlvc_process(&ctx, no_match_frame, sizeof(no_match_frame));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "不匹配地址应失败");
    
    atlvc_deinit(&ctx);
}

void test_address_range_match(void) {
    printf("\n===== 测试地址范围匹配 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试范围内地址（0x10）
    uint8_t in_range1[] = {0x10, 0x02, 0x01, 0x01, 0x12};
    err = atlvc_process(&ctx, in_range1, sizeof(in_range1));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "范围内地址0x10应匹配");
    
    // 测试范围内地址（0x15）
    uint8_t in_range2[] = {0x15, 0x02, 0x01, 0x01, 0x17};
    err = atlvc_process(&ctx, in_range2, sizeof(in_range2));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "范围内地址0x15应匹配");
    
    // 测试范围内地址（0x1F）
    // XOR: 0x1F ^ 0x02 ^ 0x01 ^ 0x01 = 0x1D
    uint8_t in_range3[] = {0x1F, 0x02, 0x01, 0x01, 0x1D};
    err = atlvc_process(&ctx, in_range3, sizeof(in_range3));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "范围内地址0x1F应匹配");
    
    // 测试范围外地址（0x0F）
    uint8_t out_range1[] = {0x0F, 0x02, 0x01, 0x01, 0x0B};
    err = atlvc_process(&ctx, out_range1, sizeof(out_range1));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "范围外地址0x0F应不匹配");
    
    // 测试范围外地址（0x20）
    uint8_t out_range2[] = {0x20, 0x02, 0x01, 0x01, 0x22};
    err = atlvc_process(&ctx, out_range2, sizeof(out_range2));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "范围外地址0x20应不匹配");
    
    atlvc_deinit(&ctx);
}

void test_address_mask_match(void) {
    printf("\n===== 测试地址掩码匹配 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试匹配地址（0xF0）
    // XOR: 0xF0 ^ 0x03 ^ 0x01 ^ 0x01 = 0xF3
    uint8_t match1[] = {0xF0, 0x03, 0x01, 0x01, 0xF3};
    err = atlvc_process(&ctx, match1, sizeof(match1));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0xF0应匹配");
    
    // 测试匹配地址（0xF5）
    // XOR: 0xF5 ^ 0x03 ^ 0x01 ^ 0x01 = 0xF6
    uint8_t match2[] = {0xF5, 0x03, 0x01, 0x01, 0xF6};
    err = atlvc_process(&ctx, match2, sizeof(match2));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0xF5应匹配");
    
    // 测试匹配地址（0xFF）
    // XOR: 0xFF ^ 0x03 ^ 0x01 ^ 0x01 = 0xFC
    uint8_t match3[] = {0xFF, 0x03, 0x01, 0x01, 0xFC};
    err = atlvc_process(&ctx, match3, sizeof(match3));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0xFF应匹配");
    
    // 测试不匹配地址（0x0F）
    uint8_t no_match1[] = {0x0F, 0x03, 0x01, 0x01, 0x0D};
    err = atlvc_process(&ctx, no_match1, sizeof(no_match1));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "地址0x0F应不匹配");
    
    // 测试不匹配地址（0x10）
    uint8_t no_match2[] = {0x10, 0x03, 0x01, 0x01, 0x12};
    err = atlvc_process(&ctx, no_match2, sizeof(no_match2));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "地址0x10应不匹配");
    
    atlvc_deinit(&ctx);
}

void test_wildcard_match(void) {
    printf("\n===== 测试通配符匹配 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    // 创建带通配符的指令表
    const atlvc_cmd_rule_t wildcard_table[] = {
        {
            .addr_match_type = ATLVC_ADDR_WILDCARD,
            .addr_pattern = 0xFF,
            .addr_end = 0,
            .cmd = 0x04,
            .min_len = 1,
            .max_len = 1,
            .cs_type = ATLVC_CHECKSUM_XOR,
            .handler = simple_callback,
            .desc = "Wildcard",
            .user_data = NULL
        }
    };
    
    err = atlvc_init(&ctx, wildcard_table, 1);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试任意地址匹配
    // XOR: 0x01 ^ 0x04 ^ 0x01 ^ 0x01 = 0x05
    uint8_t addr1[] = {0x01, 0x04, 0x01, 0x01, 0x05};
    err = atlvc_process(&ctx, addr1, sizeof(addr1));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0x01应匹配");
    
    // XOR: 0xFF ^ 0x04 ^ 0x01 ^ 0x01 = 0xFB
    uint8_t addr2[] = {0xFF, 0x04, 0x01, 0x01, 0xFB};
    err = atlvc_process(&ctx, addr2, sizeof(addr2));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0xFF应匹配");
    
    // XOR: 0x00 ^ 0x04 ^ 0x01 ^ 0x01 = 0x04
    uint8_t addr3[] = {0x00, 0x04, 0x01, 0x01, 0x04};
    err = atlvc_process(&ctx, addr3, sizeof(addr3));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "地址0x00应匹配");
    
    atlvc_deinit(&ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  ATLVC 地址匹配功能测试\n");
    printf("========================================\n");
    
    test_single_address_match();
    test_address_range_match();
    test_address_mask_match();
    test_wildcard_match();
    
    printf("\n========================================\n");
    printf("  测试结果统计\n");
    printf("========================================\n");
    printf("  通过: %d\n", test_passed);
    printf("  失败: %d\n", test_failed);
    printf("  总计: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}