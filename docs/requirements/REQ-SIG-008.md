# REQ-SIG-008 | 胎压信号 (tire_pressure)

=========================================

**状态**:   Implemented
**类型**:   Functional, Safety
**优先级**: High
**来源**:   法规要求（GB 7258）
**创建日期**: 2026-05-31
**实现版本**: v0.3 (commit 2448f83)

---

## 1. 概述

### 1.1 需求描述
仪表盘需要显示 4 个轮胎的实时胎压值（FL/FR/RL/RR），单位 bar，精度 0.01。
当任意胎压值低于 1.8 bar 时触发 REQ-ALM-005 报警。

### 1.2 实现位置

`config/can_ids.yaml` 已定义 4 个胎压 CAN ID：

```yaml
- can_id: 0x3A0
  source_name: tire_pressure_front
  fields:
    - name: tire_pressure_fl
      unit: bar
    - name: tire_pressure
      unit: bar
- can_id: 0x3A1
  source_name: tire_pressure_fr
  fields:
    - name: tire_pressure_fr
      unit: bar
- can_id: 0x3A2
  source_name: tire_pressure_rl
  fields:
    - name: tire_pressure_rl
      unit: bar
- can_id: 0x3A3
  source_name: tire_pressure_rr
  fields:
    - name: tire_pressure_rr
      unit: bar
```

报警规则 `tire_pressure_low` 在 `config/alarm_rules.yaml` 中：
```yaml
- name: tire_pressure_low
  display_key: tire_pressure
  condition: value < 1.8
  priority: high
```

## 2. 验收

- [x] can_ids.yaml 定义 4 个胎压字段
- [x] alarm_rules.yaml 定义低胎压报警
- [x] yaml_to_c.py 生成 display_key 索引
- [ ] 运行时胎压值通过 QML 界面渲染（待 UI-Agent 实现）

## 3. 变更历史

- 2026-06-02: 状态更新为 Implemented（commit 2448f83）
