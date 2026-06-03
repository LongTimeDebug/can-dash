// test_trip_computer.cpp
// Layer 2 TripComputer 单元测试（纯 C++，无 Qt）

#include <cstdio>
#include <cassert>
#include "../src/layer2/trip_computer.h"

static void test_initial_state() {
    printf("\n[测试1] 初始状态\n");
    TripComputer t;
    assert(t.tripDistanceKm() == 0.0f);
    assert(t.tripAvgSpeedKmh() == 0.0f);
    assert(t.tripDurationS() == 0);
    assert(!t.isMoving());
    printf("  ✓ 全部为 0\n");
}

static void test_first_tick_only_records_baseline() {
    printf("\n[测试2] 首次 tick 仅记录基准, 不算距离\n");
    TripComputer t;
    t.tick(1000, 50.0f);  // 首次
    // 距离/时长 都不应被计算 (无 dt)
    assert(t.tripDistanceKm() == 0.0f);
    assert(t.tripDurationS() == 0);
    assert(t.isMoving());  // 但 isMoving 应当正确反映
    printf("  ✓ 首次 tick 不算距离, isMoving 正确\n");
}

static void test_constant_speed_60kmh_for_1h() {
    printf("\n[测试3] 60 km/h 匀速 (16ms tick × 5625 次 = 90s = 1.5 km)\n");
    TripComputer t;
    t.tick(0, 60.0f);  // 首次 (t=0, baseline)
    // 模拟 60fps × 90s = 5400 次 tick, 但只跑 5625 次确认 (因为 60kmh * 1.5h = 90km 错!)
    // 重算: 60 km/h × 90s / 3600 = 1.5 km
    const int N = 5625;  // 5625 * 16ms = 90s
    for (int i = 1; i <= N; i++) {
        t.tick(static_cast<uint64_t>(i) * 16, 60.0f);
    }
    // distance = 60 km/h × 90s / 3600 = 1.5 km
    assert(t.tripDistanceKm() > 1.49f && t.tripDistanceKm() < 1.51f);
    assert(t.tripDurationS() == 90);
    assert(t.tripAvgSpeedKmh() > 59.9f && t.tripAvgSpeedKmh() < 60.1f);
    printf("  ✓ distance=%.3fkm, duration=%us, avg=%.2fkmh\n",
           t.tripDistanceKm(), t.tripDurationS(), t.tripAvgSpeedKmh());
}

static void test_stop_doesnt_accumulate() {
    printf("\n[测试4] 停车时不累计距离/时长\n");
    TripComputer t;
    t.tick(0, 60.0f);
    t.tick(1000, 0.0f);   // 立即停车
    t.tick(60000, 0.0f);  // 1 分钟停车
    assert(t.tripDistanceKm() == 0.0f);
    assert(t.tripDurationS() == 0);
    assert(!t.isMoving());
    printf("  ✓ 停车 60s 后 distance=0, duration=0\n");
}

static void test_below_threshold_creep_doesnt_count() {
    printf("\n[测试5] 怠速 0.5 km/h (低于 1.0 阈值) 不算行驶\n");
    TripComputer t;
    t.tick(0, 0.5f);
    t.tick(60000, 0.5f);  // 1 分钟 0.5 km/h
    assert(t.tripDistanceKm() == 0.0f);
    assert(t.tripDurationS() == 0);
    assert(!t.isMoving());
    printf("  ✓ 怠速 1min distance=0\n");
}

static void test_trapezoid_integration() {
    printf("\n[测试6] 梯形积分: 0→100 km/h 线性加速 60s, 16ms tick\n");
    TripComputer t;
    t.tick(0, 0.0f);
    // 60s = 3750 ticks × 16ms, 速度从 0 线性增到 100 km/h
    const int N = 3750;
    const float speed_step = 100.0f / static_cast<float>(N);
    for (int i = 1; i <= N; i++) {
        const uint64_t t_ms = static_cast<uint64_t>(i) * 16;
        const float v = static_cast<float>(i) * speed_step;
        t.tick(t_ms, v);
    }
    // 理论: 0→100 线性 60s, 面积 = 50/3600 * 100 = 0.833 km
    // (50 是 (0+100)/2 平均速度 km/h, 100/3600 是 60s 换 h)
    // 实际 = 0.5 * 100 * 60 / 3600 = 0.833 km
    assert(t.tripDistanceKm() > 0.82f && t.tripDistanceKm() < 0.85f);
    printf("  ✓ 0→100 线性 60s: distance=%.3fkm (期望 ~0.833km)\n",
           t.tripDistanceKm());
}

static void test_reset() {
    printf("\n[测试7] reset() 清零\n");
    TripComputer t;
    t.tick(0, 60.0f);
    t.tick(60000, 60.0f);  // 1 min at 60 km/h = 1 km
    assert(t.tripDistanceKm() > 0.9f);
    t.reset();
    assert(t.tripDistanceKm() == 0.0f);
    assert(t.tripDurationS() == 0);
    assert(t.tripAvgSpeedKmh() == 0.0f);
    assert(!t.isMoving());
    printf("  ✓ reset 后全部清零\n");
}

static void test_time_jump_clamped() {
    printf("\n[测试8] 时间跳变 (>10s) 被 cap, 防止距离暴增\n");
    TripComputer t;
    t.tick(0, 60.0f);  // baseline
    t.tick(16, 60.0f); // 16ms 后, baseline 后第一次正常 tick
    // 模拟挂起 1 小时后唤醒
    t.tick(3600 * 1000, 60.0f);
    // dt 应被 cap 到 10s, distance 应只增加 10s × 60 km/h = 0.167 km
    // 加上 16ms tick 的 16ms × 60 = 0.000267 km ≈ 1.5km 总 (实际应只有 ~0.17km)
    assert(t.tripDistanceKm() < 0.2f);
    assert(t.tripDistanceKm() > 0.15f);
    printf("  ✓ 1h 跳变被 cap 到 10s, distance=%.3fkm (期望 ~0.167km)\n",
           t.tripDistanceKm());
}

int main() {
    printf("=== TripComputer 单元测试 ===\n");
    test_initial_state();
    test_first_tick_only_records_baseline();
    test_constant_speed_60kmh_for_1h();
    test_stop_doesnt_accumulate();
    test_below_threshold_creep_doesnt_count();
    test_trapezoid_integration();
    test_reset();
    test_time_jump_clamped();
    printf("\n所有测试通过。\n");
    return 0;
}
