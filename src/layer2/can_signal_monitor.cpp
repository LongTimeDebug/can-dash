// can_signal_monitor.cpp
#include "can_signal_monitor.h"
#include "time_util.h"
#include <cmath>
#include <cstdio>

CanSignalMonitor::CanSignalMonitor(const MonitorCallbacks& cb)
    : m_cb(cb) {}

CanSignalMonitor::~CanSignalMonitor() {
    // m_states 是 init() 中 new[] 出来的，必须释放
    // m_historyPool 是类的静态成员数组，生命周期 = 对象生命周期，无需释放
    delete[] m_states;
    m_states = nullptr;
}

void CanSignalMonitor::init(const SignalMonitorDef* table, int table_count) {
    m_table = table;
    m_count = table_count;

    // 重新 init 安全性：先释放旧的 m_states（pool 是静态数组，无需释放）
    if (m_states) {
        delete[] m_states;
        m_states = nullptr;
    }
    m_poolUsed = 0;  // 每次 init 重置 pool 游标

    if (table_count <= 0) return;
    m_states = new SignalState[table_count]();

    for (int i = 0; i < table_count; i++) {
        m_states[i].def = &table[i];
        m_states[i].quality = SIGNAL_NEVER_RECEIVED;
        m_states[i].lastValue = 0.0f;
        m_states[i].smoothedValue = 0.0f;
        m_states[i].prevValue = 0.0f;
        m_states[i].lastUpdateMs = 0;
        m_states[i].firstSeenMs = 0;
        m_states[i].received = false;
        m_states[i].historyIndex = 0;

        // 平滑窗口从静态池分配，禁止 new[] 堆分配（避免析构时泄漏）
        // window 必须同时：smoothing 启用 + 1..MAX_SIGNAL_HISTORY + 池有剩余槽位
        uint8_t win = table[i].smoothing_window;
        if (table[i].smoothing && win > 0 && win <= MAX_SIGNAL_HISTORY &&
            m_poolUsed < MAX_SIGNAL_MONITORS) {
            m_states[i].history = m_historyPool[m_poolUsed];
            m_states[i].historyCount = win;
            m_poolUsed++;
        } else {
            // 不启用平滑（smoothing=false / window=0 / window 超限 / 池满）
            m_states[i].history = nullptr;
            m_states[i].historyCount = 0;
        }
    }
}

// NOLINT(bugprone-easily-swappable-parameters) — CAN 帧: (id, dlc, data)
void CanSignalMonitor::onCanFrame(uint32_t can_id, float value) {  // NOLINT(bugprone-easily-swappable-parameters)
    SignalState* state = findByCanId(can_id);
    if (!state) return;

    const SignalMonitorDef* def = state->def;
    state->prevValue = state->lastValue;
    state->lastValue = value;
    state->received = true;
    uint64_t now = candash::now_monotonic_ms();
    state->lastUpdateMs = now;

    if (state->firstSeenMs == 0) state->firstSeenMs = now;

    // 范围检测
    if (value < def->min_value || value > def->max_value) {
        updateQuality(state, SIGNAL_INVALID_RANGE, now);
        return;
    }

    // 突变检测（仅在前序质量正常/超时时检测：异常质量的上一次值不可信）
    if (def->max_delta > 0 && state->received &&
        (state->quality == SIGNAL_GOOD || state->quality == SIGNAL_STALE)) {
        float delta = std::fabs(value - state->prevValue);
        if (delta > def->max_delta) {
            updateQuality(state, SIGNAL_ABNORMAL_DELTA, now);
            return;
        }
    }

    // 平滑
    if (def->smoothing && state->history) {
        state->history[state->historyIndex] = value;
        state->historyIndex = (state->historyIndex + 1) % state->historyCount;

        float sum = 0.0f;
        for (int i = 0; i < state->historyCount; i++) sum += state->history[i];
        state->smoothedValue = sum / static_cast<float>(state->historyCount);
    } else {
        state->smoothedValue = value;
    }

    updateQuality(state, SIGNAL_GOOD, now);

    if (m_cb.onValueUpdated) {
        m_cb.onValueUpdated(def->name, state->smoothedValue, m_cb.user_data);
    }
}

void CanSignalMonitor::tick(uint64_t now_ms) {
    m_lastTickMs = now_ms;

    for (int i = 0; i < m_count; i++) {
        SignalState* state = &m_states[i];
        if (!state->received) continue;

        uint64_t elapsed = now_ms - state->lastUpdateMs;
        if (elapsed >= state->def->timeout_ms) {
            updateQuality(state, SIGNAL_STALE, now_ms);
        }
    }
}

SignalState* CanSignalMonitor::findByCanId(uint32_t can_id) {
    for (int i = 0; i < m_count; i++) {
        if (m_table[i].can_id == can_id) return &m_states[i];
    }
    return nullptr;
}

void CanSignalMonitor::updateQuality(SignalState* state, SignalQuality q, uint64_t now_ms) {
    if (state->quality == q) return;
    state->quality = q;
    if (m_cb.onQualityChanged) {
        m_cb.onQualityChanged(state->def->name, q, m_cb.user_data);
    }
    (void)now_ms;
}

SignalQuality CanSignalMonitor::getQuality(const char* signal) const {
    for (int i = 0; i < m_count; i++) {
        if (strcmp(m_table[i].name, signal) == 0) return m_states[i].quality;
    }
    return SIGNAL_NEVER_RECEIVED;
}

float CanSignalMonitor::getSmoothedValue(const char* signal) const {
    for (int i = 0; i < m_count; i++) {
        if (strcmp(m_table[i].name, signal) == 0) return m_states[i].smoothedValue;
    }
    return 0.0f;
}
