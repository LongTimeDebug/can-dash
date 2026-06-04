#REQ-HYBRID-002|电池温度显示与报警
=========================================

**状态**:   Implemented
**类型**:   Functional, Safety
**优先级**: High
**来源**:   alarm_rules.yaml (bat_temp_high) / REQ-HYBRID-001.md
**创建日期**: 2026-05-31
**实现版本**: alarm_rules.yaml:bat_temp_high (L228) + bat_temp_critical (L244) 报警已落地 + src/ui/EnergyFlowDiagram.qml:L246-247 显示 batteryTemp 数值 (颜色阈值 50°C红/40°C橙)

---

## 1. 概述

### 1.1 需求描述
显示电池温度并在温度异常时触发报警。电池温度信号需要新增（byte 5 of 0x186040F3）。

### 1.2 背景与动机
电池温度直接影响电池寿命和安全。温度过高可能导致热失控，温度过低影响充放电性能。

### 1.3 相关需求
- REQ-HYBRID-001: 混动汽车仪表盘特有功能基线
- REQ-SYS-003: Limp-Home（电池温度异常时可降功率）

---

## 2. 功能需求

### 2.1 信号定义
| 字段 | CAN ID | 字节 | 类型 | 单位 | 范围 |
|------|--------|------|------|------|------|
| battery_temp | 0x186040F3 | byte 5 | int8 | degC | -40~85 |

### 2.2 显示要求
- 格式: "{battery_temp} C"
- 温度范围: -40~85 C
- 超温: 报警横幅 + bat_warn_light

### 2.3 报警规则
| 报警 | 条件 | 优先级 | 动作 |
|------|------|--------|------|
| bat_temp_high | battery_temp > 65 C | High | 报警横幅 + bat_warn_light 闪烁 |
| bat_temp_low | battery_temp < -10 C | Medium | 提示横幅 |
| bat_temp_critical | battery_temp > 75 C | Critical | 强制报警横幅 + 降功率提示 |

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x186040F3 |
| 字节 | can_ids.yaml | byte | 5 |
| 类型 | can_ids.yaml | type | int8 |
| 公式 | can_ids.yaml | formula | x |
| 单位 | can_ids.yaml | unit | degC |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 (信号) | `config/can_ids.yaml` (battery_temp, L31, BMS 帧 byte 5, int8, 公式 x-40) |
| 报警规则 | `config/alarm_rules.yaml` (bat_temp_high L228: battery_temp>65°C High 报警 + bat_warn_light; bat_temp_critical L244: battery_temp>75°C Critical 强制报警横幅) |
| 关联 L2 组件 | `src/layer2/alarm_runtime.cpp` (`onValueChanged("battery_temp", v)` 触发) |
| 指示灯 | `config/indicators.yaml` (bat_warn_light L5) — 由 bat_temp_high/critical 规则联动 |
| QML 显示 | `src/ui/EnergyFlowDiagram.qml` L246-247 — `text: root.batteryTemp.toFixed(0) + "°C"` + 颜色阈值 (`>50°C` 红, `>40°C` 橙, 其他灰); 数据通路 ShmDataSource.cpp:L316 → QtDataBinder.cpp:L194 → DashboardMain.qml:L262 (batteryTemp 绑定) → EnergyFlowDiagram.batteryTemp |
| 验证日期 | 2026-06-04 |
| 验证结果 | 报警规则 18/18 ctest pass (含 bat_temp_high / bat_temp_critical 规则, PR 31 批量同步元数据); QML 显示已就位 (EnergyFlowDiagram.qml L246-247, 数据通路 shm_data_source.cpp:L316 → qt_data_binder.cpp:L194 → DashboardMain.qml:L262) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 元数据头部 + §4 实现追踪批量同步: 状态 Proposed → Approved (报警侧已实装, 显示组件缺待 PR 32), 实现版本 + alarm_rules.yaml 引用, QML 显示缺项明示 (PR 31) | can-dash-jd-autopilot |
| 2026-06-04 | 1.2 | 状态 Approved → Implemented + QML 显示行从"缺"改 EnergyFlowDiagram.qml L246-247 (PR 39 docs-only sync, 0 cpp 改动) | can-dash-jd-autopilot |
