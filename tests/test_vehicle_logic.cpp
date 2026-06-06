// test_vehicle_logic.cpp
// Layer 2 VehicleLogic 单元测试（纯 C++，无 Qt）

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cmath>
#include <limits>
#include <vector>
#include "../src/layer2/vehicle_logic.h"
#include "../src/layer2/event_bus.h"
#include "../src/layer2/time_util.h"  // candash::now_monotonic_ms() for absolute-time tick()
#include "../src/generated/vehicle_config.h"  // v3 探针: kDefaultVehicleConfig

// 验证 EventBus 发布的事件
static std::vector<std::string> published_keys;
static void clear_events() { published_keys.clear(); }

int main() {
    printf("=== VehicleLogic 单元测试 ===\n");

    VehicleConfigDef config;
    config.soc_warning_low = 15.0f;
    config.soc_critical_low = 5.0f;
    config.speed_max = 260.0f;
    config.precharge_timeout_ms = 3000;
    config.soc_smoothing_window = 5;

    VehicleLogic logic;
    logic.init(&config);

    // ─── 测试1：初始状态 ───
    printf("\n[测试1] 初始状态\n");
    assert(logic.getSpeed() == 0.0f);
    assert(!logic.isSpeedValid());
    assert(!logic.isReadyGo());
    assert(!logic.isHvActive());
    assert(logic.getPrechargeState() == PRECHARGE_IDLE);
    printf("  ✓ 初始值正确\n");

    // ─── 测试2：车速更新 ───
    printf("\n[测试2] 车速更新\n");
    logic.onSpeedUpdate(50.0f, true);
    assert(logic.getSpeed() == 50.0f);
    assert(logic.isSpeedValid());
    logic.onSpeedUpdate(0.0f, true);
    assert(logic.getSpeed() == 0.0f);
    printf("  ✓ 车速更新正确\n");

    // ─── 测试3：SOC 平滑 ───
    printf("\n[测试3] SOC 平滑\n");
    // 前5次更新填充窗口
    for (int i = 0; i < 5; i++) {
        logic.onSocUpdate(50.0f + i);  // 50, 51, 52, 53, 54
    }
    // 前5次：平均 = (50+51+52+53+54)/5 = 52.0
    assert(logic.getSmoothedSoc() > 51.9f && logic.getSmoothedSoc() < 52.1f);
    printf("  ✓ SOC 平滑窗口正确 (窗口=5)\n");

    // ─── 测试4：SOC 低电量判断 ───
    printf("\n[测试4] SOC 低电量判断\n");
    logic.onSocUpdate(20.0f);  // > 15，不触发低电量
    assert(!logic.isSocLow());
    assert(!logic.isSocCritical());

    logic.onSocUpdate(12.0f);  // 10 < 12 < 15，触发低电量
    assert(logic.isSocLow());
    assert(!logic.isSocCritical());

    logic.onSocUpdate(3.0f);   // < 5，触发临界
    assert(logic.isSocLow());
    assert(logic.isSocCritical());
    printf("  ✓ SOC 阈值判断正确\n");

    // ─── 测试5：驾驶模式字符串 ───
    printf("\n[测试5] 驾驶模式字符串\n");
    assert(strcmp(logic.getDriveModeStr(), "NORMAL") == 0);
    printf("  ✓ 默认驾驶模式 NORMAL\n");

    // ─── 测试6：预充电状态机 ───
    printf("\n[测试6] 预充电状态机\n");
    assert(logic.getPrechargeState() == PRECHARGE_IDLE);

    // 高压上电 → 开始预充电
    // 注意: vehicle_logic::onHvStatusUpdate() 内部用 candash::now_monotonic_ms() 记录 m_prechargeStartMs,
    //       tick(now_ms) 接收的是绝对时间 (uint64_t), 不是相对 delta.
    //       故测试须先捕获 "现在" 的 monotonic ms, 再传 "现在 + 600ms" 给 tick().
    const uint64_t hv_start_ms = candash::now_monotonic_ms();
    logic.onHvStatusUpdate(true);
    assert(logic.getPrechargeState() == PRECHARGE_ACTIVE);

    // 模拟预充电完成（tick 推进时间, 600ms > precharge_auto_done_ms=500）
    logic.tick(hv_start_ms + 600);
    assert(logic.getPrechargeState() == PRECHARGE_DONE);
    assert(logic.isReadyGo());
    printf("  ✓ 预充电状态机正确（ACTIVE → DONE → ReadyGo）\n");

    // ─── 测试7：预充电超时失败 ───
    printf("\n[测试7] 预充电超时失败\n");
    VehicleLogic logic2;
    logic2.init(&config);
    const uint64_t hv_start_ms2 = candash::now_monotonic_ms();
    logic2.onHvStatusUpdate(true);
    // tick(now + 3100ms) → elapsed = 3100ms >= precharge_timeout_ms=3000ms → PRECHARGE_FAILED
    logic2.tick(hv_start_ms2 + 3100);
    assert(logic2.getPrechargeState() == PRECHARGE_FAILED);
    printf("  ✓ 预充电超时检测正确\n");

    // ─── 测试8：高压下电 ───
    printf("\n[测试8] 高压下电\n");
    logic.onHvStatusUpdate(false);
    assert(logic.getPrechargeState() == PRECHARGE_IDLE);
    assert(!logic.isReadyGo());
    assert(!logic.isHvActive());
    printf("  ✓ 高压下电状态正确\n");

    // ─── 测试9：ReadyGo 逻辑 ───
    printf("\n[测试9] ReadyGo 逻辑\n");
    // 注意: tick(now_ms) 接收绝对 monotonic 时间, 不是 delta.
    //       ReadyGo 测试需在已知 monotonic 基线上推进时间.
    const uint64_t readygo_base_ms = candash::now_monotonic_ms();
    logic.tick(readygo_base_ms);
    logic.onHvStatusUpdate(true);
    logic.tick(readygo_base_ms + 600);  // 预充电完成 (> precharge_auto_done_ms=500)
    logic.tick(readygo_base_ms + 700);
    logic.onSpeedUpdate(0.1f, true);  // 静止
    logic.tick(readygo_base_ms + 800);
    // PRECHARGE_DONE 且 speed < readygo_speed_engage_kmh (默认 0.5) → readyGo
    assert(logic.isReadyGo());

    // 行驶中关闭 ReadyGo (speed > readygo_speed_disengage_kmh, 默认 5.0)
    logic.onSpeedUpdate(10.0f, true);
    logic.tick(readygo_base_ms + 900);
    assert(!logic.isReadyGo());
    printf("  ✓ ReadyGo 激活/关闭逻辑正确\n");

    // ─── 测试10：v3 探针 — yaml 生成的 kDefaultVehicleConfig 字段一致性 ───
    // 这是 v3 探针的核心断言: 改 yaml 后重新跑此测试,
    // 若任何阈值与 config/vehicle_thresholds.yaml 不一致则 fail.
    printf("\n[测试10] v3 探针: kDefaultVehicleConfig == vehicle_thresholds.yaml\n");
    assert(kDefaultVehicleConfig.soc_warning_low == 10.0f);
    assert(kDefaultVehicleConfig.soc_critical_low == 5.0f);
    assert(kDefaultVehicleConfig.speed_max == 260.0f);
    assert(kDefaultVehicleConfig.precharge_timeout_ms == 3000u);
    assert(kDefaultVehicleConfig.precharge_auto_done_ms == 500u);
    assert(kDefaultVehicleConfig.soc_smoothing_window == 5);
    assert(kDefaultVehicleConfig.readygo_speed_engage_kmh == 0.5f);
    assert(kDefaultVehicleConfig.readygo_speed_disengage_kmh == 5.0f);
    printf("  ✓ 8 个阈值与 yaml 一致 (soc_warn=10, soc_crit=5, speed_max=260,\n");
    printf("    precharge_timeout=3000ms, precharge_auto_done=500ms,\n");
    printf("    soc_window=5, readygo_engage=0.5kmh, readygo_disengage=5.0kmh)\n");

    // ─── 测试11：v3 探针 — init(nullptr) 走 yaml 默认 ───
    printf("\n[测试11] v3 探针: init(nullptr) 应用 yaml 默认值\n");
    VehicleLogic logic3;
    logic3.init(nullptr);  // 不传 config, 走 yaml
    assert(logic3.config().soc_warning_low == 10.0f);
    assert(logic3.config().readygo_speed_disengage_kmh == 5.0f);
    assert(logic3.config().precharge_auto_done_ms == 500u);
    // 跑一遍行为, 验证阈值生效
    const uint64_t logic3_start_ms = candash::now_monotonic_ms();
    logic3.onHvStatusUpdate(true);
    logic3.tick(logic3_start_ms + 600);  // > 500ms, 应该走 auto_done
    assert(logic3.getPrechargeState() == PRECHARGE_DONE);
    printf("  ✓ init(nullptr) 行为正确 (走 yaml 默认配置)\n");

    // ─── 测试12：边界检查 — SOC NaN 不掩盖低电量告警 ───
    printf("\n[测试12] 边界检查: SOC NaN/Inf 被拒绝, 不掩盖低电量告警\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        // 先把 SOC 设到低电量 (< 15)
        for (int i = 0; i < 5; i++) vl.onSocUpdate(8.0f);
        assert(vl.isSocLow());
        assert(vl.getSmoothedSoc() > 7.9f && vl.getSmoothedSoc() < 8.1f);
        // 注入 NaN — 期望: 拒绝更新, m_soc 保持 8.0, 告警继续触发
        const float saved_soc = vl.getSmoothedSoc();
        vl.onSocUpdate(std::nanf(""));
        assert(vl.getSmoothedSoc() == saved_soc);  // 平滑值未变
        assert(vl.isSocLow());  // 告警仍触发 (关键!)
        // 注入 +Inf
        vl.onSocUpdate(std::numeric_limits<float>::infinity());
        assert(vl.getSmoothedSoc() == saved_soc);
        assert(vl.isSocLow());
        // 注入 -Inf
        vl.onSocUpdate(-std::numeric_limits<float>::infinity());
        assert(vl.getSmoothedSoc() == saved_soc);
        assert(vl.isSocLow());
    }
    printf("  ✓ NaN/+Inf/-Inf 都不污染 m_soc, 低电量告警不被掩盖\n");

    // ─── 测试13：边界检查 — SOC 越界钳到 [0, 100] ───
    printf("\n[测试13] 边界检查: SOC 越界被钳位到 [0, 100]\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        // 先用正常值填充窗口, 让初始 m_soc 稳定
        for (int i = 0; i < 5; i++) vl.onSocUpdate(50.0f);
        // 越界 150 → 应被钳到 100
        vl.onSocUpdate(150.0f);
        assert(vl.getSmoothedSoc() <= 100.0f);
        assert(!vl.isSocCritical());
        // 越界 -10 → 应被钳到 0
        vl.onSocUpdate(-10.0f);
        // 多次 -10 后, 平滑窗口全是 0 → smoothed=0, 但 isSocLow 仍看 m_soc(0)
        assert(vl.isSocCritical());  // 0 < 5
    }
    printf("  ✓ SOC > 100 钳到 100, SOC < 0 钳到 0, 告警阈值仍正确\n");

    // ─── 测试14：边界检查 — 车速 NaN/Inf/负值/超速 ───
    printf("\n[测试14] 边界检查: 车速 NaN/负值/超速 都被安全处理\n");
    {
        VehicleLogic vl;
        vl.init(&config);  // speed_max=260
        // NaN → speed=0, valid=false
        vl.onSpeedUpdate(std::nanf(""), true);
        assert(vl.getSpeed() == 0.0f);
        assert(!vl.isSpeedValid());
        // +Inf → 同 NaN 处理
        vl.onSpeedUpdate(std::numeric_limits<float>::infinity(), true);
        assert(vl.getSpeed() == 0.0f);
        assert(!vl.isSpeedValid());
        // 负值 → 钳到 0
        vl.onSpeedUpdate(-5.0f, true);
        assert(vl.getSpeed() == 0.0f);
        assert(vl.isSpeedValid());  // 负值钳到 0, valid 保持原值
        // 超速 → 钳到 speed_max
        vl.onSpeedUpdate(9999.0f, true);
        assert(vl.getSpeed() == config.speed_max);
        assert(vl.isSpeedValid());
    }
    printf("  ✓ NaN/±Inf 拒绝 (valid=false, speed=0), 负值钳到 0, 超速钳到 speed_max\n");

    // ─── 测试15：边界检查 — 正常值不受影响 ───
    printf("\n[测试15] 边界检查: 正常输入 (0~speed_max) 行为不变\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        vl.onSpeedUpdate(60.0f, true);
        assert(vl.getSpeed() == 60.0f);
        assert(vl.isSpeedValid());
        vl.onSpeedUpdate(0.0f, true);
        assert(vl.getSpeed() == 0.0f);
        // 边界值: 正好等于 speed_max 不应被钳
        vl.onSpeedUpdate(config.speed_max, true);
        assert(vl.getSpeed() == config.speed_max);
        // 边界值: 正好 0 不应被钳
        vl.onSpeedUpdate(0.0f, true);
        assert(vl.getSpeed() == 0.0f);
    }
    printf("  ✓ 正常车速 (含边界) 不受影响\n");

    // ─── 测试16：驾驶模式自动检测 — ECO 触发 + 滞后 ───
    printf("\n[测试16] 驾驶模式自动检测 — ECO 触发 + 滞后 3000ms\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        // 默认 NORMAL
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        assert(!vl.isDriveModeManual());

        // 注入 ECO 条件: |power|<5kW, speed<60km/h
        vl.onPowerUpdate(2.0f);
        vl.onSpeedUpdate(30.0f, true);
        vl.onMotorRpmUpdate(1000);

        const uint64_t t0 = candash::now_monotonic_ms();

        // t0+1s: 首次 tick, candidate=ECO, since=t0+1s
        vl.tick(t0 + 1000);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        printf("  ✓ t0+1s, 候选 ECO 保持中, 仍 NORMAL\n");

        // t0+2.5s: 候选 ECO 保持 1.5s, 未达 3s 滞后
        vl.tick(t0 + 2500);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);

        // t0+4.1s: 候选 ECO 保持 3.1s, 滞后满足, 切到 ECO
        vl.tick(t0 + 4100);
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);
        assert(strcmp(vl.getDriveModeStr(), "ECO") == 0);
        printf("  ✓ t0+4.1s, 滞后满足, 切到 ECO\n");

        // 退出 ECO 条件: 功率突然增大 → candidate=NORMAL, 重置滞后
        vl.onPowerUpdate(20.0f);
        vl.tick(t0 + 5000);
        // candidate=NORMAL 刚建立, since=t0+5s, 未到 3s
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);  // 仍 ECO, 因为 NORMAL 候选没保持够
        vl.tick(t0 + 8200);
        // candidate=NORMAL 保持 3.2s, 滞后满足, 切到 NORMAL
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        printf("  ✓ 功率突增 → 3s 后回 NORMAL\n");
    }

    // ─── 测试17：驾驶模式自动检测 — SPORT 触发 (强放电) ───
    printf("\n[测试17] 驾驶模式自动检测 — SPORT 触发 (charge_power < -15kW)\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        vl.onSpeedUpdate(80.0f, true);
        vl.onPowerUpdate(-20.0f);  // 强放电
        vl.onMotorRpmUpdate(2000);

        const uint64_t t0 = candash::now_monotonic_ms();
        vl.tick(t0 + 1000);   // candidate=SPORT, since=t0+1s
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        vl.tick(t0 + 4500);   // candidate SPORT 保持 3.5s, 切到 SPORT
        assert(vl.getDriveMode() == DRIVE_MODE_SPORT);
        assert(strcmp(vl.getDriveModeStr(), "SPORT") == 0);
        printf("  ✓ 强放电 3.5s 后切到 SPORT\n");

        // 功率回正 → 切回 NORMAL
        vl.onPowerUpdate(2.0f);
        vl.tick(t0 + 5500);
        vl.tick(t0 + 8700);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        printf("  ✓ 功率正常 → 3s 后回 NORMAL\n");
    }

    // ─── 测试18：驾驶模式自动检测 — SPORT 触发 (高速+高RPM) ───
    printf("\n[测试18] 驾驶模式自动检测 — SPORT (speed>100 AND rpm>3000)\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        // 中等功率, 高速 + 高 RPM
        vl.onSpeedUpdate(120.0f, true);
        vl.onPowerUpdate(-5.0f);
        vl.onMotorRpmUpdate(3500);

        const uint64_t t0 = candash::now_monotonic_ms();
        vl.tick(t0 + 1000);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        vl.tick(t0 + 4500);
        assert(vl.getDriveMode() == DRIVE_MODE_SPORT);
        printf("  ✓ 高速+高RPM 3.5s 后切到 SPORT\n");
    }

    // ─── 测试19：驾驶模式手动覆盖 — 锁定 + 不被自动覆盖 ───
    printf("\n[测试19] 驾驶模式手动覆盖 — 锁定后 tick 不再自动切换\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        vl.setDriveMode(DRIVE_MODE_ECO, true);  // 手动锁定 ECO
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);
        assert(vl.isDriveModeManual());

        // 即使给了 SPORT 条件, 也不切
        const uint64_t t0 = candash::now_monotonic_ms();
        vl.onSpeedUpdate(120.0f, true);
        vl.onPowerUpdate(-30.0f);
        vl.onMotorRpmUpdate(4000);
        vl.tick(t0 + 5000);
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);
        printf("  ✓ 手动 ECO 锁定, 强 SPORT 条件仍不切换\n");

        // 解除手动 (manual=false) → 恢复自动
        vl.setDriveMode(DRIVE_MODE_NORMAL, false);
        assert(!vl.isDriveModeManual());
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        // 当前条件是 SPORT (speed=120, power=-30, rpm=4000)
        // setDriveMode 复位了 sinceMs=0, 所以下一个 tick 立即接受一次 SPORT 作为 baseline
        vl.tick(t0 + 5500);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);  // baseline 接受了 SPORT, 但 m_driveMode 仍 NORMAL
        vl.tick(t0 + 9000);
        // candidate=SPORT 已保持 3.5s, 切到 SPORT
        assert(vl.getDriveMode() == DRIVE_MODE_SPORT);
        printf("  ✓ manual=false 后 tick 重新参与自动检测, 强条件 3.5s 后切到 SPORT\n");
    }

    // ─── 测试20：驾驶模式 — 滞后内抖动不切换 ───
    printf("\n[测试20] 驾驶模式 — 滞后内抖动不切换\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        vl.onSpeedUpdate(30.0f, true);
        vl.onPowerUpdate(2.0f);  // ECO

        const uint64_t t0 = candash::now_monotonic_ms();
        vl.tick(t0 + 1000);
        vl.tick(t0 + 4500);  // 切到 ECO
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);

        // 在滞后窗口内 (3s) 抖动: ECO -> NORMAL -> ECO
        // candidate=NORMAL 重新开始计时, 但未满 3s 就变回 ECO
        vl.onPowerUpdate(10.0f);
        vl.tick(t0 + 5000);  // candidate=NORMAL, since=t0+5s
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);  // 仍 ECO (未到滞后)
        vl.onPowerUpdate(2.0f);
        vl.tick(t0 + 6000);  // candidate 变回 ECO, 重置
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);
        printf("  ✓ 滞后窗口内抖动不切换模式\n");
    }

    // ─── 测试21：驾驶模式 — onPowerUpdate NaN/Inf 拒绝 ───
    printf("\n[测试21] 驾驶模式 — onPowerUpdate NaN/Inf 拒绝, 保留 m_powerKw\n");
    {
        VehicleLogic vl;
        vl.init(&config);
        vl.onPowerUpdate(3.0f);
        assert(vl.getDriveMode() == DRIVE_MODE_NORMAL);
        // 注入 NaN — 期望: 拒绝, 保持 3.0
        vl.onPowerUpdate(std::nanf(""));
        vl.onPowerUpdate(std::numeric_limits<float>::infinity());
        vl.onPowerUpdate(-std::numeric_limits<float>::infinity());
        // 通过构造 ECO 条件 (|3.0|<5, speed<60) 验证 m_powerKw 仍是 3.0
        vl.onSpeedUpdate(30.0f, true);
        const uint64_t t0 = candash::now_monotonic_ms();
        vl.tick(t0 + 1000);
        vl.tick(t0 + 4500);
        assert(vl.getDriveMode() == DRIVE_MODE_ECO);
        printf("  ✓ NaN/±Inf 被拒绝, m_powerKw 保留 3.0, 自动检测仍按 3.0 走\n");
    }

    printf("\n所有测试通过。\n");
    return 0;
}
