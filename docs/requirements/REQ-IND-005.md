#REQ-IND-005|高压指示灯
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: indicators.yaml:high_voltage_light (L32) + src/ui/IndicatorLight.qml

---

## 1. 概述

### 1.1 需求描述
高压指示灯, 高压系统上电时点亮橙色 HV 图标, 位置 (20, 20) 屏幕左上角, 尺寸 50×50.

### 1.2 相关需求
- REQ-IND-006: 高压指示灯 (同主题, PR 33 已同步, 实际 indicators.yaml L39 是 engine_run_light, IND-005 是 L32 high_voltage_light — 区分清楚)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| ID | indicators.yaml:high_voltage_light | ✓ |
| 位置 | position | x=20, y=20 (左上角) |
| 尺寸 | size | 50×50 |
| 状态 | 高压上电时橙色常亮 | ✓ |

---

## 3. 实现追踪

| 实现文件 | `config/indicators.yaml` (L32-37, high_voltage_light) |
| QML 组件 | `src/ui/IndicatorLight.qml` (image_on=hv_orange.png, image_off=hv_dim.png) |
| 关联 L2 组件 | IndicatorRuntime (PR 9, 监听 high_voltage_active 联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
