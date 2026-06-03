#REQ-ALM-007|电机超速报警 (Motor Overspeed)
=========================================

**状态**:   Implemented
**类型**:   Safety
**优先级**: High
**来源**:   alarm_rules.yaml (motor_overspeed)
**创建日期**: 2026-05-31
**实现版本**: PR 20 (2026-06-04)

---

## 1. 概述

### 1.1 需求描述
当 motor_rpm > 8000 RPM 持续 500ms 时，触发电机超速报警横幅，motor_warn_light 闪烁（4Hz）。

### 1.2 背景与动机
电机超速可能导致控制器过流、轴承损坏或减速齿轮异常磨损，是混动车型的关键安全报警。驾驶员需要立即看到警告以减速。

### 1.3 相关需求
- REQ-SIG-005: 电机转速信号 (motor_rpm)
- REQ-ALM-004: 电机温度过高报警 (motor_overtemp)
- REQ-IND-003: 电机温度警告指示灯 (motor_warn_light)

---

## 2. 功能需求

### 2.1 触发条件
- motor_rpm > 8000 RPM (超过安全阈值)
- 持续时间 ≥ 500ms（防抖，避免瞬时尖峰误报）

### 2.2 输入
| 字段 | 来源 | 格式 | 说明 |
|------|------|------|------|
| motor_rpm | CAN总线 (MCU 消息) | uint16 | 0-12000 RPM，实际工作范围 0-8000 |

### 2.3 输出
| 字段 | 目标 | 格式 |
|------|------|------|
| motor_warn_light | 指示灯闪烁 | widget id: motor_warn_light, flash 4Hz |
| alarm_text | 报警横幅 | text_zh: "电机超速！请减速！" / text_en: "MOTOR OVERSPEED", 字号 32, 颜色 #FF4400 |

### 2.4 处理逻辑
```
[motor_rpm > 8000] → 持续 500ms → 报警触发
    ├── motor_warn_light ON（闪烁 4Hz）
    └── AlarmBanner 显示 "电机超速！请减速！"

[motor_rpm ≤ 8000] → 立即清除
    ├── motor_warn_light OFF
    └── AlarmBanner 隐藏
```

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 条件 | alarm_rules.yaml | condition | value > 8000 |
| 优先级 | alarm_rules.yaml | priority | high |
| 防抖 | alarm_rules.yaml | duration_ms | 500 |
| 指示灯 | alarm_rules.yaml | widget | motor_warn_light |
| 闪烁频率 | alarm_rules.yaml | flash_hz | 4 |

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/alarm_rules.yaml` (`motor_overspeed` 规则) |
| 生成文件 | `src/generated/alarm_rule_table.cpp` (ALARM_RULE_TABLE 索引 17) |
| 关联 L2 组件 | `src/layer2/alarm_runtime.cpp` (`onValueChanged("motor_rpm", ...)`) |
| QML组件 | `src/ui/AlarmBanner.qml` (通过 alarmList / alarmMessageZh 触发) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 alarm_rule_table 18 条规则) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 (Hybrid Mode 主题, 文档错位) | requirements-document-agent |
| 2026-06-04 | 2.0 | 改写为电机超速报警 (匹配 INDEX.md 标题, 新增 motor_overspeed 规则) | can-dash-jd-autopilot |
