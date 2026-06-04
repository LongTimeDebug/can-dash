#REQ-SIG-006|电机温度信号 (motor_temp)
=========================================

**状态**:   Implemented
**类型**:   Functional, Safety
**优先级**: High
**来源**:   can_ids.yaml (已有) / INDEX
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L68 + src/layer3/shm_data_source.cpp:L320

---

## 1. 概述

### 1.1 信号描述
驱动电机温度信号，用于电机过热报警。

### 1.2 相关需求
- REQ-ALM-004: motor_overtemp (motor_temp > 120 C)

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
| 名称 | motor_temp |
| 字节位置 | byte 4 |
| 位宽 | 8 bits |
| 类型 | uint8 |
| 公式 | x - 40 |
| 单位 | degC |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 正常范围 | -40 ~ 120 degC |
| 过温阈值 | > 120 degC |

### 2.4 超时与监控
- 超时: 2000ms
- 范围: -40 ~ 200 degC

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x101 |
| 字节 | can_ids.yaml | byte | 4 |
| 公式 | can_ids.yaml | formula | x - 40 |
| 单位 | can_ids.yaml | unit | degC |
| 超时 | can_signal_status.yaml | timeout_ms | 2000 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 监控配置 | `config/can_signal_status.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 35 同步) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented (PR 35 SIG 批量同步) | requirements-document-agent |
