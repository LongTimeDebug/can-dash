#REQ-IND-003|电机温度警告指示灯
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: High
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: indicators.yaml:motor_warn_light (L18) + src/ui/IndicatorLight.qml

---

## 1. 概述

### 1.1 需求描述
电机温度警告指示灯, 电机温度超过阈值时点亮红色告警图标, 位置 (720, 260), 尺寸 60×60.

### 1.2 相关需求
- REQ-SIG-006: 电机温度信号 (motor_temp, can_ids.yaml L68)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| ID | indicators.yaml:motor_warn_light | ✓ |
| 位置 | position | x=720, y=260 |
| 尺寸 | size | 60×60 |
| 报警源 | motor_temp 越界 (can_signal_status.yaml L32, max=200) | ✓ |

---

## 3. 实现追踪

| 实现文件 | `config/indicators.yaml` (L18-24, motor_warn_light) |
| QML 组件 | `src/ui/IndicatorLight.qml` (image_on=warning_motor_red.png, image_off=warning_motor_dim.png) |
| 关联 L2 组件 | IndicatorRuntime (PR 9, 监听 motor_temp 越界联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
