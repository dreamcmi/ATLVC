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
        .min_len = 1,
        .max_len = 1,
        .cs_type = ATLVC_CHECKSUM_XOR,
        .handler = simple_callback,
        .desc = "Test Command",
        .user_data = NULL
    }
};

#define TEST_CMD_TABLE_SIZE (sizeof(test_cmd_table) / sizeof(atlvc_cmd_rule_t))

void test_init_deinit(void) {
    printf("\n===== 测试初始化和反初始化 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    // 测试正常初始化
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常初始化");
    
    // 测试反初始化
    err = atlvc_deinit(&ctx);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常反初始化");
    
    // 测试空指针初始化
    err = atlvc_init(NULL, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空指针初始化应失败");
    
    // 测试空表初始化
    err = atlvc_init(&ctx, NULL, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空表初始化应失败");
    
    // 测试零长度初始化
    err = atlvc_init(&ctx, test_cmd_table, 0);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "零长度初始化应失败");
}

void test_process_basic(void) {
    printf("\n===== 测试基本处理流程 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试空指针处理
    err = atlvc_process(NULL, (uint8_t[]){0x01, 0x01, 0x01, 0x01, 0x00}, 5);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空上下文处理应失败");
    
    err = atlvc_process(&ctx, NULL, 5);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空数据指针处理应失败");
    
    err = atlvc_process(&ctx, (uint8_t[]){0x01, 0x01, 0x01, 0x01, 0x00}, 0);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "零长度处理应失败");
    
    // 测试帧长度不足
    err = atlvc_process(&ctx, (uint8_t[]){0x01, 0x01}, 2);
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "帧长度不足应失败");
    
    // 测试正常处理
    uint8_t valid_frame[] = {0x01, 0x01, 0x01, 0x01, 0x00};  // XOR: 0x01^0x01^0x01^0x01 = 0x00
    err = atlvc_process(&ctx, valid_frame, sizeof(valid_frame));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常帧处理应成功");
    
    // 测试未知指令
    uint8_t unknown_cmd[] = {0x01, 0xFF, 0x01, 0x01, 0xFE};
    err = atlvc_process(&ctx, unknown_cmd, sizeof(unknown_cmd));
    TEST_ASSERT(err == ATLVC_ERR_CMD_NOT_FOUND, "未知指令应失败");
    
    // 测试校验错误
    uint8_t bad_checksum[] = {0x01, 0x01, 0x01, 0x01, 0xFF};
    err = atlvc_process(&ctx, bad_checksum, sizeof(bad_checksum));
    TEST_ASSERT(err == ATLVC_ERR_CHECKSUM_ERR, "校验错误应失败");
    
    atlvc_deinit(&ctx);
}

void test_data_length_validation(void) {
    printf("\n===== 测试数据长度验证 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "初始化成功");
    
    // 测试数据长度过小
    uint8_t short_data[] = {0x01, 0x01, 0x00, 0x00};  // 长度0，但min_len=1
    err = atlvc_process(&ctx, short_data, sizeof(short_data));
    TEST_ASSERT(err == ATLVC_ERR_DATA_LEN_ERR, "数据长度过小应失败");
    
    // 测试数据长度过大
    uint8_t long_data[] = {0x01, 0x01, 0x02, 0x01, 0x02, 0x00};
    err = atlvc_process(&ctx, long_data, sizeof(long_data));
    TEST_ASSERT(err == ATLVC_ERR_DATA_LEN_ERR, "数据长度过大应失败");
    
    atlvc_deinit(&ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  ATLVC 基本功能测试\n");
    printf("========================================\n");
    
    test_init_deinit();
    test_process_basic();
    test_data_length_validation();
    
    printf("\n========================================\n");
    printf("  测试结果统计\n");
    printf("========================================\n");
    printf("  通过: %d\n", test_passed);
    printf("  失败: %d\n", test_failed);
    printf("  总计: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}