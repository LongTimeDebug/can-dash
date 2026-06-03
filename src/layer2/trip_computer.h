// trip_computer.h
// Layer 2: 小计里程 / 平均车速 / 行驶时长 派生指标
// 纯 C++，无 Qt，无动态内存
//
// 派生逻辑:
//   - trip_distance_km: ∫ speed_kmh × dt  (梯形积分)
//   - trip_duration_s:  ∑ dt (speed > 1 km/h 时计为行驶)
//   - trip_avg_speed_kmh: distance / duration (避免除 0)
//
// 设计要点:
//   - tick(now_ms, speed_kmh) 由外部驱动 (ShmDataSource 每 16ms 调用一次)
//   - reset() 清零 (用户在 QML 点 "重置小计" 按钮时调用, UI 接入见 TODO)
//   - 速度 < MIN_MOVING_KMH 视为停止, 距离/时长都不计 (避免怠速噪音)
//   - 时间跳变 (例如挂起后唤醒) cap 到 10s, 防止 dt 过大导致距离暴增

#pragma once
#include <cstdint>

class TripComputer {
public:
    TripComputer();

    // 主入口: 外部每 ~16ms 调用, speed_kmh 来自 vehicle_speed
    void tick(uint64_t now_ms, float speed_kmh);

    // 重置全部计数器 (用户操作)
    void reset();

    // 查询
    float   tripDistanceKm() const   { return m_distanceKm; }
    float   tripAvgSpeedKmh() const  { return m_avgSpeedKmh; }
    uint32_t tripDurationS() const   { return static_cast<uint32_t>(m_durationMs / 1000); }
    bool    isMoving() const         { return m_isMoving; }

    // 配置
    static constexpr float MIN_MOVING_KMH = 1.0f;  // 低于此值视为停车
    static constexpr uint64_t MAX_DT_MS = 10000;   // dt 上限 (防时间跳变)

private:
    float      m_distanceKm   = 0.0f;
    float      m_avgSpeedKmh  = 0.0f;
    uint64_t   m_durationMs   = 0;  // 用 ms 累加, 否则 16ms tick / 1000 = 0
    bool       m_isMoving     = false;
    bool       m_initialized  = false;  // 必须用 flag 区分 "未初始化" 和 "t=0 已初始化"
    uint64_t   m_lastTickMs   = 0;
    float      m_lastSpeed    = 0.0f;
};
