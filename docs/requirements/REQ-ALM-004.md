#REQ-ALM-004|电机温度过高报警
=========================================

**状态**:   Implemented
**类型**:   Safety
**优先级**: High
**来源**:   alarm_rules.yaml (motor_overtemp)
**创建日期**: 2026-05-31
**实现版本**: alarm_rules.yaml:motor_overtemp (L69)

---

## 1. 概述

### 1.1 需求描述
当电机温度超过 120°C 时，触发报警横幅并使电机警告指示灯闪烁（4Hz）。

### 1.2 背景与动机
电机温度过高可能导致电机退磁或损坏，需要及时提醒驾驶员。

### 1.3 相关需求
- REQ-SIG-007: motor_temp 信号

---

## 2. 功能需求

### 2.1 触发条件
- motor_temp > 120
- 持续时间 ≥ 500ms（防抖）

### 2.2 输入
| 字段 | 来源 | 格式 | 正常范围 |
|------|------|------|---------|
| motor_temp | CAN总线 (0x101, byte 4) | int8 °C | -40~120°C |

### 2.3 输出
| 字段 | 目标 | 格式 |
|------|------|------|
| alarm_text | QML 报警横幅 | QString: "电机温度异常！" |
| alarm_text_en | QML 报警横幅 | QString: "MOTOR OVERTEMP" |
| indicator | motor_warn_light | widget id: motor_warn_light |
| flash_hz | 闪烁频率 | 4 Hz |

### 2.4 处理逻辑
```
[motor_temp > 120] → 持续 500ms → 触发 motor_overtemp
    ├── motor_warn_light 闪烁（4Hz）
    └── 报警横幅显示
```

### 2.5 边界条件与异常处理
- motor_temp 信号超时 → 显示 "---"
- 温度恢复正常（≤115°C）→ 500ms 防抖后清除

---

## 3. 配置参数

| 参数 | YAML文件 | 字段 | 值 |
|------|---------|------|-----|
| 条件 | alarm_rules.yaml | condition | value > 120 |
| 优先级 | alarm_rules.yaml | priority | high |
| 防抖 | alarm_rules.yaml | duration_ms | 500 |
| 闪烁频率 | alarm_rules.yaml | flash_hz | 4 |
| 指示灯 | alarm_rules.yaml | widget | motor_warn_light |

---

## 4. 测试用例

| 用例ID | 场景 | 输入 | 预期输出 | 状态 |
|--------|------|------|---------|------|
| TC-ALM-004-01 | 触发高温报警 | motor_temp=125°C 持续 550ms | 报警横幅+4Hz闪烁 | Approved |
| TC-ALM-004-02 | 自动清除 | motor_temp恢复到110°C | 500ms后清除 | Approved |

---

## 5. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `config/alarm_rules.yaml` (motor_overtemp 规则, L69) |
| 生成代码 | `src/generated/alarm_rule_table.cpp` (ALARM_RULE_TABLE 索引 4) |
| 关联 L2 组件 | `src/layer2/alarm_runtime.cpp` (`onValueChanged("motor_temp", 110)` 触发) |
| QML组件 | `src/ui/AlarmBanner.qml` (红色横幅) / `src/ui/IndicatorLight.qml` (motor_overtemp_light) |
| 验证日期 | 2026-06-04 |
| 验证结果 | 18/18 ctest pass (含 motor_overtemp 规则, PR 28 批量同步元数据) |

---

## 6. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建（从 alarm_rules.yaml 补充） | requirements-document-agent |
| 2026-06-04 | 1.1 | 元数据头部 + §5 实现追踪批量同步: 状态 Approved → Implemented, 实现版本 + alarm_rules.yaml:motor_overtemp (L69), 验证日期/结果填充 (PR 28) | can-dash-jd-autopilot |
