// vehicle_logic.h
// Layer 2: 车速/SOC/驾驶模式业务逻辑

#pragma once
#include <cstdint>
#include <cstring>
#include <array>

// v3 探针: 默认值由 yaml_to_c.py 从 config/vehicle_thresholds.yaml 生成
// (src/generated/vehicle_config.cpp → kDefaultVehicleConfig)
// 保留成员默认值仅作 "yaml 加载失败" 兜底 (理论上不会触发)
struct VehicleConfigDef {
    float  soc_warning_low = 10.0f;
    float  soc_critical_low = 5.0f;
    float  speed_max = 260.0f;
    uint32_t precharge_timeout_ms = 3000;
    int    soc_smoothing_window = 5;
    // tick() 内原硬编码魔数 (PR-v3 移入 yaml)
    uint32_t precharge_auto_done_ms = 500;
    float  readygo_speed_engage_kmh = 0.5f;
    float  readygo_speed_disengage_kmh = 5.0f;
};

enum DriveMode { DRIVE_MODE_ECO, DRIVE_MODE_NORMAL, DRIVE_MODE_SPORT };
enum PrechargeState { PRECHARGE_IDLE, PRECHARGE_ACTIVE, PRECHARGE_DONE, PRECHARGE_FAILED };

class VehicleLogic {
public:
    VehicleLogic();

    // v3 探针: init() 接受 nullptr 时使用 yaml 生成的 kDefaultVehicleConfig
    // (而非硬编码默认值)
    void init(const VehicleConfigDef* config);

    // 查询当前生效配置 (用于测试和诊断)
    const VehicleConfigDef& config() const { return m_config; }

    // 输入
    void onSpeedUpdate(float speed, bool valid);
    void onSocUpdate(float soc);
    void onHvStatusUpdate(bool active);
    // 驾驶模式检测信号 (功率 = charge_power; 负值=放电, 正值=回馈)
    // NaN/Inf 拒绝, 保留上一次值
    void onPowerUpdate(float charge_power_kw);
    void onMotorRpmUpdate(int16_t motor_rpm);

    // 手动覆盖驾驶模式 (manual=true 时, tick() 不会再自动切换)
    // 传 manual=false 等价于 "恢复自动模式"
    void setDriveMode(DriveMode mode, bool manual);

    // 定时调用
    void tick(uint64_t now_ms);

    // 查询
    float getSpeed() const { return m_speed; }
    bool  isSpeedValid() const { return m_speedValid; }
    float getSmoothedSoc() const;
    bool  isSocLow() const;
    bool  isSocCritical() const;
    bool  isReadyGo() const;
    bool  isHvActive() const { return m_hvActive; }
    PrechargeState getPrechargeState() const { return m_prechargeState; }
    DriveMode getDriveMode() const { return m_driveMode; }
    const char* getDriveModeStr() const;
    // 是否手动覆盖 (false=auto-detect)
    bool isDriveModeManual() const { return m_driveModeManual; }

    const char* name() const { return "VehicleLogic"; }

private:
    // 驾驶模式自动检测 (v3+ 增量: 之前 tick() 末尾是空注释)
    // 内部使用: 返回在 AUTO 模式下, 当前负载建议的驾驶模式
    //   - ECO:    |power| < eco_power_kw AND speed < eco_speed_kmh
    //   - SPORT:  power < sport_discharge_kw OR (speed > sport_speed_kmh AND rpm > sport_rpm)
    //   - NORMAL: 其他
    DriveMode detectAutoDriveMode() const;

    VehicleConfigDef  m_config;
    float             m_speed = 0.0f;
    float             m_lastSpeed = 0.0f;        // cppcheck: 必须初始化，避免 uninitMemberVar
    bool              m_speedValid = false;
    float             m_targetSpeed = 0.0f;
    float             m_soc = 0.0f;
    float             m_socSmoothed = 0.0f;
    float             m_lastSoc = 0.0f;
    std::array<float, 10> m_socHistory = {};
    int               m_socHistoryIndex = 0;
    DriveMode         m_driveMode = DRIVE_MODE_NORMAL;
    PrechargeState    m_prechargeState = PRECHARGE_IDLE;
    uint64_t          m_prechargeStartMs = 0;    // cppcheck: 必须初始化
    bool              m_readyGoActive = false;
    bool              m_hvActive = false;
    uint64_t          m_lastSpeedUpdateMs = 0;
    uint64_t          m_lastTickMs = 0;

    // ── 驾驶模式自动检测 (PR 增量, 之前是空注释) ─────────
    float             m_powerKw = 0.0f;           // 上次 charge_power, NaN/Inf 拒绝时保留
    int16_t           m_motorRpm = 0;             // 上次 motor_rpm
    bool              m_driveModeManual = false;  // true=锁定手动, false=自动
    DriveMode         m_driveModeCandidate = DRIVE_MODE_NORMAL;  // 自动检测中"候选"模式
    uint64_t          m_driveModeCandidateSinceMs = 0;            // 候选保持开始时间 (用于滞后)
};
