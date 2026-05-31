# CAN-Dash 需求索引

最后更新: 2026-05-31

## 统计

| 类别 | 总数 | Approved | Implemented | Verified |
|------|------|----------|-------------|----------|
| ALM (报警) | 11 | 11 | 0 | 0 |
| HYBRID (混动特有) | 6 | 1 | 0 | 0 |
| IND (指示灯) | 12 | 12 | 0 | 0 |
| SIG (CAN信号) | 19 | 19 | 0 | 0 |
| UI (界面) | 5 | 5 | 0 | 0 |
| SYS (系统) | 4 | 4 | 0 | 0 |
| **合计** | **57** | **52** | **0** | **0** |

---

## 需求列表

### ALM (报警) — 11项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-ALM-001 | 电池过压报警 | Safety | High | Approved | - |
| REQ-ALM-002 | 电池欠压报警 | Safety | High | Approved | - |
| REQ-ALM-003 | 电量低报警 (SOC<10%) | Safety | Medium | Approved | - |
| REQ-ALM-004 | 电机温度过高报警 | Safety | High | Approved | - |
| REQ-ALM-005 | 胎压低报警 | Safety, Functional | Critical | Proposed | - |
| REQ-ALM-006 | 充电中禁止行驶报警 | Safety | High | Approved | - |
| REQ-ALM-007 | 电机超速报警 | Safety | High | Approved | - |
| REQ-ALM-008 | 电池高温报警 | Safety | High | Approved | - |
| REQ-ALM-009 | 充电超时报警 | Functional | Medium | Approved | - |
| REQ-ALM-010 | 充电设备故障报警 | Safety | High | Approved | - |
| REQ-ALM-011 | 充电桩通信超时报警 | Functional | Medium | Approved | - |

### HYBRID (混动特有) — 6项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-HYBRID-001 | 混动汽车仪表盘特有功能需求基线 | Functional, Safety | High | Proposed | - |
| REQ-HYBRID-002 | 充电状态显示 | Functional | Medium | Approved | - |
| REQ-HYBRID-003 | 能量流动图 | Functional | Medium | Approved | - |
| REQ-HYBRID-004 | 续航里程预测 | Functional | High | Approved | - |
| REQ-HYBRID-005 | 驾驶模式切换 | Functional | High | Approved | - |
| REQ-HYBRID-006 | 充电功率显示 | Functional | Medium | Approved | - |

### IND (指示灯) — 12项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-IND-001 | 电池警告指示灯 | Functional | High | Approved | - |
| REQ-IND-002 | 电量低指示灯 | Functional | Medium | Approved | - |
| REQ-IND-003 | 电机温度警告指示灯 | Functional | High | Approved | - |
| REQ-IND-004 | Ready/Go 指示灯 | Functional | Medium | Approved | - |
| REQ-IND-005 | 高压指示灯 | Functional | Medium | Approved | - |
| REQ-IND-006 | 高压警示灯 | Functional | High | Approved | - |
| REQ-IND-007 | 驾驶模式指示灯 | Functional | Medium | Approved | - |
| REQ-IND-008 | 充电指示灯 | Functional | Medium | Approved | - |
| REQ-IND-009 | 系统故障指示灯 | Safety | Critical | Approved | - |
| REQ-IND-010 | 电池警告灯 | Safety | High | Approved | - |
| REQ-IND-011 | 充电进行中指示灯 | Functional | Medium | Approved | - |
| REQ-IND-012 | 充电故障指示灯 | Safety | High | Approved | - |

### SIG (CAN信号) — 19项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-SIG-001 | 电池电压信号 (bat_volt) | Functional | Critical | Approved | - |
| REQ-SIG-002 | 电池SOC信号 (bat_soc) | Functional | Critical | Approved | - |
| REQ-SIG-003 | 车速信号 (vehicle_speed) | Functional | Critical | Approved | - |
| REQ-SIG-004 | 制动信号 (brake) | Functional | High | Approved | - |
| REQ-SIG-005 | 电机转速信号 (motor_rpm) | Functional | High | Approved | - |
| REQ-SIG-006 | 电机温度信号 (motor_temp) | Functional | High | Approved | - |
| REQ-SIG-007 | 电池温度信号 (battery_temp) | Functional, Safety | High | Approved | - |
| REQ-SIG-008 | 胎压信号 (tire_pressure) | Functional | High | Proposed | - |
| REQ-SIG-009 | 驾驶员座椅占用信号 | Functional | Medium | Approved | - |
| REQ-SIG-010 | 副驾驶员座椅占用信号 | Functional | Medium | Approved | - |
| REQ-SIG-011 | 驾驶员安全带状态信号 | Safety | High | Approved | - |
| REQ-SIG-012 | 副驾驶员安全带状态信号 | Safety | High | Approved | - |
| REQ-SIG-013 | 驾驶员座椅温度信号 | Functional | Low | Approved | - |
| REQ-SIG-014 | 副驾驶员座椅温度信号 | Functional | Low | Approved | - |
| REQ-SIG-015 | 充电指示灯信号 | Functional | Medium | Approved | - |
| REQ-SIG-016 | 充电功率信号 | Functional | Medium | Approved | - |
| REQ-SIG-017 | 剩余充电时间信号 | Functional | Low | Approved | - |
| REQ-SIG-018 | 能量模式信号 | Functional | Medium | Approved | - |
| REQ-SIG-019 | 电池电流信号 (bat_curr) | Functional | High | Approved | - |

### UI (界面) — 5项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-UI-001 | 多语言切换 (zh_CN / en_US) | UI | Medium | Approved | - |
| REQ-UI-002 | 报警横幅 (AlarmBanner) | UI | High | Approved | - |
| REQ-UI-003 | 仪表表盘 (GaugeCanvas) | UI | Critical | Approved | - |
| REQ-UI-004 | 界面布局规格 | UI | High | Approved | - |
| REQ-UI-005 | 颜色主题需求 | UI | Medium | Approved | - |

### SYS (系统) — 4项

| ID | 标题 | 类型 | 优先级 | 状态 | 实现版本 |
|----|------|------|--------|------|---------|
| REQ-SYS-001 | CAN总线超时检测 | Reliability | High | Approved | - |
| REQ-SYS-002 | CAN信号平滑与范围检查 | Reliability | High | Approved | - |
| REQ-SYS-003 | LCD背光超时逻辑 | Functional | Low | Approved | - |
| REQ-SYS-004 | 安全带状态运行时监控 | Safety | High | Approved | - |

---

## 待规划需求

| ID | 标题 | 来源 | 优先级 | 状态 |
|----|------|------|--------|------|
| REQ-SYS-005 | 仪表黑屏/白屏自检 | 用户需求 | High | Proposed |

---

## 需求文档

每个需求的详细规格文档位于本目录 (`*.md`)。

---

## 生命周期状态

```
Proposed → Approved → Implemented → Verified
    ↓           ↓            ↓
 Rejected   Rejected     Rejected
```

## 更新规则

1. 新增需求 → 分配 ID → 写 REQ-XXX.md → 更新本 INDEX.md
2. 需求实现 → 更新 `实现版本` 列 → 更新 `状态` 为 Implemented
3. 需求验收 → 更新 `状态` 为 Verified + 验证日期
4. 需求拒绝 → 更新 `状态` 为 Rejected
