#REQ-SIG-011|副驾占用信号 (passenger_occupied)
=========================================

**状态**:   Approved
**类型**:   Functional
**优先级**: Medium
**来源**:   can_ids.yaml (已有)
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
副驾座椅占用检测信号，用于判断副驾是否有人坐。

### 1.2 相关需求
- REQ-SIG-013: passenger_buckled (安全带状态)
- REQ-SYS-004: SeatBeltRuntime (安全带监控)

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x2F1 |
| 来源 | SEAT_P |
| 周期 | 500ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | passenger_occupied |
| 字节位置 | byte 0 |
| 位宽 | 1 bit |
| 类型 | uint8 |
| 公式 | x |
| 单位 | - |

### 2.3 有效范围
| 值 | 状态 |
|-----|------|
| 0 | 无人 |
| 1 | 有人 |

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x2F1 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 1 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
