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

void test_init_with_boundary(void) {
    printf("\n===== 测试带帧头帧尾初始化 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    // 定义帧头帧尾配置
    const atlvc_frame_boundary_t boundary = {
        .header_len = 2,
        .header = {0xAA, 0xBB},
        .trailer_len = 2,
        .trailer = {0xCC, 0xDD}
    };
    
    // 测试正常初始化
    err = atlvc_init_with_boundary(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE, &boundary);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常初始化带帧头帧尾");
    
    // 测试反初始化
    err = atlvc_deinit(&ctx);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "反初始化成功");
    
    // 测试空指针初始化
    err = atlvc_init_with_boundary(NULL, test_cmd_table, TEST_CMD_TABLE_SIZE, &boundary);
    TEST_ASSERT(err == ATLVC_ERR_INPUT_NULL, "空指针初始化应失败");
}

void test_process_with_boundary(void) {
    printf("\n===== 测试带帧头帧尾处理 =====\n");
    
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
    
    // 测试正常帧（带帧头帧尾）
    // 完整帧：帧头(0xAA 0xBB) + 地址(0x01) + 指令(0x01) + 长度(0x01) + 数据(0x01) + XOR校验(0x00) + 帧尾(0xCC 0xDD)
    uint8_t valid_frame[] = {0xAA, 0xBB, 0x01, 0x01, 0x01, 0x01, 0x00, 0xCC, 0xDD};
    err = atlvc_process(&ctx, valid_frame, sizeof(valid_frame));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常帧（带帧头帧尾）处理应成功");
    
    // 测试帧头错误
    uint8_t bad_header[] = {0xAB, 0xBB, 0x01, 0x01, 0x01, 0x01, 0x00, 0xCC, 0xDD};
    err = atlvc_process(&ctx, bad_header, sizeof(bad_header));
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "帧头错误应失败");
    
    // 测试帧尾错误
    uint8_t bad_trailer[] = {0xAA, 0xBB, 0x01, 0x01, 0x01, 0x01, 0x00, 0xCD, 0xDD};
    err = atlvc_process(&ctx, bad_trailer, sizeof(bad_trailer));
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "帧尾错误应失败");
    
    // 测试帧头长度不足
    uint8_t short_header[] = {0xAA, 0x01, 0x01, 0x01, 0x01, 0x00, 0xCC, 0xDD};
    err = atlvc_process(&ctx, short_header, sizeof(short_header));
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "帧头长度不足应失败");
    
    // 测试帧尾长度不足
    uint8_t short_trailer[] = {0xAA, 0xBB, 0x01, 0x01, 0x01, 0x01, 0x00, 0xCC};
    err = atlvc_process(&ctx, short_trailer, sizeof(short_trailer));
    TEST_ASSERT(err == ATLVC_ERR_INVALID_FRAME, "帧尾长度不足应失败");
    
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
    
    // 测试打包
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "打包应成功");
    TEST_ASSERT(send_len == 9, "打包长度应为9（2帧头+4核心+2帧尾+1校验）");
    
    // 验证帧头
    TEST_ASSERT(send_buff[0] == 0xAA && send_buff[1] == 0xBB, "帧头应正确");
    
    // 验证帧尾
    TEST_ASSERT(send_buff[7] == 0xCC && send_buff[8] == 0xDD, "帧尾应正确");
    
    // 验证核心数据
    TEST_ASSERT(send_buff[2] == 0x01 && send_buff[3] == 0x01 && 
                send_buff[4] == 0x01 && send_buff[5] == 0x01, "核心数据应正确");
    
    // 验证校验位
    uint8_t expected_checksum = 0x01 ^ 0x01 ^ 0x01 ^ 0x01;  // XOR
    TEST_ASSERT(send_buff[6] == expected_checksum, "校验位应正确");
    
    atlvc_deinit(&ctx);
}

void test_no_boundary_mode(void) {
    printf("\n===== 测试无帧头帧尾模式 =====\n");
    
    atlvc_context_t ctx;
    atlvc_err_t err;
    
    // 无帧头帧尾初始化
    err = atlvc_init(&ctx, test_cmd_table, TEST_CMD_TABLE_SIZE);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "无帧头帧尾初始化成功");
    
    // 测试正常帧（无帧头帧尾）
    uint8_t valid_frame[] = {0x01, 0x01, 0x01, 0x01, 0x00};
    err = atlvc_process(&ctx, valid_frame, sizeof(valid_frame));
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "正常帧（无帧头帧尾）处理应成功");
    
    // 测试打包（无帧头帧尾）
    uint8_t send_buff[20] = {0};
    uint16_t send_len = 0;
    uint8_t test_data[] = {0x01};
    atlvc_frame_t test_frame = {
        .address = 0x01,
        .cmd = 0x01,
        .length = sizeof(test_data),
        .data = test_data,
    };
    
    err = atlvc_pack(&ctx, &test_frame, ATLVC_CHECKSUM_XOR, send_buff, 20, &send_len);
    TEST_ASSERT(err == ATLVC_ERR_SUCCESS, "打包应成功");
    TEST_ASSERT(send_len == 5, "打包长度应为5（4核心+1校验）");
    
    atlvc_deinit(&ctx);
}

int main(void) {
    printf("========================================\n");
    printf("  ATLVC 帧头帧尾功能测试\n");
    printf("========================================\n");
    
    test_init_with_boundary();
    test_process_with_boundary();
    test_pack_with_boundary();
    test_no_boundary_mode();
    
    printf("\n========================================\n");
    printf("  测试结果统计\n");
    printf("========================================\n");
    printf("  通过: %d\n", test_passed);
    printf("  失败: %d\n", test_failed);
    printf("  总计: %d\n", test_passed + test_failed);
    printf("========================================\n");
    
    return (test_failed == 0) ? 0 : 1;
}