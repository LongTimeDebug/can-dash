#REQ-SYS-004|安全带运行时监控 (SeatBeltRuntime)
=========================================

**状态**:   Implemented
**类型**:   Safety
**优先级**: High
**来源**:   config/seat_belt.yaml
**创建日期**: 2026-05-31
**实现版本**: SeatBeltRuntime (PR 23 L2+test 升级, 2026-06-04) — config/seat_belt.yaml:trigger.speed_threshold (L57), 监控 5 个座位 (driver L4 / passenger L15 / rear_left L26 / rear_center L36 / rear_right L46)

---

## 1. 概述

### 1.1 需求描述
实时监控驾驶员和乘客的安全带扣紧状态，在车速超过阈值且安全带未系时显示报警横幅。

### 1.2 背景与动机
安全带是汽车最重要的被动安全装置。安全带未系报警是法规要求（GB 4094）。ISO 26262 ASIL B 安全相关。

### 1.3 相关需求
- REQ-SIG-010: driver_occupied
- REQ-SIG-011: passenger_occupied
- REQ-SIG-012: driver_buckled
- REQ-SIG-013: passenger_buckled
- REQ-SIG-014: rear_buckle

---

## 2. 功能需求

### 2.1 监控位置
| 位置 | 占用信号 | 安全带信号 | 图标 |
|------|---------|---------|------|
| 主驾 | driver_occupied (0x2F0) | driver_buckled (0x3B0) | seat_driver_*.png |
| 副驾 | passenger_occupied (0x2F1) | passenger_buckled (0x3B1) | seat_passenger_*.png |
| 后左 | - (默认有人) | rear_buckle bit0 (0x3B2) | seat_rear_left_*.png |
| 后中 | - (默认有人) | rear_buckle bit1 (0x3B2) | seat_rear_center_*.png |
| 后右 | - (默认有人) | rear_buckle bit2 (0x3B2) | seat_rear_right_*.png |

### 2.2 报警触发条件
| 条件 | 值 |
|------|-----|
| 速度阈值 | vehicle_speed > 5 km/h |
| 必须座位有人 | require_seat_occupied: true |
| 安全带未系 | buckle == 0 |

### 2.3 报警消息
| 语言 | 消息模板 |
|------|---------|
| zh_CN (单人) | "{position}请系安全带" |
| zh_CN (多人) | "{positions}请系安全带" |
| en_US (单人) | "{position} please buckle up" |
| en_US (多人) | "{positions} please buckle up" |

### 2.4 指示灯状态
| 状态 | 图标 |
|------|------|
| 有人+已系 | icon_buckled (绿色/已系图标) |
| 有人+未系 | icon_unbuckled (红色/未系图标) + 2Hz闪烁 |
| 无人 | icon_empty (灰色/空座图标) |

### 2.5 防抖逻辑
- 占用状态变化: 500ms 防抖（防止误判）
- 安全带扣紧变化: 200ms 防抖

---

## 3. 非功能需求

### 3.1 性能要求
- 响应延迟: ≤ 100ms（信号变化到图标更新）

### 3.2 安全性需求
- ISO 26262 ASIL B
- 安全带报警不得被驾驶员手动关闭

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 速度阈值 | seat_belt.yaml | trigger.speed_threshold | 5.0 km/h |
| 必须有人 | seat_belt.yaml | trigger.require_seat_occupied | true |
| 闪烁频率 | seat_belt.yaml | flash.frequency_hz | 2 Hz |
| 占用防抖 | seat_belt.yaml | debounce.occupied_ms | 500 ms |
| 安全带防抖 | seat_belt.yaml | debounce.buckle_ms | 200 ms |

---

## 5. 测试用例

| 用例ID | 场景 | 输入 | 预期输出 | 状态 |
|--------|------|------|---------|------|
| TC-SYS-004-01 | 主驾未系安全带行驶 | speed=20, unbuckled | 报警横幅+图标闪烁 | Approved |
| TC-SYS-004-02 | 系安全带后清除 | speed=20, buckled | 报警清除+图标变绿 | Approved |
| TC-SYS-004-03 | 停车时未系 | speed=0, unbuckled | 无报警 | Approved |

---

## 6. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/seat_belt.yaml` (driver L4, passenger L15, rear_left L26, rear_center L36, rear_right L46, trigger.speed_threshold L57) |
| 生成代码 | `src/generated/seat_belt_def.h` (SEAT_BELT_POSITION_TABLE) / `src/generated/seat_belt_table.cpp` |
| 关联 L2 组件 | `src/layer2/seat_belt_runtime.cpp` (PR 23 L2+test, 280 行测试) |
| QML组件 | `src/ui/SeatBeltZone.qml` (5 座位图标 + 报警横幅联动) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 test_seat_belt_runtime 280 行 / 多 case, PR 23 stub 升级) |

---

## 7. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented, 元数据头部 + §6 实现追踪批量同步: 实现版本填 seat_belt.yaml 行号 + L2 组件 (PR 23) + 验证日期/结果 (PR 34) | can-dash-jd-autopilot |
