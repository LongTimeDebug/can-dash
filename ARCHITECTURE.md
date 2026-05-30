# CAN-Dash 架构文档

## 设计原则

1. **YAML 即业务**：所有可配置的业务逻辑在 YAML 中，修改 YAML 等价于修改业务
2. **Layer 严格单向依赖**：Layer N 只依赖 Layer N-1，不能反向依赖
3. **Layer 2 无 Qt**：业务逻辑层可独立编译测试，不需要 Qt 环境
4. **EventBus 统一通信**：所有 Runtime 通过 EventBus 通信，不直接调用

## 架构分层

```
┌──────────────────────────────────────────────────────────────┐
│ Layer 4: ui/                QML 纯展示                        │
│             display_layout.yaml 驱动生成，无硬编码            │
├──────────────────────────────────────────────────────────────┤
│ Layer 3: layer3/           Qt 适配层                         │
│             QTimer、Q_PROPERTY、QML 绑定                     │
│             EventBus → Qt 信号 → QML                         │
├──────────────────────────────────────────────────────────────┤
│ Layer 2: layer2/           纯 C++ 业务逻辑                   │
│             无 Qt、无 YAML 运行时、无动态内存（static buffer）  │
│             EventBus 订阅/发布                               │
├──────────────────────────────────────────────────────────────┤
│ Layer 1: generated/         YAML → C 代码（yaml_to_c.py）    │
│             纯 C struct + const 查找表                       │
└──────────────────────────────────────────────────────────────┘
```

## 数据流

```
CAN 帧
  │
  ▼ Layer3: CanReceiver (Unix Socket / QCanBus)
原始字节
  │
  ▼ Layer2: CanConverter (查找 CAN_FIELD_TABLE)
DisplayData (技术值)
  │
  ├─→ Layer2: CanSignalMonitor  →  SignalQuality
  │       (超时/范围/突变检测)
  │
  ├─→ Layer2: VehicleLogic
  │       (业务逻辑：超时显示"---"、SOC平滑、驾驶模式)
  │
  ├─→ Layer2: AlarmRuntime
  │       (遍历 ALARM_RULE_TABLE，条件成立→触发动作)
  │
  ├─→ Layer2: SeatBeltRuntime
  │       (行驶状态机：静止→行驶切换时重新评估所有座位)
  │
  └─→ Layer2: IndicatorRuntime
          (指示灯状态)
          │
          ▼ Layer3: DashboardBackend (聚合 Q_PROPERTY)
          ▼ Layer4: QML UI
```

## YAML 配置体系

| 文件 | 描述 | 示例字段 |
|------|------|---------|
| `can_ids.yaml` | CAN ID → 显示变量 + formula | `byte`, `bits`, `formula: "x/10.0"` |
| `alarm_rules.yaml` | 报警规则 | `condition: "value > 420"`, `actions` |
| `seat_belt.yaml` | 安全带座位布局 | `positions`, `trigger.speed_threshold` |
| `indicators.yaml` | 指示灯定义 | `image_on`, `image_off` |
| `can_signal_status.yaml` | 信号健康监控 | `timeout_ms`, `validity.max_delta` |
| `display_layout.yaml` | 界面布局 | `position`, `size`, `bindings` |

## 关键约束

- Layer 2 的 Runtime 不能直接 include `<QtCore>` 或任何 Qt 头文件
- Layer 2 所有内存分配在栈上或通过预分配的 static buffer（无 new/malloc）
- YAML 配置校验失败时，`yaml_to_c.py` 输出 AI 友好的错误信息
- 所有生成的 C 代码包含 YAML 源文件的注释
- `src/generated/` 由 `yaml_to_c.py` 自动生成，禁止手动修改

## 扩展指南

### 新增一个 Runtime

1. 在 `src/layer2/` 新建 `xxx_runtime.cpp`，继承 `Runtime` 基类
2. 实现 `name()`、`init()`、`onEvent()`、`tick()`
3. 在 `src/generated/` 下新建 `xxx_def.h`（`yaml_to_c.py` 生成元数据）
4. 运行 `python tools/yaml_to_c.py`
5. `make test` 验证

### 新增一个 QML 组件类型

1. 在 `src/ui/` 下新建 `XxxControl.qml`
2. 在 `tools/qml_generator.py` 添加生成规则
3. 修改 `display_layout.yaml` 使用新组件类型

### 新增报警类型

1. `config/alarm_rules.yaml` 添加规则（定义 condition 和 actions）
2. `config/indicators.yaml` 添加图标定义（如需新图标）
3. `python tools/validate.py && python tools/yaml_to_c.py && make`

## EventBus

```cpp
// 发布事件
EventBus::instance().publish({"can.bat_volt", value, prev, nowMs, this});

// 订阅 key
EventBus::instance().subscribe_key("can.bat_volt", [](const Event& e) {
    // 处理
});

// 通配符订阅
EventBus::instance().subscribe_key("*", [](const Event& e) {
    // 监听所有变量变化
});
```

## 禁止事项

1. **不要手动修改 `src/generated/` 下的任何文件**
2. **不要在 Layer 2 代码中 include 任何 Qt 头文件**
3. **不要在 Layer 2 中 new 动态内存**
4. **不要跳过 `validate.py` 直接运行 `yaml_to_c.py`**
