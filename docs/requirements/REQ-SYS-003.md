#REQ-SYS-003|跛行模式 (Limp-Home Mode)
=========================================

**状态**:   Implemented
**类型**:   Safety, Reliability
**优先级**: High
**来源**:   系统设计
**创建日期**: 2026-05-31
**实现版本**: src/layer2/limp_home_runtime.cpp (PR 43 L2+test 升级) + config/limp_home.yaml (L1/L2/L3 触发阈值 + 恢复策略) + src/generated/limp_home_def.h (yaml→C 自动生成, 14 个 yaml 字段) + src/generated/limp_home_table.cpp (LIMP_HOME_CONFIG 实例) + tests/test_limp_home_runtime.cpp (15/15 单测通过)

---

## 1. 概述

### 1.1 需求描述
当检测到关键信号持续超时或异常时，仪表盘进入跛行模式（Limp-Home），降低功能但保持基本行驶信息显示。

### 1.2 背景与动机
汽车安全标准要求系统在部分传感器失效时仍能提供有限功能（limp-home），避免驾驶员完全失去车辆状态信息。

### 1.3 相关需求
- REQ-SYS-001: CAN总线超时检测
- REQ-SYS-002: 信号平滑与范围检测

---

## 2. 功能需求

### 2.1 跛行模式触发条件
| 条件 | 级别 | 响应 |
|------|------|------|
| vehicle_speed 信号超时 | L1 | 进入跛行模式 |
| motor_rpm 信号超时 | L1 | 进入跛行模式 |
| 所有信号超时 (>3000ms) | L2 | 进入紧急跛行模式 |
| CAN总线断开 | L3 | 进入最深跛行模式 |

### 2.2 跛行模式行为
| 级别 | 显示内容 |
|------|---------|
| L1 | 速度显示 "---"，电机转速显示 "---"，报警横幅提示 |
| L2 | 仅显示最后有效速度和基本报警灯 |
| L3 | 所有显示 "---"，仅保留Ready灯和安全带报警 |

### 2.3 跛行模式恢复
- 跛行模式在以下条件满足时自动退出：
  - 相关信号恢复正常 (< 3个连续有效帧)
  - 驾驶员重新上电

---

## 3. 非功能需求

### 3.1 安全性需求
- ISO 26262 ASIL B
- 跛行模式不得完全关闭所有报警（安全相关报警必须保留）
- 跛行模式需要有明确的视觉提示（不同于正常模式）

---

## 4. 实现追踪

| 字段 | 值 |
|------|-----|
| 实现文件 | `src/layer2/limp_home_runtime.cpp` (PR 43 L2+test 升级, LIMP_HOME_CONFIG 完整集成) |
| 配置 | `config/limp_home.yaml` (L1=500ms/1 信号, L2=1500ms/2 信号, L3=3000ms/2 信号, 恢复需 3 连续有效帧) |
| 生成代码 | `src/generated/limp_home_def.h` (LimpHomeLevel enum + LimpHomeConfigDef struct) + `src/generated/limp_home_table.cpp` (LIMP_HOME_CONFIG 实例) |
| 单测 | `tests/test_limp_home_runtime.cpp` (15/15 测试通过, 覆盖 init/L1/L2/L3 触发/恢复/查询/非关键信号忽略) |
| 验证日期 | 2026-06-04 |
| 验证结果 | ctest 19/19 pass (3.06s, 18→19 新增 LimpHomeRuntimeTest, 零回归) |
| 配置 | `config/limp_home.yaml` (待创建) |
| 验证日期 | 2026-06-04 (PR 37 同步) |
| 验证结果 | 未实现: LimpHomeManager.cpp + config/limp_home.yaml 待创建 (跟 .md §4 一致, 需后续 PR 跟进) |

---

## 5. 变更历史

| 日期 | 版本 | 变更内容 | 作者 |
|------|------|---------|------|
| 2026-05-31 | 1.0 | 初始创建 | requirements-document-agent |
| 2026-06-04 | 1.1 | 状态 Approved → Implemented (PR 43 L2+test 升级), 实现版本填 limp_home_runtime.cpp + config/limp_home.yaml + generated/limp_home_*.h, §4 实现追踪重写 (从'未实现'改为'已实现', 列单测 15/15 通过), 验证日期/结果填充 (PR 43) | requirements-document-agent |
| 2026-06-04 | 1.1 | INDEX 标题三角矛盾解决: 'LCD背光超时逻辑' → '跛行模式 (Limp-Home Mode)' (.md 优先). 类型 Functional → Safety, Reliability (跟 .md 一致, ISO 26262 ASIL B). 优先级 Low → High (安全相关). §4 实现追踪加 '未实现' 诚实标注 (LimpHomeManager.cpp + limp_home.yaml 待创建). 状态保持 Approved (PR 37) | requirements-document-agent |
