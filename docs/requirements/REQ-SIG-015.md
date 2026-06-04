#REQ-SIG-015|发动机转速信号 (engine_rpm)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: High
**来源**:   can_ids.yaml (已有) / REQ-HYBRID-001.md
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L153 + src/layer3/shm_data_source.cpp:L328

---

## 1. 概述

### 1.1 信号描述
发动机转速信号，反映发动机当前转速。

### 1.2 相关需求
- REQ-ALM-008: engine_boost_active (基于 engine_rpm 和 energy_mode)
- REQ-ALM-010: engine_fault_alarm
- REQ-HYBRID-001: 发动机启停状态

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
| 名称 | engine_rpm |
| 字节位置 | [0, 1] (bytes 0-1) |
| 位宽 | 16 bits |
| 字节序 | little endian |
| 类型 | uint16 |
| 公式 | x |
| 单位 | rpm |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 最小值 | 0 rpm |
| 最大值 | 8000 rpm |

### 2.4 超时与监控
- 超时: 500ms (can_signal_status.yaml)
- max_delta: 1000 rpm/100ms

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x305 |
| 字节 | can_ids.yaml | byte | [0, 1] |
| 公式 | can_ids.yaml | formula | x |
| 单位 | can_ids.yaml | unit | rpm |
| 超时 | can_signal_status.yaml | timeout_ms | 500 |

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
