// test_self_test_data_flow.cpp
// SelfTestRuntime 数据流集成测试 (PR 17)
//
// 覆盖 (跟 ShmDataSource 16ms tick 节奏同步):
// 1. 首次 onTick (所有 14 信号从 shm feed) → status=OK(1), 5 critical 都收到
// 2. shm bat_soc=150 (越界 max=100) → status=WARN(2), out_of_range=1
// 3. shm vehicle_speed=500 (越界 max=300) → status=WARN(2)
// 4. resetSelfTestForTest → 推新越界值 → status=WARN (reset 不破坏后续数据流)
// 5. 全链透传: ShmDataSource → QtDataBinder → DashboardBackend Q_PROPERTY (OK 状态)
// 6. 全链透传: Q_PROPERTY 在 WARN 状态下正确反映
//
// 设计要点:
// - 复用 tests/test_chime_data_flow.cpp 的 writeShmFrame() 模式 (16ms tick 节奏)
// - 复用 ShmDataSource::pushSignalValueForTest() 注入信号值 (跟生产路径一致)
// - 用 tickSelfTestForTest() 强制触发 L2 evaluateStatus (绕过 1Hz 节流)
// - 不依赖 QML, 只验证 L3 DisplaySnapshot.self_test 字段正确填充 + DashboardBackend 透传
//
// ⚠️ 测试设计权衡:
// - L2 "stuck" 状态 (timeout 检测) 难以通过 snapshot 验证: onTick 每次都重新 feed 所有 14 信号
//   (用当前 commit_ms), 所以 onTick 后 last_update_ms 总是新的, age 总是 0, 无法触发 stuck
// - 改用 shm-driven 越界测试 (每次 onTick 都会重新评估 out_of_range)
// - 真正的 stuck 测试要走 mock shm 或直接调 m_self_test (需要 L3 暴露更多 getter)
//
// 性能预算: 6 个测试 case < 200ms (主要是 sleep_for 16ms × 几次)

#include "layer3/shm_data_source.h"
#include "layer3/dashboard_backend.h"
#include "layer3/qt_data_binder.h"
#include "layer3/display_data_types.h"
#include "layer1/shm/shm_display.h"
#include "layer2/time_util.h"
#include "layer2/self_test_runtime.h"
#include "../generated/signal_def.h"
#include "mock_data_binder.h"

#include <QCoreApplication>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <unistd.h>

static int g_test_count = 0;
static int g_test_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_test_count++; \
    if (cond) { \
        g_test_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        printf("  ✗ %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

namespace {

// 喂一组基础正常值 (所有 14 信号都在 SIGNAL_TABLE 范围内)
// vehicle_speed=60, bat_volt=350, bat_soc=75, motor_rpm=3000, motor_temp=45
void writeShmFrameAllOK(uint32_t frame_seq) {
    shm_display_close();
    unlink(SHM_DISPLAY_PATH);
    shm_display_create();
    DisplayDataShm shm = {};
    shm.magic = SHM_MAGIC;
    shm.version = SHM_VERSION;
    shm.last_commit_ms = candash::now_monotonic_ms();
    shm.updated_mask = 0xFFFFFFFF;
    shm.frame_seq = frame_seq;
    shm.vehicle_speed = 60.0f;
    shm.bat_volt = 350.0f;
    shm.bat_soc = 75;
    shm.motor_rpm = 3000.0f;
    shm.motor_temp = 45.0f;
    shm.alarm_active = 0;
    shm.checksum = shm_display_compute_checksum(&shm);
    shm_display_write(&shm);
}

// 喂一组 bat_soc 越界 (150 > max=100), 其他正常
void writeShmFrameBatSocOutOfRange(uint32_t frame_seq) {
    shm_display_close();
    unlink(SHM_DISPLAY_PATH);
    shm_display_create();
    DisplayDataShm shm = {};
    shm.magic = SHM_MAGIC;
    shm.version = SHM_VERSION;
    shm.last_commit_ms = candash::now_monotonic_ms();
    shm.updated_mask = 0xFFFFFFFF;
    shm.frame_seq = frame_seq;
    shm.vehicle_speed = 60.0f;
    shm.bat_volt = 350.0f;
    shm.bat_soc = 150;  // 越界! (max=100)
    shm.motor_rpm = 3000.0f;
    shm.motor_temp = 45.0f;
    shm.alarm_active = 0;
    shm.checksum = shm_display_compute_checksum(&shm);
    shm_display_write(&shm);
}

// 喂一组 vehicle_speed 越界 (500 > max=300), 其他正常
void writeShmFrameSpeedOutOfRange(uint32_t frame_seq) {
    shm_display_close();
    unlink(SHM_DISPLAY_PATH);
    shm_display_create();
    DisplayDataShm shm = {};
    shm.magic = SHM_MAGIC;
    shm.version = SHM_VERSION;
    shm.last_commit_ms = candash::now_monotonic_ms();
    shm.updated_mask = 0xFFFFFFFF;
    shm.frame_seq = frame_seq;
    shm.vehicle_speed = 500.0f;  // 越界! (max=300)
    shm.bat_volt = 350.0f;
    shm.bat_soc = 75;
    shm.motor_rpm = 3000.0f;
    shm.motor_temp = 45.0f;
    shm.alarm_active = 0;
    shm.checksum = shm_display_compute_checksum(&shm);
    shm_display_write(&shm);
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    printf("\n=== SelfTestRuntime 数据流测试 (PR 17) ===\n");

    shm_display_close();  // 初始清理

    // ─── Test 1: 首次 onTick → status=OK (5 critical 都收到, 所有信号在范围) ───
    printf("\n[1] 首次 onTick (shm 喂所有正常值) → status=OK(1):\n");
    {
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.start();

        writeShmFrameAllOK(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s = binder.lastSnapshot();

        TEST_ASSERT(s.self_test.status            == 1, "首次 onTick (5 critical 收到 + 在范围) → status = OK (1)");
        TEST_ASSERT(s.self_test.critical_received >= 5, "critical_received ≥ 5");
        TEST_ASSERT(s.self_test.critical_total    >= 5, "critical_total ≥ 5 (vehicle_speed/bat_volt/bat_soc/motor_rpm/motor_temp)");
        TEST_ASSERT(s.self_test.critical_stuck    == 0, "无 critical 卡死");
        TEST_ASSERT(s.self_test.warn_stuck        == 0, "无 warn 卡死");
        TEST_ASSERT(s.self_test.out_of_range      == 0, "无越界");

        src.stop();
    }

    // ─── Test 2: shm bat_soc=150 越界 → status=WARN(2), out_of_range=1 ───
    printf("\n[2] shm bat_soc=150 (越界 max=100) → status=WARN(2), out_of_range=1:\n");
    {
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.start();

        writeShmFrameBatSocOutOfRange(2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s = binder.lastSnapshot();

        TEST_ASSERT(s.self_test.out_of_range >= 1, "1 个信号越界 (bat_soc=150 > max=100)");
        TEST_ASSERT(s.self_test.status       == 2, "单个越界 → status = WARN (2) (out_of_range < 3 → WARN, 不是 FAIL)");

        src.stop();
    }

    // ─── Test 3: shm vehicle_speed=500 越界 → status=WARN(2) ───
    printf("\n[3] shm vehicle_speed=500 (越界 max=300) → status=WARN(2):\n");
    {
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.start();

        writeShmFrameSpeedOutOfRange(3);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s = binder.lastSnapshot();

        TEST_ASSERT(s.self_test.out_of_range >= 1, "1 个信号越界 (vehicle_speed=500 > max=300)");
        TEST_ASSERT(s.self_test.status       == 2, "vehicle_speed 越界 → status = WARN (2)");

        src.stop();
    }

    // ─── Test 4: resetSelfTestForTest → 推越界值, 验证 reset 不破坏数据流 ───
    // 注意: L2 reset() 清理 ever_received + 计数, 但下次 onTick 会立即 re-feed 所有 14 信号
    // 所以"reset 后 status=NOT_READY"无法通过 snapshot 观察 (下次 tick 就回到 OK)
    // 此测试改为: reset 后推越界值 → status=WARN (验证 reset 不破坏后续数据流)
    printf("\n[4] resetSelfTestForTest → 推越界值, 验证 reset 不破坏数据流:\n");
    {
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.start();

        // 先建立 OK 状态
        writeShmFrameAllOK(4);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s1 = binder.lastSnapshot();
        TEST_ASSERT(s1.self_test.status == 1, "reset 前 status = OK (1)");

        // reset
        src.resetSelfTestForTest();
        // 推越界值 (bat_soc=150 > max=100) + tick 评估
        src.pushSignalValueForTest("bat_soc", 150.0f, candash::now_monotonic_ms());
        src.tickSelfTestForTest(candash::now_monotonic_ms());
        writeShmFrameBatSocOutOfRange(5);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s2 = binder.lastSnapshot();

        // reset 后, 推新越界值 → status=WARN (reset 不破坏后续数据流, bat_soc 越界仍被检测)
        TEST_ASSERT(s2.self_test.status       == 2, "reset + 推 bat_soc=150 → status = WARN (2) (reset 不破坏数据流)");
        TEST_ASSERT(s2.self_test.out_of_range >= 1, "越界计数 ≥ 1 (bat_soc=150 > max=100)");

        src.stop();
    }

    // ─── Test 5: 全链透传 OK 状态: ShmDataSource → QtDataBinder → DashboardBackend Q_PROPERTY ───
    printf("\n[5] 全链透传 OK 状态: ShmDataSource → QtDataBinder → DashboardBackend Q_PROPERTY:\n");
    {
        DashboardBackend backend;
        auto shm = std::make_unique<ShmDataSource>();
        auto* raw_shm = shm.get();
        auto binder = std::make_unique<QtDataBinder>();
        backend.setDataBinder(std::move(binder));
        backend.setDataSource(std::move(shm));

        // 喂全部正常值
        writeShmFrameAllOK(6);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raw_shm->tickForTest();

        // 验证 DashboardBackend Q_PROPERTY getter (走 QtDataBinder 透传)
        TEST_ASSERT(backend.selfTestStatus()            == 1, "Q_PROPERTY selfTestStatus = OK (1)");
        TEST_ASSERT(backend.selfTestCriticalReceived()  >= 5, "Q_PROPERTY selfTestCriticalReceived ≥ 5");
        TEST_ASSERT(backend.selfTestCriticalTotal()     >= 5, "Q_PROPERTY selfTestCriticalTotal ≥ 5");
        TEST_ASSERT(backend.selfTestCriticalStuck()     == 0, "Q_PROPERTY selfTestCriticalStuck = 0");
        TEST_ASSERT(backend.selfTestWarnStuck()         == 0, "Q_PROPERTY selfTestWarnStuck = 0");
        TEST_ASSERT(backend.selfTestOutOfRange()        == 0, "Q_PROPERTY selfTestOutOfRange = 0");
    }

    // ─── Test 6: 全链透传 WARN 状态: 推越界值 → Q_PROPERTY 反映 WARN ───
    printf("\n[6] 全链透传 WARN 状态: 推越界值 → Q_PROPERTY 反映:\n");
    {
        DashboardBackend backend;
        auto shm = std::make_unique<ShmDataSource>();
        auto* raw_shm = shm.get();
        auto binder = std::make_unique<QtDataBinder>();
        backend.setDataBinder(std::move(binder));
        backend.setDataSource(std::move(shm));

        // 先 OK 状态
        writeShmFrameAllOK(7);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raw_shm->tickForTest();
        TEST_ASSERT(backend.selfTestStatus() == 1, "Q_PROPERTY selfTestStatus = OK (1) (WARN 前)");

        // reset + 推越界值
        backend.resetSelfTest();
        raw_shm->pushSignalValueForTest("bat_soc", 150.0f, candash::now_monotonic_ms());
        raw_shm->tickSelfTestForTest(candash::now_monotonic_ms());
        writeShmFrameBatSocOutOfRange(8);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        raw_shm->tickForTest();

        TEST_ASSERT(backend.selfTestStatus()     == 2, "Q_PROPERTY selfTestStatus = WARN (2) (越界后)");
        TEST_ASSERT(backend.selfTestOutOfRange() >= 1, "Q_PROPERTY selfTestOutOfRange ≥ 1");
    }

    // ─── 清理 ───
    shm_display_close();
    unlink(SHM_DISPLAY_PATH);

    printf("\n=== %d/%d 测试通过 ===\n", g_test_passed, g_test_count);
    return (g_test_passed == g_test_count) ? 0 : 1;
}
