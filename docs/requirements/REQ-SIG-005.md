#REQ-SIG-005|电机转速信号 (motor_rpm)
=========================================

**状态**:   Approved
**类型**:   Functional
**优先级**: High
**来源**:   can_ids.yaml (已有) / INDEX
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
驱动电机实时转速信号，反映电机工作状态。

### 1.2 相关需求
- REQ-UI-003: GaugeCanvas (电机转速表)
- REQ-HYBRID-001: 能量流显示

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x101 |
| 来源 | MCU |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | motor_rpm |
| 字节位置 | [0, 1] (bytes 0-1) |
| 位宽 | 16 bits |
| 字节序 | little endian |
| 类型 | int16 |
| 公式 | x |
| 单位 | rpm |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 最小值 | -5000 rpm |
| 最大值 | 15000 rpm |
| 正常范围 | 0 ~ 12000 rpm |

### 2.4 超时与监控
- 超时: 1000ms
- max_delta: 2000 rpm per 100ms

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x101 |
| 字节 | can_ids.yaml | byte | [0, 1] |
| 类型 | can_ids.yaml | type | int16 |
| 单位 | can_ids.yaml | unit | rpm |
| 超时 | can_signal_status.yaml | timeout_ms | 1000 |
| 最大突变 | can_signal_status.yaml | max_delta | 2000 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 监控配置 | `config/can_signal_status.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
