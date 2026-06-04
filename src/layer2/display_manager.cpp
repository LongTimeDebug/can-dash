// display_manager.cpp
// Layer 2: LCD 背光超时管理 (无对应 REQ, 预留 REQ-SYS-00X 资源)
// 纯 C++，无 Qt，无动态内存
#include "display_manager.h"

DisplayManager::DisplayManager(uint8_t timeout_seconds)
    : m_timeoutSeconds(timeout_seconds)
    , m_state(BACKLIGHT_NORMAL)
    , m_idleMs(0)
    , m_lastTickMs(0)
    , m_firstTick(true)  // PR 51: 第一次 tick 初始化 (避免 m_lastTickMs=0 跟未初始化冲突)
    , m_lastSpeed(0.0f)
    , m_wasMoving(true)
    , m_stationaryMs(0)
{}

void DisplayManager::onUserInteraction() {
    m_idleMs = 0;
    if (m_state != BACKLIGHT_NORMAL) {
        m_state = BACKLIGHT_NORMAL;
    }
}

void DisplayManager::setVehicleSpeed(float speed_kmh) {
    m_lastSpeed = speed_kmh;
}

void DisplayManager::tick(uint64_t now_ms) {
    if (m_firstTick) {  // PR 51: 修原 m_lastTickMs==0 条件跟"未初始化"冲突, 改用 m_firstTick 标志
        m_firstTick = false;
        m_lastTickMs = now_ms;
        // PR 51: 同步设置 m_wasMoving 跟当前速度, 避免 init=true 造成
        // 第一次 tick 后从"动"到"停"的 stationaryMs 误重置
        m_wasMoving = (m_lastSpeed > 1.0f);
        if (!m_wasMoving) {
            m_stationaryMs = 0;  // 启动时已停车, 计时从 0 开始
        }
        return;
    }

    uint64_t elapsed = now_ms - m_lastTickMs;
    if (elapsed > 10000) elapsed = 10000;  // 防止时间跳变
    m_lastTickMs = now_ms;

    bool isMoving = (m_lastSpeed > 1.0f);  // 车速 > 1 km/h 视为行驶

    if (isMoving) {
        // 行驶中：背光正常，停车计时复位
        m_stationaryMs = 0;
        m_wasMoving = true;
        if (m_state != BACKLIGHT_NORMAL) {
            m_state = BACKLIGHT_NORMAL;
        }
    } else {
        // 停车中
        if (m_wasMoving) {
            // 刚从行驶变为停车，重置停车计时
            m_stationaryMs = 0;
            m_wasMoving = false;
        } else {
            // 持续停车
            m_stationaryMs += elapsed;
        }

        // 检查超时阈值
        uint64_t timeoutMs = (uint64_t)m_timeoutSeconds * 1000U;
        if (m_stationaryMs >= timeoutMs) {
            // 超时：背光降低
            if (m_state == BACKLIGHT_NORMAL) {
                m_state = BACKLIGHT_DIM;
            }
        }
    }

    // 闲置计时（用户无操作）
    m_idleMs += elapsed;
}
