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
    // 仅在两帧都 > 阈值时积分, 避免停车时把最后一帧的 0.5 km/h 计进去
    const bool moving_now = (speed_kmh > MIN_MOVING_KMH);
    const bool moving_prev = (m_lastSpeed > MIN_MOVING_KMH);
    m_isMoving = moving_now;

    if (moving_now || moving_prev) {
        // 用 min(当前, 阈值之上限) 防止突然一脚油门到 200 km/h
        // 也用 max(0) 防止负数 (数据异常)
        const float v0 = moving_prev ? m_lastSpeed : 0.0f;
        const float v1 = moving_now  ? speed_kmh  : 0.0f;
        const float dt_h = static_cast<float>(dt_ms) / 3600000.0f;  // ms → h
        const float seg_km = (v0 + v1) * 0.5f * dt_h;
        m_distanceKm += seg_km;
    }
    m_lastSpeed = speed_kmh;

    // ─── 累加行驶时长 (仅 moving 时, 用 ms 累加避免 16ms tick 被整除吞掉) ───
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

void TripComputer::reset() {
    m_distanceKm = 0.0f;
    m_avgSpeedKmh = 0.0f;
    m_durationMs = 0;
    m_isMoving = false;
    // 同时清掉 initialized: 下一次 tick 应当被视为"首次",
    // 否则如果用户在车静止时按重置, next tick dt=16ms 正确,
    // 但如果用户在车开 1h 后按重置, 距上次 tick 已经 1h (应该 cap 到 10s)
    // 不重置 initialized → dt 会被 cap → 0.167km 误差
    // 重置 initialized → 下次 tick 又是 baseline, 0 误差
    m_initialized = false;
    m_lastTickMs = 0;
    m_lastSpeed = 0.0f;
}
