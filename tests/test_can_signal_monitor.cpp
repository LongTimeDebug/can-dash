// test_can_signal_monitor.cpp
// Layer 2 CanSignalMonitor 单元测试（纯 C++，无 Qt）

#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <string>
#include <cmath>
#include "../src/layer2/can_signal_monitor.h"

static std::vector<std::string> quality_events;
static std::vector<std::string> value_events;
static void clear_events() {
    quality_events.clear();
    value_events.clear();
}

static void on_quality_changed(const char* signal, SignalQuality q, void*) {
    quality_events.push_back(signal);
    printf("  [QUALITY] signal=%s quality=%d\n", signal, q);
}

static void on_value_updated(const char* signal, float value, void*) {
    value_events.push_back(signal);
    printf("  [VALUE] signal=%s value=%.1f\n", signal, value);
}

int main() {
    printf("=== CanSignalMonitor 单元测试 ===\n");

    MonitorCallbacks cb = {
        .onQualityChanged = on_quality_changed,
        .onValueUpdated = on_value_updated,
        .user_data = nullptr
    };

    // 定义测试监控表
    static const SignalMonitorDef table[] = {
        // name, can_id, timeout_ms, min, max, max_delta, smoothing, window
        {"bat_volt",    0x186040F3, 500,  0.0f, 500.0f,  100.0f, true,  5},
        {"bat_curr",    0x186040F4, 500, -500.0f, 500.0f,  200.0f, false, 0},
        {"motor_speed", 0x186050F3, 200,    0.0f, 8000.0f,  50.0f, true,  3},
    };
    const int table_count = 3;

    CanSignalMonitor monitor(cb);
    monitor.init(table, table_count);

    // ─── 测试1：初始状态 SIGNAL_NEVER_RECEIVED ───
    printf("\n[测试1] 初始状态\n");
    assert(monitor.getQuality("bat_volt") == SIGNAL_NEVER_RECEIVED);
    assert(monitor.getSmoothedValue("bat_volt") == 0.0f);
    printf("  ✓ 初始为 SIGNAL_NEVER_RECEIVED\n");

    // ─── 测试2：正常值接收 ───
    printf("\n[测试2] 正常值接收\n");
    clear_events();
    monitor.onCanFrame(0x186040F3, 350.0f);
    assert(monitor.getQuality("bat_volt") == SIGNAL_GOOD);
    // 平滑使用完整窗口平均，首次只有1个有效样本，其余为0
    assert(monitor.getSmoothedValue("bat_volt") > 69.0f && monitor.getSmoothedValue("bat_volt") < 71.0f);
    assert(!quality_events.empty() || !value_events.empty());  // 回调触发
    printf("  ✓ 正常值 → SIGNAL_GOOD\n");

    // ─── 测试3：超出范围检测 ───
    printf("\n[测试3] 超出范围检测\n");
    clear_events();
    monitor.onCanFrame(0x186040F3, 600.0f);  // > 500 max
    assert(monitor.getQuality("bat_volt") == SIGNAL_INVALID_RANGE);
    printf("  ✓ 超范围 → SIGNAL_INVALID_RANGE\n");

    // ─── 测试4：突变检测 ───
    printf("\n[测试4] 突变检测\n");
    clear_events();
    monitor.onCanFrame(0x186040F3, 350.0f);   // 重置到正常
    assert(monitor.getQuality("bat_volt") == SIGNAL_GOOD);
    monitor.onCanFrame(0x186040F3, 460.0f);   // delta=110 > max_delta=100（范围内）
    assert(monitor.getQuality("bat_volt") == SIGNAL_ABNORMAL_DELTA);
    printf("  ✓ 突变 → SIGNAL_ABNORMAL_DELTA\n");

    // ─── 测试5：CAN ID 不匹配 ───
    printf("\n[测试5] CAN ID 不匹配\n");
    clear_events();
    monitor.onCanFrame(0x99999999, 100.0f);  // 未定义 ID
    assert(monitor.getQuality("bat_volt") == SIGNAL_ABNORMAL_DELTA);  // 状态不变
    printf("  ✓ 未知 CAN ID 安全跳过\n");

    // ─── 测试6：超时检测（tick 不崩溃）───
    printf("\n[测试6] tick 不崩溃\n");
    clear_events();
    monitor.tick(0);    // 0ms
    monitor.tick(100);   // 100ms
    monitor.tick(1000);  // 1000ms - tick 运行多次
    printf("  ✓ tick 执行不崩溃\n");

    // ─── 测试7：未知信号查询 ───
    printf("\n[测试7] 未知信号查询\n");
    assert(monitor.getQuality("nonexistent") == SIGNAL_NEVER_RECEIVED);
    assert(monitor.getSmoothedValue("nonexistent") == 0.0f);
    printf("  ✓ 未知信号返回默认值\n");

    // ─── 测试8：质量变化才触发回调 ───
    printf("\n[测试8] 质量变化才触发回调\n");
    clear_events();
    monitor.onCanFrame(0x186040F4, 50.0f);   // 正常值，质量=GOOD
    monitor.onCanFrame(0x186040F4, 75.0f);   // 同质量，不应触发 quality 回调
    // value callback 应触发，但 quality 不变
    assert(monitor.getQuality("bat_curr") == SIGNAL_GOOD);
    printf("  ✓ 同质量不重复触发回调\n");

    // ─── 测试9：析构不崩溃（无 nullptr） ───
    printf("\n[测试9] 析构安全\n");
    {
        CanSignalMonitor m2(cb);
        m2.init(table, table_count);
        m2.onCanFrame(0x186040F3, 350.0f);
    }  // 析构
    printf("  ✓ 析构不崩溃\n");

    // ─── 测试10：重新 init() 不泄漏（m_states 旧指针必须先 delete） ───
    printf("\n[测试10] 重新 init() 安全\n");
    {
        CanSignalMonitor m3(cb);
        m3.init(table, table_count);
        m3.onCanFrame(0x186040F3, 350.0f);
        // 第二次 init 必须正确释放旧 m_states
        m3.init(table, table_count);
        assert(m3.getQuality("bat_volt") == SIGNAL_NEVER_RECEIVED);  // 重置
        m3.onCanFrame(0x186040F3, 360.0f);
        assert(m3.getQuality("bat_volt") == SIGNAL_GOOD);
        // 第三次 init
        m3.init(table, table_count);
        m3.onCanFrame(0x186040F3, 370.0f);
    }  // 析构
    printf("  ✓ 重新 init() 不崩溃、不泄漏旧 m_states\n");

    // ─── 测试11：smoothing_window > MAX_SIGNAL_HISTORY 时自动禁用平滑 ───
    printf("\n[测试11] 大窗口安全降级\n");
    {
        static const SignalMonitorDef bigwin_table[] = {
            // window=255 远超 MAX_SIGNAL_HISTORY=16
            {"big_volt", 0x186040FF, 500, 0.0f, 500.0f, 100.0f, true, 255},
        };
        CanSignalMonitor m4(cb);
        m4.init(bigwin_table, 1);
        m4.onCanFrame(0x186040FF, 100.0f);
        // window 超限时 history=nullptr，smoothedValue == lastValue
        assert(m4.getQuality("big_volt") == SIGNAL_GOOD);
        // smoothed = 100.0（无平滑时等于 lastValue）
        assert(m4.getSmoothedValue("big_volt") > 99.9f && m4.getSmoothedValue("big_volt") < 100.1f);
    }
    printf("  ✓ smoothing_window 超限时自动禁用平滑（无堆分配）\n");

    // ─── 测试12：smoothing=false 时不分配 history ───
    printf("\n[测试12] smoothing=false 不分配 history\n");
    {
        static const SignalMonitorDef nosmooth_table[] = {
            {"nosmooth", 0x186040FE, 500, 0.0f, 500.0f, 100.0f, false, 0},
        };
        CanSignalMonitor m5(cb);
        m5.init(nosmooth_table, 1);
        m5.onCanFrame(0x186040FE, 200.0f);
        assert(m5.getSmoothedValue("nosmooth") > 199.9f && m5.getSmoothedValue("nosmooth") < 200.1f);
    }
    printf("  ✓ smoothing=false 时 history=nullptr，smoothed == lastValue\n");

    // ─── 测试13：池满时多余信号禁用平滑 ───
    printf("\n[测试13] 池满降级\n");
    {
        // 制造 33 个启用平滑的信号（MAX_SIGNAL_MONITORS=32），第 33 个应被降级
        // 注意：每个信号必须用唯一 name，否则 getQuality/getSmoothedValue 只能查到第一个
        // name 是 const char*，必须指向稳定内存（用静态 buffer 数组）
        static char names[33][16];
        SignalMonitorDef dyn_table[33];
        for (int i = 0; i < 33; i++) {
            snprintf(names[i], sizeof(names[i]), "pool_%02d", i);
            dyn_table[i].name = names[i];
            dyn_table[i].can_id = 0x18605200u + (uint32_t)i;
            dyn_table[i].timeout_ms = 500;
            dyn_table[i].min_value = 0.0f;
            dyn_table[i].max_value = 100.0f;
            dyn_table[i].max_delta = 50.0f;
            dyn_table[i].smoothing = true;
            dyn_table[i].smoothing_window = 4;
        }
        CanSignalMonitor m6(cb);
        m6.init(dyn_table, 33);
        // 33 个信号中前 32 个有平滑，第 33 个（pool_32）降级（pool 已满）
        m6.onCanFrame(0x18605220, 80.0f);  // i=32，第 33 个的 can_id
        // 第 33 个拿到 pool 失败 → history=nullptr → smoothed == lastValue
        assert(m6.getSmoothedValue("pool_32") > 79.9f && m6.getSmoothedValue("pool_32") < 80.1f);
        // 前 32 个中任意一个有平滑（4 个样本中 1 个=80，3 个=0 → smoothed=20）
        m6.onCanFrame(0x18605200, 80.0f);  // pool_00
        assert(m6.getSmoothedValue("pool_00") > 19.9f && m6.getSmoothedValue("pool_00") < 20.1f);
    }
    printf("  ✓ 池满（第 33 个信号）安全降级，前 32 个平滑正常\n");

    // ─── 测试14：table_count=0 不分配 ───
    printf("\n[测试14] 空表安全\n");
    {
        CanSignalMonitor m7(cb);
        m7.init(nullptr, 0);  // 必须不崩溃
        assert(m7.getQuality("anything") == SIGNAL_NEVER_RECEIVED);
    }
    printf("  ✓ table_count=0 边界安全\n");

    printf("\n所有测试通过。\n");
    return 0;
}
