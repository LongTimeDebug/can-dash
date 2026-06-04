#REQ-IND-002|电量低指示灯
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: indicators.yaml:soc_warn_light (L11) + src/ui/IndicatorLight.qml

---

## 1. 概述

### 1.1 需求描述
电量低指示灯, SOC 低于 20% 时点亮橙色提示图标, 位置 (660, 260), 尺寸 60×60.

### 1.2 相关需求
- REQ-ALM-003: 电池电量严重低报警 (soc_critical_low, alarm_rules.yaml L53, 8%)
- REQ-ALM-012: 电池电量低报警 (bat_soc_low, alarm_rules.yaml L37, 10%)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| ID | indicators.yaml:soc_warn_light | ✓ |
| 位置 | position | x=660, y=260 |
| 尺寸 | size | 60×60 |
| 闪烁 | flash_on_fault | false (常亮) |
| 报警源 | alarm_rules.yaml:bat_soc_low/soc_critical_low | L37, L53 |

---

## 3. 实现追踪

| 实现文件 | `config/indicators.yaml` (L11-17, soc_warn_light) |
| QML 组件 | `src/ui/IndicatorLight.qml` (image_on=warning_soc_orange.png, image_off=warning_soc_dim.png) |
| 关联 L2 组件 | IndicatorRuntime (PR 9, 监听 bat_soc < 20% 联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
