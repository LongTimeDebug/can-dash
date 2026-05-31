#REQ-SIG-007|电池温度信号 (battery_temp)
=========================================

**状态**:   Approved
**类型**:   Functional, Safety
**优先级**: High
**来源**:   can_ids.yaml (已有) / REQ-HYBRID-002.md
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
电池包温度信号，与电机温度是不同传感器，存在于同一个CAN帧的不同字节。

### 1.2 背景与动机
电池温度影响充放电性能和安全性，需要与电机温度区分显示。

### 1.3 相关需求
- REQ-HYBRID-002: 电池温度显示与报警

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x186040F3 |
| 来源 | BMS |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | battery_temp |
| 字节位置 | byte 5 |
| 位宽 | 8 bits |
| 类型 | int8 |
| 公式 | x |
| 单位 | degC |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 正常范围 | -40 ~ 85 degC |
| 高温阈值 | > 65 degC |
| 危险高温 | > 75 degC |
| 低温阈值 | < -10 degC |

### 2.4 报警规则
| 报警 | 条件 | 优先级 | 动作 |
|------|------|--------|------|
| bat_temp_high | battery_temp > 65 C | High | 报警横幅 + bat_warn_light 闪烁 |
| bat_temp_low | battery_temp < -10 C | Medium | 提示横幅 |
| bat_temp_critical | battery_temp > 75 C | Critical | 强制报警横幅 |

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
| 实现文件 | `config/can_ids.yaml` (待新增 byte 5) |
| 报警规则 | `config/alarm_rules.yaml` (待新增) |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
