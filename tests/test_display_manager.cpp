// test_display_manager.cpp
// Layer 2 DisplayManager 单元测试 (PR 51: 覆盖 LCD 背光超时管理状态机)
// 跟 PR 21/22/23/47 同 TEST_ASSERT 骨架.
//
// DisplayManager::tick() 行为契约 (读完 src/layer2/display_manager.cpp 提炼):
//   1. 第一个 tick (m_lastTickMs == 0) 是"init tick": 只设 m_lastTickMs, 不动状态
//   2. m_wasMoving 构造时初始化为 true
//   3. 第一个非 init tick, 若 isMoving=false, m_wasMoving=true:
//      → 走 transition 分支: m_stationaryMs=0, m_wasMoving=false
//      → 该 tick 的 elapsed 不累计到 stationaryMs (但累加到 idleMs)
//   4. 后续 tick 走 stationary 分支: m_stationaryMs += elapsed
//   5. elapsed 在 > 10000ms 时被截断到 10000ms (防时间跳变)
//   6. idleMs 在每个非 init tick 累加 capped elapsed
//   7. isMoving = (m_lastSpeed > 1.0f) — 严格 > 1.0
//   8. onUserInteraction 强制 idleMs=0 且 state=NORMAL
//
// 因此"静止 N 秒"的标准测试模式是 3 个 tick:
//   tick(T0)         init (lastTickMs=T0)
//   tick(T0+dt1)     transition (wasMoving=true→false, stationaryMs=0, idleMs+=dt1)
//   tick(T0+dt1+dt2) 累积 (stationaryMs += min(dt2, 10000), idleMs += min(dt2, 10000))
// dt1 和 dt2 都 ≤ 10000 避免 cap.

#include <cstdio>
#include <cstring>
#include <cassert>
#include "../src/layer2/display_manager.h"

static int g_test_count = 0;
static int g_test_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_test_count++; \
    if (cond) { g_test_passed++; printf("  ✓ %s\n", msg); } \
    else { printf("  ✗ %s (line %d)\n", msg, __LINE__); } \
} while(0)

// 辅助: 走完"静止"的标准 3-tick 序列, 返回最终 state + 累计 idleMs
//   tick(T) → init
//   tick(T+100) → transition (wasMoving=true→false)
//   tick(T+100+elapsed) → 累积 elapsed (≤10000)
struct TickResult {
    BacklightState state;
    uint64_t idle_ms;
};

static TickResult runStationary(DisplayManager& dm, uint64_t base_ms,
                                uint64_t elapsed_ms) {
    dm.tick(base_ms);            // init
    dm.tick(base_ms + 100);      // transition
    dm.tick(base_ms + 100 + elapsed_ms);  // accumulate
    return {dm.getBacklightState(), dm.idleTimeMs()};
}

int main() {
    printf("=== DisplayManager 单元测试 ===\n\n");

    // ─── 测试 1: init 初始状态 ───
    printf("[测试 1] init 初始状态 (timeout=30s)\n");
    {
        DisplayManager dm(30);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "初始状态 = NORMAL");
        TEST_ASSERT(dm.idleTimeMs() == 0, "初始 idleTimeMs = 0");
    }

    // ─── 测试 2: 默认 timeout 30 秒 (无参构造) ───
    printf("\n[测试 2] 默认 timeout 30s, 静止 30s+ → DIM\n");
    {
        DisplayManager dm;  // 默认 30
        dm.setVehicleSpeed(0.0f);
        // 用 5 个 6s 累积 = 30s 静止
        dm.tick(0);            // init
        dm.tick(100);          // transition
        dm.tick(100 + 6000);   // 6s
        dm.tick(100 + 12000);  // 12s
        dm.tick(100 + 18000);  // 18s
        dm.tick(100 + 24000);  // 24s
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "静止 24s (timeout=30) → NORMAL");
        dm.tick(100 + 30000);  // 30s
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "静止 30s (timeout=30) → DIM");
    }

    // ─── 测试 3: 速度 > 1.0 km/h 视为行驶, 状态恒 NORMAL ───
    printf("\n[测试 3] 速度 > 1.0 km/h → 一直 NORMAL\n");
    {
        DisplayManager dm(10);  // 短 timeout, 加速测试
        dm.setVehicleSpeed(30.0f);
        // 13 个 5s tick = 65s, 第 1 个是 init, 后 12 个累加 idleMs
        // 远大于 10s timeout
        uint64_t t = 0;
        for (int i = 0; i < 13; i++) {
            t += 5000;
            dm.tick(t);
        }
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "行驶 65s (timeout=10) → NORMAL");
        // idleMs: 第 1 个 tick init 跳过, 后 12 个累加 5000 = 60000
        TEST_ASSERT(dm.idleTimeMs() == 60000,
                    "idleMs 累计 = 60000 (行驶中也累计)");
    }

    // ─── 测试 4: 速度 0 持续 timeout → DIM ───
    printf("\n[测试 4] 速度 0 持续 timeout → DIM\n");
    {
        DisplayManager dm(5);  // 5s timeout
        dm.setVehicleSpeed(0.0f);
        TickResult r = runStationary(dm, 0, 4000);
        TEST_ASSERT(r.state == BACKLIGHT_NORMAL,
                    "静止 4s (timeout=5) → NORMAL");
        TickResult r2 = runStationary(dm, 100000, 5000);
        TEST_ASSERT(r2.state == BACKLIGHT_DIM,
                    "静止 5s (timeout=5) → DIM");
        // 继续停在 10s: 状态不应回弹 (DIM → DIM)
        dm.tick(100000 + 100 + 10000);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "静止 10s → 仍然 DIM");
    }

    // ─── 测试 5: DIM 后车速回升 → NORMAL ───
    printf("\n[测试 5] DIM 后车速回升 → NORMAL\n");
    {
        DisplayManager dm(5);
        dm.setVehicleSpeed(0.0f);
        // 触发 DIM
        dm.tick(0); dm.tick(100); dm.tick(5100);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "前置: 静止 5s → DIM");
        // 车辆开动
        dm.setVehicleSpeed(30.0f);
        dm.tick(5200);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "切到 moving → NORMAL");
        // 再次停车, stationaryMs 必须从 0 重新累计 (m_wasMoving 切换)
        dm.setVehicleSpeed(0.0f);
        dm.tick(5300);  // transition (wasMoving=true→false), stationaryMs=0
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "再次停车 0s (transition) → NORMAL");
        dm.tick(5300 + 3000);  // 3s 后
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "再次停车 3s (timeout=5) → NORMAL (未到 5s)");
    }

    // ─── 测试 6: onUserInteraction 强制回 NORMAL + 清 idle ───
    printf("\n[测试 6] onUserInteraction 强制 NORMAL\n");
    {
        DisplayManager dm(5);
        dm.setVehicleSpeed(0.0f);
        dm.tick(0);        // init
        dm.tick(2000);     // transition + idleMs += 2000 = 2000
        TEST_ASSERT(dm.idleTimeMs() == 2000, "idle 累计 2s (一次 tick 2000ms)");
        dm.onUserInteraction();
        TEST_ASSERT(dm.idleTimeMs() == 0, "onUserInteraction 后 idle=0");
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "onUserInteraction 后状态=NORMAL");
        // 后续 tick idleMs 重新从 0 累加
        dm.tick(3000);
        TEST_ASSERT(dm.idleTimeMs() == 1000, "onUserInteraction 后 tick idle=1000");
    }

    // ─── 测试 7: 时间跳变 (>10s) 截断到 10s, 防止 fake 静止 N 小时 ───
    printf("\n[测试 7] 时间跳变 (>10s) 截断到 10s\n");
    {
        DisplayManager dm(5);
        dm.setVehicleSpeed(0.0f);
        // 先完成 transition (避免 transition 把 elapsed 吃掉)
        dm.tick(0);            // init (lastTickMs=0)
        dm.tick(100);          // transition: wasMoving=true→false, idleMs += 100 = 100
        // 跳变 100s (elapsed > 10000 截断到 10000)
        dm.tick(100 + 100000);
        // 10000ms 已超过 timeout 5000ms → DIM
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "100s 跳变后被截断为 10s, 触发 DIM (5s timeout)");
        // idleMs = 100 (transition) + 10000 (capped 跳变) = 10100
        TEST_ASSERT(dm.idleTimeMs() == 10100,
                    "100s 跳变 idleMs 截断, 总计 transition(100) + capped(10000) = 10100");
    }

    // ─── 测试 8: 速度阈值 1.0 km/h 严格 > (边界) ───
    printf("\n[测试 8] 速度阈值 1.0 km/h 严格 >\n");
    {
        // 0.9 km/h: 不动 (0.9 > 1.0 = false)
        DisplayManager dm(5);
        dm.setVehicleSpeed(0.9f);
        dm.tick(0); dm.tick(100);  // init + transition
        dm.tick(100 + 5000);       // accumulate 5s
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "speed=0.9 (≤1.0) → 视为停车 → DIM");

        // 1.0 km/h: 严格 > 1.0, 1.0 > 1.0 = false → 停车
        DisplayManager dm2(5);
        dm2.setVehicleSpeed(1.0f);
        dm2.tick(0); dm2.tick(100); dm2.tick(100 + 5000);
        TEST_ASSERT(dm2.getBacklightState() == BACKLIGHT_DIM,
                    "speed=1.0 (==1.0, 严格 > 失败) → 视为停车 → DIM");

        // 1.1 km/h: 动
        DisplayManager dm3(5);
        dm3.setVehicleSpeed(1.1f);
        dm3.tick(0); dm3.tick(100);  // init + 1 tick 时 isMoving=true
        // 后续 tick 也保持 moving
        for (int i = 0; i < 12; i++) dm3.tick(100 + (i+1) * 5000);
        TEST_ASSERT(dm3.getBacklightState() == BACKLIGHT_NORMAL,
                    "speed=1.1 (>1.0) → 行驶 60s → NORMAL");
    }

    // ─── 测试 9: setVehicleSpeed 独立调用, 不触发 tick 状态变更 ───
    printf("\n[测试 9] setVehicleSpeed 独立调用, 不依赖 tick\n");
    {
        DisplayManager dm(5);
        // 不调 tick, 只调 setVehicleSpeed
        dm.setVehicleSpeed(0.0f);
        dm.setVehicleSpeed(30.0f);
        dm.setVehicleSpeed(0.0f);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "无 tick 调用 → 状态维持 NORMAL");
        TEST_ASSERT(dm.idleTimeMs() == 0, "无 tick → idle=0");
    }

    // ─── 测试 10: 第一个 tick 初始化 lastTickMs, 不动状态 ───
    printf("\n[测试 10] 第一个 tick 初始化 lastTickMs\n");
    {
        DisplayManager dm(5);
        dm.setVehicleSpeed(0.0f);
        // 即使 now_ms 巨大, 第一个 tick 也不计入 elapsed (init branch)
        dm.tick(999999999ULL);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "第一个 tick 不动状态 (init lastTickMs 即可)");
        TEST_ASSERT(dm.idleTimeMs() == 0, "第一个 tick idleMs=0 (不累加)");
        // 第二次 tick: transition (wasMoving=true→false)
        dm.tick(999999999ULL + 100);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_NORMAL,
                    "第二次 tick transition → 仍 NORMAL (stationaryMs=0)");
        // 第三次 tick: 累积 5s
        dm.tick(999999999ULL + 100 + 5000);
        TEST_ASSERT(dm.getBacklightState() == BACKLIGHT_DIM,
                    "第三次 tick + 5s (timeout=5) → DIM");
    }

    // ─── 测试 11: 多个 timeout_seconds 值 ───
    printf("\n[测试 11] 不同 timeout_seconds\n");
    {
        // timeout=1: 静止 1s 即 DIM
        DisplayManager dm1(1);
        dm1.setVehicleSpeed(0.0f);
        dm1.tick(0); dm1.tick(100); dm1.tick(100 + 1000);
        TEST_ASSERT(dm1.getBacklightState() == BACKLIGHT_DIM,
                    "timeout=1, 静止 1s → DIM");

        // timeout=255 (max uint8_t)
        DisplayManager dm255(255);
        dm255.setVehicleSpeed(0.0f);
        // 累 254s
        dm255.tick(0); dm255.tick(100);  // init + transition
        for (uint64_t i = 0; i < 25; i++) dm255.tick(100 + (i+1) * 10000);  // 25*10s = 250s
        TEST_ASSERT(dm255.getBacklightState() == BACKLIGHT_NORMAL,
                    "timeout=255, 静止 250s → 仍 NORMAL");
    }

    // ─── 汇总 ───
    printf("\n=== 测试结果: %d/%d 通过 ===\n", g_test_passed, g_test_count);
    if (g_test_passed != g_test_count) {
        printf("FAILED: %d assertions failed\n", g_test_count - g_test_passed);
        return 1;
    }
    return 0;
}
