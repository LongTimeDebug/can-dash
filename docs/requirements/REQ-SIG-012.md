#REQ-SIG-012|主驾安全带状态信号 (driver_buckled)
=========================================

**状态**:   Approved
**类型**:   Safety
**优先级**: High
**来源**:   can_ids.yaml (已有) / seat_belt.yaml
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
主驾安全带扣紧状态信号，用于安全带未系报警检测。

### 1.2 背景与动机
安全带是车辆最重要的被动安全装置。ISO 26262 ASIL B 安全相关。

### 1.3 相关需求
- REQ-SYS-004: SeatBeltRuntime (安全带监控逻辑)
- REQ-SIG-010: driver_occupied (配合使用)

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x3B0 |
| 来源 | SEAT_BELT |
| 周期 | 200ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | driver_buckled |
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
| CAN ID | can_ids.yaml | can_id | 0x3B0 |
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
