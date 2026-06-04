#REQ-SIG-013|副驾安全带状态信号 (passenger_buckled)
=========================================

**状态**:   Implemented
**类型**:   Safety
**优先级**: High
**来源**:   can_ids.yaml (已有) / seat_belt.yaml
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L112 + src/layer3/shm_data_source.cpp:L324

---

## 1. 概述

### 1.1 信号描述
副驾安全带扣紧状态信号。

### 1.2 相关需求
- REQ-SYS-004: SeatBeltRuntime
- REQ-SIG-011: passenger_occupied

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x3B1 |
| 来源 | SEAT_BELT_P |
| 周期 | 200ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | passenger_buckled |
| 字节位置 | byte 0 |
| 位宽 | 1 bit |
| 类型 | uint8 |
| 公式 | x |
| 单位 | - |

### 2.3 有效范围
| 值 | 状态 |
|-----|------|
| 0 | 未系 |
| 1 | 已系 |

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x3B1 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 1 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 监控配置 | `config/seat_belt.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
