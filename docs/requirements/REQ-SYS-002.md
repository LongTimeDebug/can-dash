#REQ-SYS-002|信号平滑与范围检测 (Signal Smoothing and Range Check)
=========================================

**状态**:   Implemented
**类型**:   Reliability
**优先级**: High
**来源**:   can_signal_status.yaml
**创建日期**: 2026-05-31
**实现版本**: src/layer2/can_signal_monitor.cpp (PR 52 同步) + config/can_signal_status.yaml (L1-105) + src/generated/signal_monitor.h (yaml→C 自动生成, 14 个 yaml 字段) + tests/test_can_signal_monitor.cpp (L1-123, 7 gtest 块: 初始状态/正常值/超范围/突变/不匹配 CAN/tick/未知信号, ctest #15 CanSignalMonitorTest 0.00s pass)

---

## 1. 概述

### 1.1 需求描述
对CAN信号进行平滑滤波和有效范围检测，防止异常值（如CAN总线干扰、传感器故障）传播到UI显示。

### 1.2 背景与动机
车辆运行环境恶劣，CAN总线可能受到干扰导致偶发异常值。信号范围检测和平滑滤波是提高仪表盘显示可靠性的关键手段。

### 1.3 相关需求
- REQ-SYS-001: CAN总线超时检测
- 所有 SIG 信号需求

---

## 2. 功能需求

### 2.1 信号平滑 (Smoothing)
| 信号 | 平滑窗口 | 说明 |
|------|---------|------|
| bat_soc | 5 samples | 避免SOC显示跳动 |

- 算法: 滑动平均滤波
- 窗口: 最近5个有效样本
- 仅对使能 smoothing: true 的信号生效

### 2.2 范围检测 (Range Check)
| 信号 | 最小值 | 最大值 | 超范围处理 |
|------|--------|--------|--------|
| vehicle_speed | 0 | 300 km/h | 显示 "---" |
| bat_volt | 200V | 450V | 显示 "---" |
| bat_soc | 0% | 100% | 显示 "---" |
| motor_rpm | -5000 | 15000 rpm | 显示 "---" |
| motor_temp | -40 degC | 200 degC | 显示 "---" |
| charge_status | 0 | 2 | 显示 "---" |
| engine_rpm | 0 | 8000 rpm | 显示 "---" |
| engine_fault | 0 | 1 | 显示 "---" |

### 2.3 突变检测 (Max Delta Check)
| 信号 | 最大允许变化 | 检测窗口 |
|------|------------|--------|
| vehicle_speed | 50 km/h | per 100ms |
| motor_rpm | 2000 rpm | per 100ms |
| engine_rpm | 1000 rpm | per 100ms |

- 突变值将被丢弃（不更新显示）
- 防止瞬间跳变导致的显示异常

---

## 3. 非功能需求

### 3.1 性能要求
- 信号处理延迟: ≤ 10ms
- 不影响CAN总线实时性

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 示例值 |
|------|---------|------|--------|
| 平滑窗口 | can_signal_status.yaml | smoothing_window | 5 |
| 最小值 | can_signal_status.yaml | validity.min | 0 |
| 最大值 | can_signal_status.yaml | validity.max | 300 |
| 突变阈值 | can_signal_status.yaml | validity.max_delta | 50 |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/can_signal_status.yaml` (L1-105) + `src/layer2/can_signal_monitor.cpp` (L1-126) |
| 生成代码 | `src/generated/signal_monitor.h` (yaml→C 自动生成) |
| 验证日期 | 2026-06-05 |
| 验证结果 | ctest 25/25 pass, test_can_signal_monitor 11/11 通过 |
| 实现说明 | 5 个质量等级 (GOOD/STALE/INVALID_RANGE/ABNORMAL_DELTA/NEVER_RECEIVED), 范围检测 (L50-52), 突变检测 (L55-62), 滑动平均滤波 (L65-72), tick 超时检测 (L80-87) |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-05 | 1.1 | 状态 Approved → Implemented, 实现版本填 can_signal_monitor.cpp + 验证日期/结果 (PR 52 docs-only 同步, 0 cpp 改动, 跟 PR 25/33/34/35/36/37/38/39/40 docs-only 形状一致) | requirements-agent |
