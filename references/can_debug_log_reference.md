# CAN 调试日志分析与 QML 绑定故障排查

## 调试日志输出位置

| 输出源 | 目的地 | 说明 |
|--------|--------|------|
| C++ qDebug() | /tmp/can_dash_debug.log | 通过 `2>&1 | tee` |
| QML console.log() | 终端 stdout | 不进日志文件 |

## 常见日志格式

```
[SocketServer] Client connected
QML: spd=50.0 rpm=2500 v=350.2 soc=75
CAN: speed=50 bat_volt=350.2 bat_curr=-50.0 soc=75
```

## 故障排查流程

### 1. C++ 层数据正常？

```
grep "CAN:" /tmp/can_dash_debug.log
```

- 有数据 -> C++ 数据层正常，跳到步骤 3
- 无数据 -> 检查 Unix Socket 连接

### 2. Unix Socket 连接正常？

```bash
ss --unix | grep can_dash
# ESTAB = 正常
# 无输出 = engine.py 未连接
```

### 3. QML Connections 收到信号？

在 DashboardMain.qml 的 Connections 中加 console.log：

```qml
Connections {
    target: dashboard
    function onDisplayDataChanged() {
        console.log("QML got displayDataChanged! speed=" + dashboard.displayData["vehicle_speed"])
    }
}
```

### 4. QML 界面不更新？

**根因**：QML binding 对 `QVariantMap["key"]` 惰性求值。

**解法**：Connections 直接赋值给元素 ID：

```qml
Connections {
    target: dashboard
    function onDisplayDataChanged() {
        speedGauge.value = dashboard.displayData["vehicle_speed"] || 0
    }
}
```

## display_key 三处必须一致

1. `can_ids.yaml` -> field.name
2. `dashboard_backend_qt.cpp` -> `strcmp(def->display_key, "???")`
3. `DashboardMain.qml` -> `dd["???"]`

不一致时 QML 收到 undefined，静默失败。

## 日志分析示例

### VCPU 帧 byte_start 边界 bug

```
MCU_FRAME: motor_rpm=2900      <- MCU 帧解析正常
VCPU_FRAME: soc=0              <- VCPU 帧 soc=0（byte_start 边界检查缺失）
```

### can_converter 结果丢弃

```
CAN: bat_volt=350.2 bat_curr=-50.0 bat_soc=75  <- C++ 日志有值
QML: v=0.0 soc=0                                     <- QML 无变化
```

根因：`processFrame()` 中 `(void)value;` 导致 DisplayData 永远为零。

## 常用调试命令

```bash
# 查看 CAN 帧数据（每20帧）
grep "CAN:" /tmp/can_dash_debug.log | tail -5

# 确认 engine.py 连接
ss --unix | grep can_dash

# 确认 can-dash 进程存活
ps aux | grep can-dash

# QML 文件同步状态
ls -la build/qml/DashboardMain.qml src/ui/DashboardMain.qml
```
