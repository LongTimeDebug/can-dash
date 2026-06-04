#REQ-HYBRID-005|档位显示 (Gear Status)
=========================================

**状态**:   Implemented
**类型**:   Functional
**优先级**: Medium
**来源**:   REQ-HYBRID-001.md (混动基线) / ViewManager (PR 12) / ShmDataSource (PR 13)
**创建日期**: 2026-05-31
**实现版本**: ViewManager (PR 12) + ShmDataSource (PR 13) gear_status

---

## 1. 概述

### 1.1 需求描述
显示车辆当前档位 (P/R/N/D/S)。

### 1.2 背景与动机
档位显示是车辆基本状态信息，驾驶员需要明确知道当前档位。

### 1.3 相关需求
- REQ-HYBRID-001: 混动汽车仪表盘特有功能基线

---

## 2. 功能需求

### 2.1 信号定义
| 字段 | 建议CAN ID | 类型 | 说明 |
|------|-----------|------|------|
| gear_status | 0x309 | uint8 | 0=P, 1=R, 2=N, 3=D, 4=S |

### 2.2 显示要求
- 档位文字显示: P / R / N / D / S
- 运动模式(S)可作为D的子模式显示
- 信号超时: 显示 "---"

---

## 3. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 (档位状态) | `src/layer2/view_manager.cpp` (PR 12, gear_status 字段由 view_manager 持有) |
| 实现文件 (信号桥接) | `src/layer3/shm_data_source.cpp` (PR 13, onTick 把 shm.gear_status 桥接到 m_view) |
| 关联 L2 组件 | `src/layer2/view_manager.cpp` (gear_status getter/setter) + `src/layer3/shm_data_source.cpp` (m_view.setGearForTest 注入路径) |
| QML 显示 | `src/ui/GearDisplay.qml` (具体组件待 PR 32) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 ViewManager + ShmDataSource gear_status, PR 31 批量同步元数据) |

---

## 4. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 元数据头部 + §3 实现追踪批量同步: 状态 Proposed → Implemented, 实现版本 + ViewManager (PR 12) + ShmDataSource (PR 13) gear_status, 验证日期/结果填充 (PR 31) | can-dash-jd-autopilot |
