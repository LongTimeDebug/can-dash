// theme_manager.h
// Layer 2: 主题管理器 (Day/Night/Auto 颜色策略)
// 纯 C++，无 Qt，无 YAML 运行时
//
// 职责:
//   - 三种模式: DAY / NIGHT / AUTO
//   - AUTO 模式下根据当前小时数 + 黎明/黄昏小时阈值判断是日间还是夜间
//   - 暴露统一的 ThemeColors (背景/前景/强调/警告/严重) 给 Layer 3 读取
//   - tick() 推进时间 (AUTO 模式重新评估) — 由 ShmDataSource 16ms 驱动
//
// 设计要点 (PR-A: L2 + test, 不接数据流):
//   - 状态机简单: mode 字段 + is_day 派生标志
//   - tick() 不强制 16ms, 支持任意 dt, 测试可注入任意 (now_ms, hour)
//   - setCurrentHourForTest() 注入时间, 不依赖 wall clock, 避免时区/测试抖动
//   - 当前小时从 candash::now_monotonic_ms() 推算 (相对启动时间 + 启动时刻的小时)
//     → 实际部署时可改读 /dev/rtc 或 NTP
//   - 默认值: DAY (保守, 浅色背景文字对比好)
//
// 复用现有模式 (参照 TripComputer):
//   - 纯 C++ 类, 状态自包含, 无 Qt
//   - tick() 由上游按 16ms 节奏调用
//   - reset() 回到默认 DAY 模式
//
// 不在本 PR 范围 (后续 PR-B 接入数据流, PR-C 改 QML):
//   - 不修改 DisplaySnapshot / QtDataBinder / QML
//   - 不与 VehicleLogic 联动 (无 CAN 信号输入)

#pragma once

#include <cstdint>

namespace candash {

// 主题模式
enum class ThemeMode : uint8_t {
    DAY = 0,    // 强制日间
    NIGHT = 1,  // 强制夜间
    AUTO = 2    // 根据 hour 自动切换
};

// 5 色板: 背景/前景/强调/警告/严重
// ARGB 格式 (与 alarm_runtime 的 color 字段一致)
struct ThemeColors {
    uint32_t background;  // 主背景
    uint32_t foreground;  // 主文字
    uint32_t accent;      // 强调 (高亮数字/指针)
    uint32_t warning;     // 警告 (黄色)
    uint32_t critical;    // 严重 (红色)
};

class ThemeManager {
public:
    ThemeManager();

    // ─── 模式控制 ───
    void setMode(ThemeMode mode);
    ThemeMode currentMode() const { return m_mode; }
    bool isDay() const { return m_isDay; }

    // ─── AUTO 阈值 ───
    // 默认: 06:00-18:00 为日间, 其余为夜间
    void setSunriseHour(uint8_t hour) { m_sunriseHour = hour; }
    void setSunsetHour(uint8_t hour) { m_sunsetHour = hour; }
    uint8_t sunriseHour() const { return m_sunriseHour; }
    uint8_t sunsetHour() const { return m_sunsetHour; }

    // ─── 时间注入 ───
    // 当前小时 (0-23). 默认 12 (中午 = DAY)
    void setCurrentHour(uint8_t hour);
    uint8_t currentHour() const { return m_currentHour; }

    // ─── tick ───
    // 推进 (AUTO 模式重新评估). now_ms 当前未使用, 保留接口与 TripComputer 对齐
    void tick(uint64_t now_ms);

    // ─── 颜色查询 ───
    ThemeColors colors() const;
    uint32_t colorOf(const char* slot) const;  // "bg" / "fg" / "accent" / "warning" / "critical"

    // ─── 重置 ───
    void reset();

    // 预设配色 (测试/外部可读)
    static const ThemeColors kDayColors;
    static const ThemeColors kNightColors;

private:
    void evaluateAutoMode();
    static uint8_t normalizeHour(int hour);  // 把 hour mod 24 限制到 [0, 23]

    ThemeMode m_mode = ThemeMode::AUTO;
    bool      m_isDay = true;     // 派生: 当前是否为日间
    uint8_t   m_sunriseHour = 6;
    uint8_t   m_sunsetHour  = 18;
    uint8_t   m_currentHour = 12; // 默认中午, 启动时为 DAY
};

}  // namespace candash
