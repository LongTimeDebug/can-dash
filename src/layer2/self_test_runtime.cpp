// self_test_runtime.cpp
// 显示自检 runtime 实现（REQ-SYS-005）
#include "self_test_runtime.h"

#include "time_util.h"
#include <cstdio>

namespace candash {

namespace {
// 关键信号（这些信号卡死会触发 FAIL）
bool isCriticalSignal(const char* name) {
    static const char* kCritical[] = {
        "vehicle_speed", "bat_volt", "bat_soc", "motor_rpm", "motor_temp",
        nullptr
    };
    for (int i = 0; kCritical[i] != nullptr; ++i) {
        if (std::strcmp(name, kCritical[i]) == 0) return true;
    }
    return false;
}
}  // namespace

SelfTestRuntime::SelfTestRuntime()
    : m_signals(nullptr),
      m_signalCount(0),
      m_checks(nullptr),
      m_criticalTotal(0),
      m_criticalReceivedCount(0),
      m_warnStuckCount(0),
      m_criticalStuckCount(0),
      m_outOfRangeCount(0),
      m_status(SelfTestStatus::NOT_READY),
      m_inited(false) {}

void SelfTestRuntime::init(const SignalDef* signals, int count) {
    m_signals = signals;
    m_signalCount = count;
    m_checks = new SignalCheck[count]();
    m_criticalTotal = 0;
    m_criticalReceivedCount = 0;
    m_warnStuckCount = 0;
    m_criticalStuckCount = 0;
    m_outOfRangeCount = 0;
    m_status = SelfTestStatus::NOT_READY;
    m_inited = true;

    // 初始化每条信号的检查项
    for (int i = 0; i < count; ++i) {
        m_checks[i].name = signals[i].name;
        m_checks[i].last_update_ms = 0;
        m_checks[i].last_value = 0.0f;
        m_checks[i].ever_received = false;
        m_checks[i].in_range = true;
        m_checks[i].critical = isCriticalSignal(signals[i].name);
        if (m_checks[i].critical) ++m_criticalTotal;
    }
}

void SelfTestRuntime::onValueChanged(const char* display_key, float value, uint64_t now_ms) {
    if (!m_inited) return;
    for (int i = 0; i < m_signalCount; ++i) {
        if (std::strcmp(m_checks[i].name, display_key) != 0) continue;
        // 找到信号
        m_checks[i].last_update_ms = now_ms;
        m_checks[i].last_value = value;
        m_checks[i].ever_received = true;
        // 范围检查
        if (m_signals[i].min_value != 0.0f || m_signals[i].max_value != 0.0f) {
            m_checks[i].in_range = (value >= m_signals[i].min_value &&
                                     value <= m_signals[i].max_value);
        } else {
            m_checks[i].in_range = true;  // 无范围限制
        }
        // 累计 critical 接收计数（用 ever_received 标志，避免 now_ms=0 误判）
        if (m_checks[i].critical) {
            ++m_criticalReceivedCount;  // 每次 onValueChanged 都 +1 不可取，改为 once
        }
        return;
    }
}

void SelfTestRuntime::evaluateStatus(uint64_t now_ms) {
    m_warnStuckCount = 0;
    m_criticalStuckCount = 0;
    m_outOfRangeCount = 0;
    // 重数：基于 ever_received 重新统计 m_criticalReceivedCount
    int critical_received_now = 0;

    for (int i = 0; i < m_signalCount; ++i) {
        const SignalCheck& c = m_checks[i];
        if (!c.ever_received) continue;

        if (c.critical) ++critical_received_now;

        // 卡死检测：now - last > timeout
        uint64_t age = now_ms - c.last_update_ms;
        if (age > m_signals[i].timeout_ms) {
            if (c.critical) ++m_criticalStuckCount;
            else ++m_warnStuckCount;
        }
        // 越界检测
        if (!c.in_range) ++m_outOfRangeCount;
    }
    m_criticalReceivedCount = critical_received_now;

    // 状态机
    if (!isReady()) {
        m_status = SelfTestStatus::NOT_READY;
    } else if (m_criticalStuckCount > 0 || m_outOfRangeCount >= 3) {
        m_status = SelfTestStatus::FAIL;
    } else if (m_warnStuckCount > 0 || m_outOfRangeCount > 0) {
        m_status = SelfTestStatus::WARN;
    } else {
        m_status = SelfTestStatus::OK;
    }
}

void SelfTestRuntime::tick(uint64_t now_ms) {
    if (!m_inited) return;
    evaluateStatus(now_ms);
}

void SelfTestRuntime::getSummary(char* out, int out_size) const {
    const char* status_str = "NOT_READY";
    switch (m_status) {
        case SelfTestStatus::OK:        status_str = "OK"; break;
        case SelfTestStatus::WARN:      status_str = "WARN"; break;
        case SelfTestStatus::FAIL:      status_str = "FAIL"; break;
        case SelfTestStatus::NOT_READY: status_str = "NOT_READY"; break;
    }
    std::snprintf(out, out_size,
        "status=%s critical_received=%d/%d critical_stuck=%d warn_stuck=%d out_of_range=%d",
        status_str, m_criticalReceivedCount, m_criticalTotal,
        m_criticalStuckCount, m_warnStuckCount, m_outOfRangeCount);
}

void SelfTestRuntime::reset() {
    m_criticalReceivedCount = 0;
    m_warnStuckCount = 0;
    m_criticalStuckCount = 0;
    m_outOfRangeCount = 0;
    m_status = SelfTestStatus::NOT_READY;
    for (int i = 0; i < m_signalCount; ++i) {
        m_checks[i].last_update_ms = 0;
        m_checks[i].in_range = true;
        m_checks[i].ever_received = false;
    }
}

}  // namespace candash
