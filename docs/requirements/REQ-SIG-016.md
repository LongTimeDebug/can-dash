#REQ-SIG-016|充电状态信号 (charge_status)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   can_ids.yaml (已有) / REQ-HYBRID-001.md
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L171 + src/layer3/shm_data_source.cpp:L329

---

## 1. 概述

### 1.1 信号描述
充电状态信号，标识当前充电模式（未充/慢充/快充/预约）。

### 1.2 相关需求
- REQ-IND-011: charge_light (充电灯控制)
- REQ-ALM-011: charge_fault_alarm
- REQ-HYBRID-001: 充电状态显示

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x306 |
| 来源 | CHG_STATUS |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | charge_status |
| 字节位置 | byte 0 |
| 位宽 | 2 bits |
| 类型 | uint8 |
| 公式 | x |
| 单位 | - |

### 2.3 状态定义
| 值 | 状态 |
|-----|------|
| 0 | 未充电 |
| 1 | 慢充 |
| 2 | 快充 |
| 3 | 预约充电 |

### 2.4 超时与监控
- 超时: 1000ms (can_signal_status.yaml)

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x306 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 2 |
| 超时 | can_signal_status.yaml | timeout_ms | 1000 |

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
