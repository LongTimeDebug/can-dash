// indicator_runtime.cpp
#include "indicator_runtime.h"
#include "time_util.h"
#include <algorithm>  // std::min
#include <cstdio>

IndicatorRuntime::IndicatorRuntime(const IndicatorCallbacks& cb)
    : m_cb(cb) {}

void IndicatorRuntime::init(const IndicatorDef* table, int table_count) {
    // 重置所有状态 (支持 re-init, 旧状态自动覆盖)
    for (int i = 0; i < MAX_INDICATORS; i++) {
        m_states[i] = IndicatorState{};
    }
    m_table = nullptr;
    m_count = 0;

    if (table_count <= 0) return;  // 早返回, table 为空
    if (table == nullptr) return;  // 防御性: 空表指针 + 正数 count

    // 编译期上限保护: 超过 MAX_INDICATORS 的部分被截断, 防止栈溢出
    // (yaml 校验工具 validate.py 已经检查 indicators.yaml 数量, 这里双保险)
    const int clamped = std::min(table_count, MAX_INDICATORS);
    m_table = table;
    m_count = clamped;

    for (int i = 0; i < clamped; i++) {
        m_states[i].def = &table[i];
        m_states[i].on = false;
        m_states[i].flash = false;
        m_states[i].flashHz = 0.0f;
        m_states[i].lastChangeMs = 0;
    }
}

void IndicatorRuntime::setIndicator(const char* widget_id, bool on, bool flash, float hz) {
    if (widget_id == nullptr) return;  // 防御: nullptr id
    IndicatorState* state = findIndicator(widget_id);
    if (!state) return;

    state->on = on;
    state->flash = flash;
    state->flashHz = hz;
    state->lastChangeMs = candash::now_monotonic_ms();

    if (m_cb.onStateChange) {
        m_cb.onStateChange(widget_id, on, flash, hz, m_cb.user_data);
    }
}

void IndicatorRuntime::tick(uint64_t now_ms) {
    m_lastTickMs = now_ms;
    // 闪烁逻辑：如果 flash=true，计算当前应该亮还是灭
    // 简化：实际闪烁由 QML 通过 flash 属性驱动
    (void)now_ms;
}

IndicatorState* IndicatorRuntime::findIndicator(const char* id) {
    for (int i = 0; i < m_count; i++) {
        if (m_table[i].id != nullptr && strcmp(m_table[i].id, id) == 0) {
            return &m_states[i];
        }
    }
    return nullptr;
}

int IndicatorRuntime::activeCount() const {
    int n = 0;
    for (int i = 0; i < m_count; i++) {
        if (m_states[i].on) n++;
    }
    return n;
}

bool IndicatorRuntime::isOn(const char* id) const {
    if (id == nullptr) return false;  // 防御
    for (int i = 0; i < m_count; i++) {
        if (m_table[i].id != nullptr && strcmp(m_table[i].id, id) == 0) {
            return m_states[i].on;
        }
    }
    return false;
}
