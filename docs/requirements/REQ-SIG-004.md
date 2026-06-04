#REQ-SIG-004|制动信号 (brake)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: High
**来源**:   can_ids.yaml (已有) / INDEX
**创建日期**: 2026-05-31
**实现版本**: can_ids.yaml:L50 + src/layer3/shm_data_source.cpp:L318

---

## 1. 概述

### 1.1 信号描述
制动踏板位置信号，反映驾驶员踩刹车的力度。

### 1.2 相关需求
- REQ-HYBRID-001: 能量回收（制动能量回收时触发）
- REQ-SYS-003: Limp-Home

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x203 |
| 来源 | VCPU |
| 周期 | 50ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | brake |
| 字节位置 | byte 2 |
| 位宽 | 8 bits |
| 类型 | uint8 |
| 公式 | x * 0.4 |
| 单位 | % |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 最小值 | 0% |
| 最大值 | 100% |

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x203 |
| 字节 | can_ids.yaml | byte | 2 |
| 公式 | can_ids.yaml | formula | x * 0.4 |
| 单位 | can_ids.yaml | unit | % |

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
