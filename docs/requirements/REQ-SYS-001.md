#REQ-SYS-001|CAN 总线超时检测
=========================================

**状态**:   Implemented
**类型**:   Reliability
**优先级**: High
**来源**:   config/* (已有) / INDEX
**创建日期**: 2026-06-04
**实现版本**: src/layer2/can_signal_monitor.cpp (L92) + config/can_signal_status.yaml

---

## 1. 概述

### 1.1 需求描述
CAN 总线超时检测, 每个信号配置 timeout_ms (vehicle_speed 500ms, bat_volt 1000ms, etc), 超时后判定信号失效, 通过 can_signal_monitor 推送到 SelfTestRuntime.

### 1.2 相关需求
- REQ-SIG-001~019: 19 条 CAN 信号超时配置 (can_signal_status.yaml)

---

## 2. 规格

| 参数 | YAML 字段 | 值 |
|------|----------|-----|
| 监控器 | can_signal_monitor.cpp L92 | elapsed >= timeout_ms |
| vehicle_speed | can_signal_status.yaml L7 | 500ms |
| bat_volt | can_signal_status.yaml L13 | 1000ms |
| bat_soc | can_signal_status.yaml L19 | 1000ms |
| 失效后动作 | 推送到 SelfTestRuntime | PR 17 |

---

## 3. 实现追踪

| 实现文件 | `src/layer2/can_signal_monitor.cpp` (L92, timeout 检测) + `config/can_signal_status.yaml` (L1+ timeout_ms 定义) |
| 关联 L2 组件 | SelfTestRuntime (PR 17, 复用超时检测做信号自检) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-06-04 | 1.0 | 初始创建 (PR 36 补 8 个无 .md 历史欠账) | requirements-document-agent |
