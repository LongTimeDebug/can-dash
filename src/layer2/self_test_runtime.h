// self_test_runtime.h
// 显示自检 runtime（REQ-SYS-005）
//
// 职责：
//   - 启动时执行"全通道 ping"自检（所有信号初始置为 NaN，期待收到值）
//   - 运行时监控"信号卡死"（timeout > 配置阈值）
//   - 监控"信号越界"（值超出 [min, max] 范围）
//   - 输出 self_test_status 枚举（OK / WARN / FAIL / NOT_READY）
//
// 设计原则：
//   - 无 Qt 依赖（Layer 2 纯 C++）
//   - 无动态分配（车规 MISRA-C:2012）
//   - tick() 由 can-processor 周期性调用
#pragma once

#include <cstdint>
#include <cstring>
#include "../generated/signal_def.h"

namespace candash {

// 自检状态枚举
enum class SelfTestStatus : uint8_t {
    NOT_READY = 0,  // 启动后未收到所有必要信号
    OK        = 1,  // 全部正常
    WARN      = 2,  // 部分次要信号卡死或越界
    FAIL      = 3,  // 关键信号（vehicle_speed/bat_volt/bat_soc）卡死
};

// 单个信号的自检状态
struct SignalCheck {
    const char* name;          // 信号名
    uint64_t    last_update_ms; // 最后一次值更新时间戳（ms）
    float       last_value;     // 最后一次值
    bool        ever_received;  // 是否曾经收到过（避免 last_update_ms=0 与 "t=0 时收到" 歧义）
    bool        in_range;       // 当前值是否在合法范围
    bool        critical;       // 是否关键信号
};

class SelfTestRuntime {
public:
    SelfTestRuntime();

    // 初始化（接收信号表）
    void init(const SignalDef* signals, int count);

    // 注册一个信号的更新（由 can_converter 调用）
    void onValueChanged(const char* display_key, float value, uint64_t now_ms);

    // tick（周期性，1Hz 足够）
    void tick(uint64_t now_ms);

    // 获取当前自检状态
    SelfTestStatus status() const { return m_status; }

    // 获取自检摘要（用于日志/调试）
    // 返回格式："status=X critical_stuck=N warn_stuck=N out_of_range=N"
    void getSummary(char* out, int out_size) const;

    // 启动后是否已经收到至少一次所有关键信号
    bool isReady() const { return m_criticalReceivedCount == m_criticalTotal; }

    // 重置（用于冷启动后重置自检状态）
    void reset();

private:
    void evaluateStatus(uint64_t now_ms);

    const SignalDef* m_signals;
    int              m_signalCount;
    SignalCheck*     m_checks;  // 大小 = m_signalCount
    int              m_criticalTotal;
    int              m_criticalReceivedCount;
    int              m_warnStuckCount;
    int              m_criticalStuckCount;
    int              m_outOfRangeCount;
    SelfTestStatus   m_status;
    bool             m_inited;
};

}  // namespace candash
