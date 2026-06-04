#REQ-SIG-010|主驾占用信号 (driver_occupied)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   can_ids.yaml (已有)
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L90 + src/layer3/shm_data_source.cpp:L322

---

## 1. 概述

### 1.1 信号描述
主驾座椅占用检测信号，用于判断主驾是否有人坐。

### 1.2 相关需求
- REQ-SIG-012: driver_buckled (安全带状态)
- REQ-SYS-004: SeatBeltRuntime (安全带监控)

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x2F0 |
| 来源 | SEAT |
| 周期 | 500ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | driver_occupied |
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
| CAN ID | can_ids.yaml | can_id | 0x2F0 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 1 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 35 同步) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented (PR 35 SIG 批量同步) | requirements-document-agent |
