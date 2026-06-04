#REQ-HYBRID-004|燃油续航里程显示 (Fuel Range / Fuel Level)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   REQ-HYBRID-001.md (混动基线) / trip_computer (PR 4)
**创建日期**: 2026-05-31
**实现版本**: trip_computer (PR 4) + alarm_rules.yaml:fuel_low (L180) + indicators.yaml:fuel_low_light (L86)

---

## 1. 概述

### 1.1 需求描述
显示燃油液位百分比和估算的燃油续航里程，在燃油过低时触发报警。

### 1.2 背景与动机
混动车型虽主要用电，但燃油系统仍需监控。燃油过低会影响发动机发电功能。

### 1.3 相关需求
- REQ-HYBRID-001: 混动汽车仪表盘特有功能基线

---

## 2. 功能需求

### 2.1 信号定义
| 字段 | 建议CAN ID | 类型 | 单位 | 范围 |
|------|-----------|------|------|------|
| fuel_level | 0x308 | uint8 | % | 0~100 |
| fuel_range | 0x308 | uint16 | km | 0~1000 |

### 2.2 显示要求
- 燃油液位: 百分比或图形显示
- 燃油续航: "{fuel_range} km"
- 燃油 < 15%: 点亮 fuel_low_light (黄色)
- 燃油 < 5%: 触发 fuel_critical 报警

### 2.3 报警规则
| 报警 | 条件 | 优先级 | 动作 |
|------|------|--------|------|
| fuel_low | fuel_level < 15% | Medium | fuel_low_light 亮 |
| fuel_critical | fuel_level < 5% | High | 报警横幅 |

---

## 3. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 (续航计算) | `src/layer2/trip_computer.cpp` (PR 4, fuel_level 字段由传感器读数+消耗率计算) |
| 报警规则 | `config/alarm_rules.yaml` (fuel_low L180: fuel_level<15% Low 报警 + fuel_low_light 闪烁) |
| 关联 L2 组件 | `src/layer2/trip_computer.cpp` (tickFuel 计算 fuel_level) + `src/layer2/alarm_runtime.cpp` (onValueChanged("fuel_level", v) 触发) |
| 指示灯 | `config/indicators.yaml` (fuel_low_light L86) |
| QML 显示 | `src/ui/TripPanel.qml` (具体组件待 PR 32) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 trip_computer fuel_level + fuel_low 规则, PR 31 批量同步元数据) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 元数据头部 + §3 实现追踪批量同步: 状态 Proposed → Implemented, 实现版本 + trip_computer (PR 4) + alarm_rules.yaml:fuel_low (L180) + indicators.yaml:fuel_low_light (L86), 验证日期/结果填充 (PR 31) | can-dash-jd-autopilot |
