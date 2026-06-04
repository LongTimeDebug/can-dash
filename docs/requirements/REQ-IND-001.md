#REQ-IND-001|电池警告指示灯
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: High
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: indicators.yaml:bat_warn_light (L3) + src/ui/IndicatorLight.qml

---

## 1. 概述

### 1.1 需求描述
电池警告指示灯, 电池包过压/欠压/温度异常时点亮红色告警图标, 位置 (600, 260), 尺寸 60×60, 故障时 1Hz 闪烁.

### 1.2 相关需求
- REQ-ALM-001: 电池过压报警 (bat_overvolt, alarm_rules.yaml L5)
- REQ-ALM-002: 电池欠压报警 (bat_undervolt, alarm_rules.yaml L21)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| ID | indicators.yaml:bat_warn_light | ✓ |
| 位置 | position | x=600, y=260 |
| 尺寸 | size | 60×60 |
| 闪烁 | flash_on_fault | true (1Hz) |
| 报警源 | alarm_rules.yaml:bat_overvolt/undervolt | L5, L21 |

---

## 3. 实现追踪

| 实现文件 | `config/indicators.yaml` (L3-9, bat_warn_light) |
| QML 组件 | `src/ui/IndicatorLight.qml` (image_on=warning_bat_red.png, image_off=warning_bat_dim.png, flash_on_fault=true) |
| 关联 L2 组件 | IndicatorRuntime (PR 9, 监听 alarm_active 联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
