// trip_computer.h
// Layer 2: 派生指标
//   小计里程 / 平均车速 / 行驶时长 / 能耗 / 续航可信度
// 纯 C++，无 Qt，无动态内存
//
// 派生逻辑:
//   - trip_distance_km:    ∫ speed_kmh × dt  (梯形积分)
//   - trip_duration_s:     ∑ dt  (speed > MIN_MOVING_KMH 时计为行驶)
//   - trip_avg_speed_kmh:  distance / duration
//   - energy_kwh:          ∫ V × I × dt  (bat_curr > 0 时为放电, 单位 kWh)
//   - efficiency_kwh100km: energy / distance × 100
//                          起步前 distance < 0.5km 时返回 0 (避免除零 + 起步噪音)
//   - range_confidence_pct: 实际 SOC 消耗率 / 显示 ev_range 消耗率 × 100
//                          显示消耗率 = 启动时 ev_range / 启动时 SOC
//                          实际消耗率 = 已驶里程 / (启动时 SOC - 当前 SOC)
//                          续航可信度 < 100% = 实际比显示耗电快 = 表显续航虚高
//                          续航可信度 > 100% = 实际比显示省电 = 表显续航保守
//
// 设计要点:
//   - tickEnergy(now_ms, volt, curr, soc, ev_range) 由 ShmDataSource 每 16ms 调
//   - tick(now_ms, speed_kmh) 同步调用一次, 保持时间基准一致
//   - reset() 清零 (QML "重置小计" 按钮)
//   - 速度 < MIN_MOVING_KMH 视为停止, 距离/时长不计 (避免怠速噪音)
//   - dt cap 到 10s, 防止挂起唤醒后距离/能耗暴增
//   - range_confidence: 启动时记录 baseline_soc + baseline_ev_range,
//     之后 SOC 下降 > 0.5% 才开始算, 避免启动噪音

#pragma once
#include <cstdint>

class TripComputer {
public:
    TripComputer();

    // 主入口 (基础里程/时长/均速)
    void tick(uint64_t now_ms, float speed_kmh);

    // 能耗 + 续航可信度入口 (同帧调用, 共享 m_lastTickMs 时间基准)
    // volt/curr 来自 bat_volt/bat_curr
    // soc/ev_range 来自 bat_soc/ev_range
    void tickEnergy(uint64_t now_ms, float volt, float curr,
                    float soc_pct, float ev_range_km);

    // 重置全部计数器
    void reset();

    // 查询
    float   tripDistanceKm() const    { return m_distanceKm; }
    float   tripAvgSpeedKmh() const   { return m_avgSpeedKmh; }
    uint32_t tripDurationS() const    { return static_cast<uint32_t>(m_durationMs / 1000); }
    bool    isMoving() const          { return m_isMoving; }
    float   energyKWh() const         { return m_energyKWh; }
    float   efficiencyKWh100Km() const;
    float   rangeConfidencePct() const;

    // 配置
    static constexpr float MIN_MOVING_KMH = 1.0f;    // 停车阈值
    static constexpr uint64_t MAX_DT_MS = 10000;     // dt 上限
    static constexpr float MIN_SOC_DELTA_PCT = 0.5f; // 续航可信度计算所需最小 SOC 变化
    static constexpr float MIN_DISTANCE_KM = 0.5f;   // 能耗显示最小里程 (起步噪音过滤)

private:
    // ─── 里程/时长/均速 ───
    float      m_distanceKm   = 0.0f;
    float      m_avgSpeedKmh  = 0.0f;
    uint64_t   m_durationMs   = 0;
    bool       m_isMoving     = false;
    bool       m_initialized  = false;
    uint64_t   m_lastTickMs   = 0;
    float      m_lastSpeed    = 0.0f;

    // ─── 能耗 ───
    float      m_energyKWh    = 0.0f;
    float      m_lastVolt     = 0.0f;  // 上帧电压 (梯形积分用)
    float      m_lastCurr     = 0.0f;  // 上帧电流
    uint64_t   m_lastEnergyMs = 0;     // 上次能耗 tick 的时间戳
    bool       m_energyInitialized = false; // 首次能耗 tick 仅记基线

    // ─── 续航可信度 ───
    bool       m_rangeBaselineSet = false;  // 是否已记录启动 baseline
    float      m_startSoc     = 0.0f;       // 启动 SOC
    float      m_startEvRange = 0.0f;       // 启动时显示续航
    float      m_lastSoc      = 0.0f;       // 上帧 SOC (用于判断变化量)
    float      m_rangeConfidence = 100.0f;  // 0-100, 默认 100% (没数据时)
};
