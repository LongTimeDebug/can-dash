#REQ-IND-004|Ready/Go 指示灯
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: indicators.yaml:ready_go_light (L25) + src/ui/IndicatorLight.qml

---

## 1. 概述

### 1.1 需求描述
Ready/Go 指示灯, 车辆就绪可行驶时点亮绿色, 位置 (780, 260), 尺寸 60×60.

### 1.2 相关需求
- REQ-SIG-003: 车速信号 (vehicle_speed, can_ids.yaml L42)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| ID | indicators.yaml:ready_go_light | ✓ |
| 位置 | position | x=780, y=260 |
| 尺寸 | size | 60×60 |
| 状态 | 车辆就绪时绿色常亮 | ✓ |

---

## 3. 实现追踪

| 实现文件 | `config/indicators.yaml` (L25-31, ready_go_light) |
| QML 组件 | `src/ui/IndicatorLight.qml` (image_on=ready_go_green.png, image_off=ready_go_dim.png) |
| 关联 L2 组件 | IndicatorRuntime (PR 9, 监听 vehicle_ready 信号联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
