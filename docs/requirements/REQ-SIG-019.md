#REQ-SIG-019|发动机故障标志信号 (engine_fault)
=========================================

**状态**:   Approved
**类型**:   Safety
**优先级**: High
**来源**:   can_ids.yaml (已有) / alarm_rules.yaml
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
发动机故障标志位，当发动机系统检测到故障时置位。

### 1.2 背景与动机
发动机故障可能影响行车安全，属于高优先级报警。ISO 26262 ASIL B。

### 1.3 相关需求
- REQ-ALM-010: engine_fault_alarm
- REQ-IND-009: engine_run_light (故障时闪烁)

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x305 |
| 来源 | ENGINE |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | engine_fault |
| 字节位置 | byte 0 |
| 位宽 | 1 bit |
| 类型 | uint8 |
| 公式 | x |
| 单位 | - |

### 2.3 有效范围
| 值 | 状态 |
|-----|------|
| 0 | 正常 |
| 1 | 故障 |

### 2.4 报警逻辑
- engine_fault == 1 → 触发 engine_fault_alarm
  - engine_run_light 闪烁 (2Hz, 红色)
  - 报警横幅显示 "发动机故障！"

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x305 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 1 |
| 超时 | can_signal_status.yaml | timeout_ms | 500 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 报警规则 | `config/alarm_rules.yaml` (engine_fault_alarm) |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
