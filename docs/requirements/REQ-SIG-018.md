#REQ-SIG-018|能量模式信号 (energy_mode)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: High
**来源**:   can_ids.yaml (已有) / REQ-HYBRID-001.md
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L183 + src/layer3/shm_data_source.cpp:L330

---

## 1. 概述

### 1.1 信号描述
能量模式信号，标识车辆当前的能量管理模式，是混动车型能量流显示的核心信号。

### 1.2 背景与动机
能量模式决定电机/发动机的驱动组合，是驾驶员了解车辆动力状态的关键信号。

### 1.3 相关需求
- REQ-ALM-006: ev_mode_active
- REQ-ALM-007: hybrid_mode_active
- REQ-ALM-008: engine_boost_active
- REQ-ALM-009: charge_mode_active
- REQ-HYBRID-001: 能量流显示

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x300 |
| 来源 | ENERGY_MODE |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | energy_mode |
| 字节位置 | byte 0 |
| 位宽 | 8 bits |
| 类型 | uint8 |
| 公式 | x |
| 单位 | - |

### 2.3 状态定义
| 值 | 模式 | 能量流显示 |
|-----|------|---------|
| 0 | EV (纯电) | ev_mode_light (绿) |
| 1 | HYBRID (混动) | hybrid_mode_light (蓝) |
| 2 | ENGINE_ONLY (发动机驱动) | engine_run_light (白) + energy_flow_light |
| 3 | CHARGE_MODE (充电) | energy_flow_light 闪烁 (黄) |

### 2.4 指示灯控制
- energy_mode == 0: ev_mode_light ON, hybrid_mode_light OFF, engine_run_light OFF
- energy_mode == 1: hybrid_mode_light ON, ev_mode_light OFF, engine_run_light OFF
- energy_mode == 2: engine_run_light ON, ev_mode_light OFF, hybrid_mode_light OFF, energy_flow_light ON
- energy_mode == 3: energy_flow_light 闪烁 (2Hz)

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x300 |
| 字节 | can_ids.yaml | byte | 0 |
| 位宽 | can_ids.yaml | bits | 8 |
| 公式 | can_ids.yaml | formula | x |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 控制规则 | `config/alarm_rules.yaml` (ev/hybrid/engine/charge_mode_active) |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 35 同步) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented (PR 35 SIG 批量同步) | requirements-document-agent |
