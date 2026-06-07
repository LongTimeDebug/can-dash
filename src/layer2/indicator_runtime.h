// indicator_runtime.h
// Layer 2: 指示灯运行时
// 纯 C++，无 Qt，无动态内存 (符合 .ai/agent_checklist.yaml code_quality 规则)

#pragma once
#include <cstdint>
#include <cstring>
#include "../generated/indicator_def.h"
struct IndicatorState {
    const IndicatorDef* def;
    bool       on;
    bool       flash;
    float      flashHz;
    uint64_t   lastChangeMs;
};

struct IndicatorCallbacks {
    void (*onStateChange)(const char* id, bool on, bool flash, float hz, void* user_data);
    void* user_data;
};

class IndicatorRuntime {
public:
    // 编译期上限: 与 .yaml indicators.yaml 注册数量上限一致 (32)。
    // 一律栈数组, 禁止 new[]/malloc (符合 Layer 2 内存规则).
    static constexpr int MAX_INDICATORS = 32;

    explicit IndicatorRuntime(const IndicatorCallbacks& cb = {});
    ~IndicatorRuntime() = default;  // 无堆内存, 不需要析构 (cppcheck: unsafeClassCanLeak 已解决)

    void init(const IndicatorDef* table, int table_count);

    // 由 AlarmRuntime 调用，更新某个 widget 的状态
    void setIndicator(const char* widget_id, bool on, bool flash, float hz);

    // 定时调用（驱动闪烁时序）
    void tick(uint64_t now_ms);

    // 查询
    int activeCount() const;
    bool isOn(const char* id) const;

    const char* name() const { return "IndicatorRuntime"; }

private:
    IndicatorState* findIndicator(const char* id);

    IndicatorCallbacks m_cb;
    const IndicatorDef* m_table = nullptr;
    // 静态数组 — 零堆分配, 析构/异常安全, 编译期固定内存占用 (~1.5KB)
    IndicatorState m_states[MAX_INDICATORS] = {};
    int m_count = 0;
    uint64_t m_lastTickMs = 0;
};
