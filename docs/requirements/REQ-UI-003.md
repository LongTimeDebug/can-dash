#REQ-UI-003|仪表表盘组件 (GaugeCanvas)
=========================================

**状态**:   Implemented
**类型**:   UI
**优先级**: Critical
**来源**:   display_layout.yaml
**创建日期**: 2026-05-31
**实现版本**: GaugeCanvas QML 组件 — config/display_layout.yaml:speed_gauge (L15, bindings.value=vehicle_speed, config.max=260, major_ticks=13) + DashboardMain.qml 20ms Timer 推算

---

## 1. 概述

### 1.1 需求描述
仪表表盘组件（GaugeCanvas）是仪表盘的核心显示区域，负责渲染车速表和电机转速表等圆形仪表。

### 1.2 背景与动机
仪表表盘是驾驶员获取行驶信息的主要途径，必须满足清晰、可读、实时的要求。

### 1.3 相关需求
- REQ-SIG-003: vehicle_speed (车速信号)
- REQ-SIG-005: motor_rpm (电机转速)
- REQ-UI-004: Display Layout

---

## 2. 功能需求

### 2.1 组件架构
| 字段 | 值 |
|------|-----|
| 组件名称 | GaugeCanvas |
| 底层技术 | QML Canvas / CustomPainter |
| 父组件 | DashboardPage |
| 子组件 | SpeedGauge, MotorRPMGauge |

### 2.2 车速表配置
| 字段 | 值 |
|------|-----|
| 绑定信号 | vehicle_speed |
| 最小值 | 0 km/h |
| 最大值 | 260 km/h |
| 主刻度数 | 13 个 |
| 主刻度标签 | 0, 20, 40, ... 260 |
| 弧度范围 | 240 度 |
| 起始角度 | 210 度 (左下) |
| 结束角度 | 330 度 (右下) |
| 单位 | km/h |

### 2.3 指针渲染
- 使用 Canvas arc 绘制弧形刻度
- 指针颜色: 绿色(正常) / 黄色(警告) / 红色(超速)
- 超速警告: 红色区域从 220 km/h 开始

### 2.4 数字显示
- 中心显示当前车速数值
- 字体: 粗体大号
- 颜色: 白色(正常) / 红色(超速)

---

## 3. 非功能需求

### 3.1 性能要求
- 刷新率: >= 30 FPS
- 指针更新延迟: <= 50ms
- 弧度计算: 实时，无缓存

### 3.2 可靠性要求
- 信号超时: 显示 "---"，指针归零位
- 异常值: 忽略，保持上一帧

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 车速最小值 | display_layout.yaml | speed_gauge.config.min | 0 |
| 车速最大值 | display_layout.yaml | speed_gauge.config.max | 260 |
| 主刻度数 | display_layout.yaml | speed_gauge.config.major_ticks | 13 |
| 绑定信号 | display_layout.yaml | speed_gauge.bindings.value | vehicle_speed |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/display_layout.yaml` (speed_gauge L15, bindings.value=vehicle_speed L20, config.max=260 L24, major_ticks=13 L25) |
| 父组件 | `src/ui/DashboardMain.qml` (20ms Timer 推算 displaySpeed/displayRpm, L20-33) |
| QML组件 | `src/ui/GaugeCanvas.qml` (仪表容器) |
| 子组件 | `src/ui/SpeedGauge.qml` (vehicle_speed 0-260km/h), `src/ui/MotorRPMGauge.qml` (motor_rpm) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 test_perf_baseline PR 5, 5-benchmark 框架含 shm_to_snapshot < 1.2µs) |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented, 元数据头部 + §5 实现追踪批量同步: 实现版本填 display_layout.yaml speed_gauge 行号 + 父组件 DashboardMain.qml 引用 + 验证日期/结果 (PR 34) | can-dash-jd-autopilot |
