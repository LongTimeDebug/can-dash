// trip_computer.cpp
// Layer 2: 派生指标实现

#include "trip_computer.h"

TripComputer::TripComputer()
    : m_distanceKm(0.0f)
    , m_avgSpeedKmh(0.0f)
    , m_durationMs(0)
    , m_isMoving(false)
    , m_initialized(false)
    , m_lastTickMs(0)
    , m_lastSpeed(0.0f)
    , m_energyKWh(0.0f)
    , m_lastVolt(0.0f)
    , m_lastCurr(0.0f)
    , m_lastEnergyMs(0)
    , m_energyInitialized(false)
    , m_rangeBaselineSet(false)
    , m_startSoc(0.0f)
    , m_startEvRange(0.0f)
    , m_lastSoc(0.0f)
    , m_rangeConfidence(100.0f)
{}

void TripComputer::tick(uint64_t now_ms, float speed_kmh) {
    // ─── 首次调用: 仅记录基准时间, 不算距离/时长 ───
    // (不能用 m_lastTickMs == 0 当哨兵 — t=0 调用时会与未初始化混淆)
    if (!m_initialized) {
        m_lastTickMs = now_ms;
        m_lastSpeed = speed_kmh;
        m_isMoving = (speed_kmh > MIN_MOVING_KMH);
        m_initialized = true;
        return;
    }

    // ─── 计算 dt, cap 防跳变 ───
    uint64_t dt_ms = now_ms - m_lastTickMs;
    if (dt_ms > MAX_DT_MS) {
        dt_ms = MAX_DT_MS;
    }
    m_lastTickMs = now_ms;

    // ─── 梯形积分 distance = (v0 + v1) / 2 × dt ───
    const bool moving_now = (speed_kmh > MIN_MOVING_KMH);
    const bool moving_prev = (m_lastSpeed > MIN_MOVING_KMH);
    m_isMoving = moving_now;

    if (moving_now || moving_prev) {
        const float v0 = moving_prev ? m_lastSpeed : 0.0f;
        const float v1 = moving_now  ? speed_kmh   : 0.0f;
        const float dt_h = static_cast<float>(dt_ms) / 3600000.0f;
        const float seg_km = (v0 + v1) * 0.5f * dt_h;
        m_distanceKm += seg_km;
    }
    m_lastSpeed = speed_kmh;

    // ─── 累加行驶时长 (仅 moving 时) ───
    if (moving_now) {
        m_durationMs += dt_ms;
    }

    // ─── 平均速度 (除零保护) ───
    if (m_durationMs > 0) {
        m_avgSpeedKmh = m_distanceKm / (static_cast<float>(m_durationMs) / 3600000.0f);
    } else {
        m_avgSpeedKmh = 0.0f;
    }
}

void TripComputer::tickEnergy(uint64_t now_ms, float volt, float curr,
                              float soc_pct, float ev_range_km) {
    // ─── 首次调用: 仅记录基线 ───
    if (!m_energyInitialized) {
        m_lastVolt = volt;
        m_lastCurr = curr;
        m_lastSoc = soc_pct;
        m_lastEnergyMs = now_ms;
        if (!m_rangeBaselineSet) {
            m_startSoc = soc_pct;
            m_startEvRange = ev_range_km;
            m_rangeBaselineSet = true;
        }
        m_energyInitialized = true;
        return;
    }

    // ─── dt (用 m_lastEnergyMs 独立维护, 不与 tick 共享) ───
    uint64_t dt_ms = now_ms - m_lastEnergyMs;
    if (dt_ms > MAX_DT_MS) dt_ms = MAX_DT_MS;
    m_lastEnergyMs = now_ms;

    // ─── 能耗积分 (梯形: V × max(0, I)) ───
    // bat_curr > 0 = 放电 (电池输出), < 0 = 充电 (再生制动)
    // 只累计放电, 用 max(0, curr)
    const float curr_pos_now = (curr > 0.0f) ? curr : 0.0f;
    const float curr_pos_prev = (m_lastCurr > 0.0f) ? m_lastCurr : 0.0f;
    const float power_w_pos_now = volt * curr_pos_now;
    const float power_w_pos_prev = m_lastVolt * curr_pos_prev;
    const float avg_power_w = (power_w_pos_now + power_w_pos_prev) * 0.5f;
    const float dt_h = static_cast<float>(dt_ms) / 3600000.0f;
    m_energyKWh += (avg_power_w * dt_h) / 1000.0f;  // Wh → kWh

    m_lastVolt = volt;
    m_lastCurr = curr;

    // ─── 续航可信度 ───
    if (m_rangeBaselineSet && m_startSoc > 0.0f) {
        const float soc_dropped = m_startSoc - soc_pct;  // 正值 = 已消耗
        if (soc_dropped > MIN_SOC_DELTA_PCT) {
            const float actual_km_per_pct = m_distanceKm / soc_dropped;
            const float displayed_km_per_pct = m_startEvRange / m_startSoc;
            if (displayed_km_per_pct > 0.001f) {
                float conf = (actual_km_per_pct / displayed_km_per_pct) * 100.0f;
                if (conf < 0.0f) conf = 0.0f;
                if (conf > 999.0f) conf = 999.0f;  // 防异常值爆掉 UI
                m_rangeConfidence = conf;
            }
        }
    }
    m_lastSoc = soc_pct;
}

float TripComputer::efficiencyKWh100Km() const {
    if (m_distanceKm < MIN_DISTANCE_KM) {
        return 0.0f;  // 起步噪音: 0.5km 内不显示
    }
    return (m_energyKWh / m_distanceKm) * 100.0f;
}

float TripComputer::rangeConfidencePct() const {
    if (!m_rangeBaselineSet) {
        return 100.0f;  // 没 baseline (刚启动还没数据) → 默认 100%
    }
    return m_rangeConfidence;
}

void TripComputer::reset() {
    m_distanceKm = 0.0f;
    m_avgSpeedKmh = 0.0f;
    m_durationMs = 0;
    m_isMoving = false;
    m_initialized = false;
    m_lastTickMs = 0;
    m_lastSpeed = 0.0f;

    m_energyKWh = 0.0f;
    m_lastVolt = 0.0f;
    m_lastCurr = 0.0f;
    m_lastEnergyMs = 0;
    m_energyInitialized = false;

    m_rangeBaselineSet = false;
    m_startSoc = 0.0f;
    m_startEvRange = 0.0f;
    m_lastSoc = 0.0f;
    m_rangeConfidence = 100.0f;
}
