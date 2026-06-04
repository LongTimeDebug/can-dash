#REQ-UI-004|仪表盘布局配置 (Display Layout)
=========================================

**状态**:   Implemented
**类型**:   UI
**优先级**: Critical
**来源**:   config/display_layout.yaml
**创建日期**: 2026-05-31
**实现版本**: config/display_layout.yaml (L1-74) + src/ui/DashboardMain.qml

---

## 1. 概述

### 1.1 需求描述
仪表盘显示区域布局配置，定义所有UI元素的位置、尺寸、图层顺序和绑定信号。

### 1.2 背景与动机
布局配置是仪表盘UI开发的基础，决定了各显示元素的空间关系。

### 1.3 相关需求
- REQ-UI-003: GaugeCanvas (仪表表盘组件)
- REQ-UI-002: AlarmBanner (报警横幅)
- REQ-UI-001: 多语言切换

---

## 2. 功能需求

### 2.1 显示区域规格
| 字段 | 值 |
|------|-----|
| 分辨率 | 800 x 480 |
| 主题 | dark |
| 背景色 | #0A0A0A |
| 默认语言 | zh |

### 2.2 主页面 (main) 图层顺序
| 层级 | ID | 类型 | 描述 |
|------|-----|------|------|
| 0 | background | image | 仪表盘背景图 dashboard_bg.png |
| 1 | speed_gauge | gauge | 车速表 (x=240, y=40, 320x320) |
| 2 | bat_volt_text | text | 电池电压 (x=600, y=60) |
| 3 | soc_bar | bar | 电量条 (x=600, y=100, 40x180) |
| 4 | bat_warn | light | 电池警告灯 (x=600, y=260) |
| 5 | engine_warn | light | 发动机警告灯 (x=540, y=260) |
| 6 | seat_belt_zone | custom | 安全带区域 (x=200, y=380, 400x80) |

### 2.3 车速表配置
| 字段 | 值 |
|------|-----|
| 绑定信号 | vehicle_speed |
| 最小值 | 0 km/h |
| 最大值 | 260 km/h |
| 主刻度 | 13 个 |
| 单位 | km/h |

### 2.4 电量条配置
| 字段 | 值 |
|------|-----|
| 绑定信号 | bat_soc |
| 最小值 | 0% |
| 最大值 | 100% |
| 方向 | bottom_up (从下往上) |
| 颜色 | #00FF88 |

---

## 3. 非功能需求

### 3.1 性能要求
- 刷新率: ≥ 30 FPS
- 信号到UI延迟: ≤ 100ms

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 分辨率 | display_layout.yaml | resolution | 800x480 |
| 主题 | display_layout.yaml | theme | dark |
| 车速绑定 | display_layout.yaml | bindings.value | vehicle_speed |
| SOC绑定 | display_layout.yaml | bindings.value | bat_soc |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/display_layout.yaml` |
| QML组件 | `src/ui/DashboardPage.qml` |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 36 同步) |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented (PR 36 同步元数据) | requirements-document-agent |
