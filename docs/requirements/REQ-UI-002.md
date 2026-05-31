#REQ-UI-002|报警横幅 (AlarmBanner)
=========================================

**状态**:   Approved
**类型**:   UI
**优先级**: High
**来源**:   alarm_rules.yaml actions
**创建日期**: 2026-05-31
**实现版本**: -

---

## 1. 概述

### 1.1 需求描述
报警横幅组件（AlarmBanner）在触发报警规则时从屏幕顶部滑入显示报警文本，颜色和字号由 alarm_rules.yaml 配置。

### 1.2 背景与动机
报警横幅是向驾驶员传递安全信息的核心UI组件，必须在各种光照条件下清晰可见。

### 1.3 相关需求
- 所有 ALM 需求（报警横幅由 alarm_rules 触发）
- REQ-UI-001: i18n (报警文本多语言)

---

## 2. 功能需求

### 2.1 组件规格
| 字段 | 值 |
|------|-----|
| 组件名称 | AlarmBanner |
| 类型 | QML Rectangle + Text |
| 位置 | 屏幕顶部居中 |
| 层级 | 最高 z-order |
| 背景色 | alarm_rules.yaml 中定义 (如 #FF4400, #FF0000) |
| 字体 | 粗体，字号由 alarm_rules.yaml 定义 |

### 2.2 动画行为
| 行为 | 参数 |
|------|------|
| 进入动画 | 从顶部滑入，200ms ease-out |
| 显示时长 | 3秒（可配置） |
| 退出动画 | 淡出，300ms |
| 最大同显数量 | 3条（超出按优先级排列） |

### 2.3 多语言支持
- 报警文本从 i18n 文件读取
- 优先级: alarm_rules 中的 text_zh / text_en > i18n fallback

---

## 3. 非功能需求

### 3.1 性能要求
- 进入延迟: <= 100ms（从报警触发到显示）
- 刷新率: 60 FPS（动画）

### 3.2 安全性需求
- 最高UI优先级（不可被其他UI遮挡）
- 颜色对比度满足 ISO 15005 要求

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 说明 |
|------|---------|------|------|
| 中文文本 | alarm_rules.yaml | actions.alarm_text.text_zh | 报警中文文本 |
| 英文文本 | alarm_rules.yaml | actions.alarm_text.text_en | 报警英文文本 |
| 字号 | alarm_rules.yaml | actions.alarm_text.font_size | 如 32 |
| 颜色 | alarm_rules.yaml | actions.alarm_text.color | 如 #FF4400 |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| QML组件 | `src/ui/AlarmBanner.qml` |
| 样式 | `src/ui/AlarmBannerStyle.qml` |
| 验证日期 | - |
| 验证结果 | - |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
