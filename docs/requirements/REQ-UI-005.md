#REQ-UI-005|多语言配置 (i18n)
=========================================

**状态**:   Approved
**类型**:   UI
**优先级**: Medium
**来源**:   config/i18n/*.json
**创建日期**: 2026-05-31
**实现版本**: config/i18n/zh_CN.json + config/i18n/en_US.json + src/ui/I18nProvider.qml

---

## 1. 概述

### 1.1 需求描述
仪表盘支持多语言切换，当前支持简体中文 (zh_CN) 和美国英语 (en_US)。

### 1.2 背景与动机
满足不同地区用户的语言需求，参考比亚迪/特斯拉等车型的多语言支持。

### 1.3 相关需求
- REQ-UI-002: AlarmBanner (报警横幅文本)
- REQ-UI-004: Display Layout

---

## 2. 功能需求

### 2.1 支持语言
| 语言代码 | 文件 | 显示名称 |
|---------|------|---------|
| zh_CN | config/i18n/zh_CN.json | 简体中文 |
| en_US | config/i18n/en_US.json | English |

### 2.2 翻译覆盖范围
| 类别 | 键示例 | 说明 |
|------|--------|------|
| 单位 | unit.speed | "km/h" |
| 状态 | status.driving | "行驶中" |
| 电池 | battery.voltage | "电池电压" |
| 电机 | motor.rpm | "电机转速" |
| 安全带 | seatbelt.driver | "主驾" |
| 指示器 | indicator.left_turn | "左转向" |
| 报警 | alarm_text.bat_overvolt | "电池过压！" |

### 2.3 字体配置
| 语言 | 字体 |
|------|------|
| zh_CN | Noto Sans SC, Microsoft YaHei, sans-serif |
| en_US | system default |

---

## 3. 非功能需求

### 3.1 性能要求
- 语言切换延迟: ≤ 200ms

---

## 4. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 语言配置目录 | config/i18n/ | - | zh_CN.json, en_US.json |
| 默认语言 | display_layout.yaml | language | zh |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/i18n/zh_CN.json`, `config/i18n/en_US.json` |
| QML组件 | `src/ui/I18nProvider.qml` |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 18/18 pass (PR 37 同步, 资源规格已落地) |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | INDEX 标题三角矛盾解决: '颜色主题需求' → '多语言配置 (i18n)' (.md 优先, 跟 SIG-002 决策同形状). 实现版本填 i18n 资源 + I18nProvider.qml, 状态保持 Approved (规格文档性质, 跟 UI-001 Implemented 区分清楚). 验证日期/结果填充 (PR 37) | requirements-document-agent |
