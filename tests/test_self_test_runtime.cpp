// tests/test_self_test_runtime.cpp
// SelfTestRuntime 单元测试
#include "layer2/self_test_runtime.h"
#include "../generated/signal_def.h"

#include <cstdio>
#include <cstring>

#define TEST_ASSERT(cond) do { \
    if (!(cond)) { \
        std::fprintf(stderr, "  ✗ ASSERT FAIL @ %s:%d: %s\n", __FILE__, __LINE__, #cond); \
        return 1; \
    } \
} while(0)

#define TEST_OK(name) std::printf("  ✓ %s\n", name)

int test_not_ready_initially() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    TEST_ASSERT(st.status() == candash::SelfTestStatus::NOT_READY);
    TEST_ASSERT(!st.isReady());
    TEST_OK("init() 后状态为 NOT_READY");
    return 0;
}

int test_ready_after_critical_signals() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    // 喂入所有关键信号
    st.onValueChanged("vehicle_speed", 60.0f, 1000);
    st.onValueChanged("bat_volt", 350.0f, 1000);
    st.onValueChanged("bat_soc", 75.0f, 1000);
    st.onValueChanged("motor_rpm", 3000, 1000);
    st.onValueChanged("motor_temp", 45.0f, 1000);
    st.tick(1100);
    TEST_ASSERT(st.isReady());
    TEST_ASSERT(st.status() == candash::SelfTestStatus::OK);
    TEST_OK("所有关键信号收到后 → OK");
    return 0;
}

int test_warn_when_non_critical_stuck() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    // 喂入所有关键信号
    st.onValueChanged("vehicle_speed", 60.0f, 0);
    st.onValueChanged("bat_volt", 350.0f, 0);
    st.onValueChanged("bat_soc", 75.0f, 0);
    st.onValueChanged("motor_rpm", 3000, 0);
    st.onValueChanged("motor_temp", 45.0f, 0);
    // 一些非关键信号保持 last_update=0
    // tick 3 秒后，关键信号 timeout 是 500ms~2s，已经过期
    st.tick(3000);
    char buf[256]; st.getSummary(buf, sizeof(buf));
    std::printf("    debug: %s\n", buf);
    // 期望：critical_stuck > 0 → FAIL
    TEST_ASSERT(st.status() == candash::SelfTestStatus::FAIL);
    TEST_OK("关键信号 3s 没更新 → FAIL");
    return 0;
}

int test_out_of_range_detection() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    // 喂入所有关键信号（让 isReady() = true）
    st.onValueChanged("vehicle_speed", 60.0f, 1000);
    st.onValueChanged("bat_volt", 350.0f, 1000);
    st.onValueChanged("bat_soc", 75.0f, 1000);   // 正常
    st.onValueChanged("motor_rpm", 3000, 1000);
    st.onValueChanged("motor_temp", 45.0f, 1000);
    // 然后把 bat_soc 改成越界值
    st.onValueChanged("bat_soc", 150.0f, 1100);  // 越界（max=100）
    st.tick(1200);
    char buf[256]; st.getSummary(buf, sizeof(buf));
    std::printf("    debug: %s\n", buf);
    // 1 个越界 → WARN（<3）
    TEST_ASSERT(st.status() == candash::SelfTestStatus::WARN);
    TEST_OK("单个信号越界 → WARN");
    return 0;
}

int test_summary_output() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    st.onValueChanged("vehicle_speed", 60.0f, 0);
    st.onValueChanged("bat_volt", 350.0f, 0);
    st.onValueChanged("bat_soc", 75.0f, 0);
    st.onValueChanged("motor_rpm", 3000, 0);
    st.onValueChanged("motor_temp", 45.0f, 0);
    st.tick(100);
    char buf[256];
    st.getSummary(buf, sizeof(buf));
    std::printf("    summary: %s\n", buf);
    TEST_ASSERT(std::strstr(buf, "status=OK") != nullptr);
    TEST_ASSERT(std::strstr(buf, "critical_received=5/5") != nullptr);
    TEST_OK("getSummary 格式正确");
    return 0;
}

int test_reset() {
    candash::SelfTestRuntime st;
    st.init(SIGNAL_TABLE, SIGNAL_TABLE_COUNT);
    st.onValueChanged("vehicle_speed", 60.0f, 0);
    st.onValueChanged("bat_volt", 350.0f, 0);
    st.onValueChanged("bat_soc", 75.0f, 0);
    st.onValueChanged("motor_rpm", 3000, 0);
    st.onValueChanged("motor_temp", 45.0f, 0);
    st.tick(100);
    TEST_ASSERT(st.isReady());
    st.reset();
    TEST_ASSERT(!st.isReady());
    TEST_ASSERT(st.status() == candash::SelfTestStatus::NOT_READY);
    TEST_OK("reset() 后回到 NOT_READY");
    return 0;
}

int main() {
    std::printf("=== SelfTestRuntime 单元测试 ===\n");
    int rc = 0;
    rc |= test_not_ready_initially();
    rc |= test_ready_after_critical_signals();
    rc |= test_warn_when_non_critical_stuck();
    rc |= test_out_of_range_detection();
    rc |= test_summary_output();
    rc |= test_reset();
    if (rc == 0) {
        std::printf("所有测试通过。\n");
        return 0;
    } else {
        std::fprintf(stderr, "测试失败 rc=%d\n", rc);
        return 1;
    }
}
