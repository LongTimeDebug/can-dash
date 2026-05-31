#REQ-SIG-003|车速信号 (vehicle_speed)
=========================================

**状态**:   Approved
**类型**:   Functional
**优先级**: Critical
**来源**:   can_ids.yaml (已有) / INDEX
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 信号描述
车辆实时行驶速度，是仪表盘最核心的显示数据之一。

### 1.2 相关需求
- REQ-UI-003: GaugeCanvas (仪表表盘)
- REQ-SYS-004: 安全带监控 (速度阈值触发)

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
| 名称 | vehicle_speed |
| 字节位置 | [3, 4] (bytes 3-4) |
| 位宽 | 16 bits |
| 字节序 | little endian |
| 类型 | float |
| 公式 | x / 10.0 |
| 单位 | km/h |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 最小值 | 0 km/h |
| 最大值 | 300 km/h (display_layout max=260) |

### 2.4 显示格式
- 格式: "{:.0f}" (整数)
- 超时显示: "---"
- 异常值检测: max_delta = 50 km/h per 100ms

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x203 |
| 字节 | can_ids.yaml | byte | [3, 4] |
| 公式 | can_ids.yaml | formula | x / 10.0 |
| 单位 | can_ids.yaml | unit | km/h |
| 超时 | can_signal_status.yaml | timeout_ms | 500 |
| 最大突变 | can_signal_status.yaml | max_delta | 50 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 监控配置 | `config/can_signal_status.yaml` |
| 显示配置 | `config/display_layout.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
