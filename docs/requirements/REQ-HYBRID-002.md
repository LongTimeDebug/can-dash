#REQ-HYBRID-002|电池温度显示与报警
=========================================

**状态**:   Proposed
**类型**:   Functional, Safety
**优先级**: High
**来源**:   REQ-HYBRID-001.md (混动基线)
**创建日期**: 2026-05-31
**实现版本**: -

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
| 实现文件 | `config/can_ids.yaml` (待新增) |
| 报警规则 | `config/alarm_rules.yaml` (待新增) |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
