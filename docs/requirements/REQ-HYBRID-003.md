#REQ-HYBRID-003|纯电续航里程显示 (EV Range)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   can_ids.yaml (ev_range) / alarm_rules.yaml (ev_range_low) / indicators.yaml (ev_range_warn_light) / REQ-HYBRID-001.md
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:ev_range (L195) + alarm_rules.yaml:ev_range_low (L212) + indicators.yaml:ev_range_warn_light (L94)

---

## 1. 概述

### 1.1 需求描述
显示当前纯电模式下的剩余续航里程，低于阈值时触发提示。

### 1.2 背景与动机
纯电续航里程是PHEV车型的重要指标，驾驶员需要了解还能纯电行驶多远。

### 1.3 相关需求
- REQ-HYBRID-001: 混动汽车仪表盘特有功能基线

---

## 2. 功能需求

### 2.1 信号定义
| 字段 | 建议CAN ID | 类型 | 单位 | 范围 |
|------|-----------|------|------|------|
| ev_range | 0x307 | uint16 | km | 0~300 |

### 2.2 显示要求
- 格式: 整数 km（"{ev_range} km"）
- 分辨率: 1 km
- SOC=0 或无法计算时: 显示 "---"
- ev_range < 5km: 触发提示报警

### 2.3 报警规则
| 报警 | 条件 | 优先级 | 动作 |
|------|------|--------|------|
| ev_range_low | ev_range < 5 km | Medium | 提示横幅 |

---

## 3. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 (信号) | `config/can_ids.yaml` (ev_range, L195, uint16, km, 范围 0~300) |
| 报警规则 | `config/alarm_rules.yaml` (ev_range_low L212: ev_range<5km Medium 提示横幅 + ev_range_warn_light) |
| 关联 L2 组件 | `src/layer2/alarm_runtime.cpp` (`onValueChanged("ev_range", v)` 触发) / `src/layer2/trip_computer.cpp` (启动时读 baseline_ev_range 算 range_confidence_pct) |
| 指示灯 | `config/indicators.yaml` (ev_range_warn_light L94) — 由 ev_range_low 规则联动 |
| QML 关联 | `src/ui/DashboardMain.qml` 间接通过 AlarmBanner / IndicatorLight 反映; 原始 ev_range 数值不在 TripPanel 直接显示, 通过 trip_computer range_confidence 间接体现 |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 ev_range_low 规则 + trip_computer range_confidence, PR 31 批量同步元数据) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 元数据头部 + §3 实现追踪批量同步: 状态 Proposed → Implemented, 实现版本 + can_ids.yaml/alarm_rules.yaml/indicators.yaml 引用, 关联 trip_computer range_confidence (PR 31) | can-dash-jd-autopilot |
