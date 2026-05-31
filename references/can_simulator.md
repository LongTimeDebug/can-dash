# CAN 仿真引擎与 Socket 通信架构

## 架构概览

```
engine.py (Python)                    DashboardBackend (C++/Qt)
     |                                        |
     |- BMS 帧 0x186040F3 (100ms)            |
     |- VCPU 帧 0x203 (50ms)                 |
     |- MCU 帧 0x101 (100ms)                 |
     |- 座椅帧 0x2F0/0x2F1 (500ms)           |
     |- 安全带帧 0x3B0/0x3B1/0x3B2 (200ms)  |
                                              |
         Unix Socket (/tmp/can_dash_socket)  <|
              TCP stream
```

## 启动方式（双终端）

```bash
# 终端1：启动仪表盘
cd /home/cjl/can-dash/build
export QT_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/qt6/plugins
export QML2_IMPORT_PATH=/usr/lib/x86_64-linux-gnu/qt6/qml
export QT_QUICK_BACKEND=software
export QT_OPENGL=software
./can-dash

# 终端2：启动 CAN 仿真
cd /home/cjl/can-dash
python3 -u can_sim/engine.py
```

## 验证连接

```bash
ss --unix | grep can_dash
# ESTAB = 连接正常
```

## 帧数据格式

### BMS 帧（0x186040F3，100ms）
- bat_volt: uint16, 原始值 = volt * 10
- bat_curr: uint16, 原始值 = curr * 10 + 10000
- bat_soc: int8, 0-100%

### VCPU 帧（0x203，50ms）
- byte[0]: brake (uint8)
- byte[1]: reserved
- byte[2-3]: speed_hi, speed_lo (uint16, little-endian)
- 实际帧长 5 字节

### MCU 帧（0x101，100ms）
- motor_rpm: int16
- reserved: int16
- motor_temp: uint8, 原始值 = temp + 40

## engine.py 关键实现

```python
def send_frame(sock, can_id, data):
    # 无 padding 的变长帧
    msg = struct.pack("<IB", can_id, len(data)) + data
    sock.send(msg)
```

**关键**：永远不要用 `<IB8s` padding 到 8 字节，这会导致 DLC=8 而 C++ 侧按 8 字节解析。

## Socket 状态机

见 `can_socket_protocol.md`。
