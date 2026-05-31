#REQ-SIG-001|电池电压信号 (bat_volt)
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
电池包总电压信号，反映电池包当前工作电压，是电池健康状态的核心指标。

### 1.2 相关需求
- REQ-ALM-001: bat_overvolt (电池过压报警)
- REQ-ALM-002: bat_undervolt (电池欠压报警)
- REQ-SIG-002: bat_curr (同属BMS)
- REQ-SIG-003: bat_soc (同属BMS)

---

## 2. 信号规格

### 2.1 CAN 帧信息
| 字段 | 值 |
|------|-----|
| CAN ID | 0x186040F3 |
| 来源 | BMS |
| 周期 | 100ms |

### 2.2 数据字段
| 字段 | 值 |
|------|-----|
| 名称 | bat_volt |
| 字节位置 | [0, 1] (bytes 0-1) |
| 位宽 | 16 bits |
| 字节序 | little endian |
| 类型 | float |
| 公式 | x / 10.0 |
| 单位 | V |

### 2.3 有效范围
| 字段 | 值 |
|------|-----|
| 正常范围 | 280V ~ 420V |
| 过压阈值 | > 420V |
| 欠压阈值 | < 280V |

### 2.4 显示格式
- 格式: "{:.1f}V"
- 精度: 1位小数
- 超时显示: "---"

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| CAN ID | can_ids.yaml | can_id | 0x186040F3 |
| 字节 | can_ids.yaml | byte | [0, 1] |
| 公式 | can_ids.yaml | formula | x / 10.0 |
| 单位 | can_ids.yaml | unit | V |
| 超时 | can_signal_status.yaml | timeout_ms | 1000 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_ids.yaml` |
| 监控配置 | `config/can_signal_status.yaml` |
| 生成代码 | `src/generated/can_field_def.h` |
| 验证日期 | - |
| 验证结果 | - |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
