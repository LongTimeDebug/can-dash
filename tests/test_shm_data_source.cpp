// test_shm_data_source.cpp
// ShmDataSource 单元测试（手写测试，遵循项目惯例）
//
// 覆盖：
// 1. 启动 + 初始状态
// 2. 健康状态变化（DISCONNECTED → OK）
// 3. 帧推送（业务字段转换）
// 4. 离线时不推送数据
// 5. 安全带警告条件（occupied && !buckled && speed>5）
// 6. 丢帧检测（frame_seq 跳号）
// 7. 指示灯映射

#include "layer3/shm_data_source.h"
#include "layer3/display_data_types.h"
#include "layer1/shm/shm_display.h"
#include "layer2/time_util.h"
#include "mock_data_binder.h"

#include <QCoreApplication>
#include <cstdio>
#include <cstring>
#include <cstdlib>
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

void writeShmFrame(uint32_t frame_seq, float speed, uint8_t driver_occ, uint8_t driver_buck) {
    shm_display_close();
    unlink(SHM_DISPLAY_PATH);  // 删旧文件（避免上次测试残留的 valid magic+checksum）
    shm_display_create();  // 测试前先建 shm
    DisplayDataShm shm = {};
    shm.magic = SHM_MAGIC;
    shm.version = SHM_VERSION;
    shm.last_commit_ms = candash::now_monotonic_ms();
    shm.updated_mask = 0xFFFFFFFF;
    shm.frame_seq = frame_seq;
    shm.vehicle_speed = speed;
    shm.motor_rpm = 3000;
    shm.bat_volt = 350.0f;
    shm.bat_soc = 75;
    shm.motor_temp = 60;
    shm.driver_occupied = driver_occ;
    shm.driver_buckled = driver_buck;
    shm.passenger_occupied = 0;
    shm.passenger_buckled = 0;
    shm.rear_buckle = 0;
    shm.alarm_active = 0;
    shm.indicators[0].on = 1;
    shm.indicators[0].flash = 1;
    shm.indicators[0].hz_x10 = 15;
    shm.checksum = shm_display_compute_checksum(&shm);  // 算 checksum 让读端校验通过
    shm_display_write(&shm);
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    printf("\n=== ShmDataSource 测试 ===\n");

    // 全局清理：每次测试前确保 shm 干净
    shm_display_close();

    // ─── Test 1: 启动 + 初始状态 ───
    printf("\n[1] 启动/停止:\n");
    {
        ShmDataSource src;
        TEST_ASSERT(!src.isRunning(), "初始 isRunning = false");
        src.start();
        TEST_ASSERT(src.isRunning(), "start() 后 isRunning = true");
        src.stop();
        TEST_ASSERT(!src.isRunning(), "stop() 后 isRunning = false");
    }

    // ─── Test 2: 健康状态变化 ───
    printf("\n[2] 健康状态变化:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        src.tickForTest();
        TEST_ASSERT(binder.lastHealth() == HEALTH_DISCONNECTED, "shm 缺失 → DISCONNECTED");

        writeShmFrame(1, 30.0f, 1, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        TEST_ASSERT(binder.lastHealth() == HEALTH_OK, "写入帧后 → OK");

        src.stop();
        shm_display_close();
    }

    // ─── Test 3: 帧推送字段映射 ───
    printf("\n[3] 业务字段映射:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        writeShmFrame(1, 60.0f, 1, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();

        TEST_ASSERT(binder.snapshotCount() >= 1, "至少 1 个 snapshot");
        if (binder.snapshotCount() >= 1) {
            auto s = binder.lastSnapshot();
            TEST_ASSERT(qFuzzyCompare(s.data.vehicle_speed, 60.0f), "vehicle_speed = 60");
            TEST_ASSERT(s.data.motor_rpm == 3000, "motor_rpm = 3000");
            TEST_ASSERT(qFuzzyCompare(s.data.bat_volt, 350.0f), "bat_volt = 350");
            TEST_ASSERT(s.data.bat_soc == 75, "bat_soc = 75");
            TEST_ASSERT(s.data.driver_occupied == 1, "driver_occupied = 1");
            TEST_ASSERT(s.meta.frame_seq == 1u, "frame_seq = 1");
            TEST_ASSERT(s.is_moving, "is_moving = true (60km/h)");
        }
        src.stop();
        shm_display_close();
    }

    // ─── Test 4: 离线时不推送数据 ───
    printf("\n[4] 离线时无数据:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        src.tickForTest();  // 离线
        size_t c1 = binder.snapshotCount();
        src.tickForTest();
        TEST_ASSERT(binder.snapshotCount() == c1, "连续离线不增加 snapshot 计数");
        src.stop();
    }

    // ─── Test 5: 安全带警告条件 ───
    printf("\n[5] 安全带警告条件:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        // 60 km/h, driver 坐了但没系 → 警告
        writeShmFrame(1, 60.0f, 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s = binder.lastSnapshot();
        TEST_ASSERT(s.seat_belt.warning_active, "60km/h + driver occupied + not buckled → warning");
        TEST_ASSERT(s.seat_belt.seats[0].warning, "driver seat warning");

        // 0 km/h, driver 坐了但没系 → 不警告（低速不报警）
        writeShmFrame(2, 0.0f, 1, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        s = binder.lastSnapshot();
        TEST_ASSERT(!s.seat_belt.warning_active, "0km/h + not buckled → no warning");
        TEST_ASSERT(!s.seat_belt.seats[0].warning, "driver seat no warning");

        src.stop();
        shm_display_close();
    }

    // ─── Test 6: 丢帧检测 ───
    printf("\n[6] 丢帧检测:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        writeShmFrame(1, 30.0f, 1, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        TEST_ASSERT(binder.lastSnapshot().meta.frame_seq == 1u, "frame_seq=1");
        TEST_ASSERT(binder.lastSnapshot().meta.dropped_frames == 0u, "丢帧数=0");

        // 跳号到 5（丢 2/3/4 共 3 帧）
        writeShmFrame(5, 30.0f, 1, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        TEST_ASSERT(binder.lastSnapshot().meta.frame_seq == 5u, "frame_seq=5");
        TEST_ASSERT(binder.lastSnapshot().meta.dropped_frames == 3u, "丢帧数=3");

        src.stop();
        shm_display_close();
    }

    // ─── Test 7: 指示灯映射 ───
    printf("\n[7] 指示灯映射:\n");
    {
        shm_display_close();
        ShmDataSource src;
        MockDataBinder binder;
        src.setUpdateCallback([&](const DisplaySnapshot& s) { binder.onDataUpdated(s); });
        src.setHealthCallback([&](HealthStatus h) { binder.onHealthChanged(h); });
        src.start();

        writeShmFrame(1, 0.0f, 0, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        src.tickForTest();
        auto s = binder.lastSnapshot();
        TEST_ASSERT(s.indicators.lights[0].on, "indicator[0].on = true");
        TEST_ASSERT(s.indicators.lights[0].flash, "indicator[0].flash = true");
        TEST_ASSERT(qFuzzyCompare(s.indicators.lights[0].hz, 1.5f), "indicator[0].hz = 1.5");

        src.stop();
        shm_display_close();
    }

    printf("\n=== 总计: %d/%d 通过 ===\n", g_test_passed, g_test_count);
    return (g_test_passed == g_test_count) ? 0 : 1;
}
