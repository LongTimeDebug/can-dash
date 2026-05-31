# CAN Socket 协议与帧解析

## Unix Socket 通信架构

```
can_sim/engine.py  ──TCP/Unix Socket──>  DashboardBackend  ──>  QML UI
       (Python)                              (C++/Qt)
                                           m_rxBuffer 状态机
```

## 帧格式

```
[can_id: uint32_t (4 bytes, little-endian)]
[dlc:    uint8_t  (1 byte)]        ← 实际数据长度 0-8
[data:   N bytes  (dlc 指定)]

总长度 = 5 + dlc 字节
```

示例（车速帧）:
```
can_id = 0x203 (VCPU)
dlc    = 5
data   = [brake(1B), 0, speed_hi(1B), speed_lo(1B)]  ← 实际5字节
```

## Python 发送端

```python
import struct

def send_frame(sock, can_id, data):
    msg = struct.pack("<IB", can_id, len(data)) + data
    sock.send(msg)
```

注意：**不要**用 `<IB8s` padding 到 8 字节，这会导致 DLC=8 而非实际长度。

## C++ 接收端（m_rxBuffer 状态机）

```cpp
void DashboardBackend::onSocketReadyRead() {
    if (!m_socketConnection) return;
    m_rxBuffer.append(m_socketConnection->readAll());  // 累积，不是替换

    while (m_rxBuffer.size() >= 5) {  // 最小帧：4B ID + 1B DLC
        uint32_t canId = *reinterpret_cast<const uint32_t*>(m_rxBuffer.constData());
        uint8_t dlc = static_cast<uint8_t>(m_rxBuffer[4]);
        int frameLen = 5 + dlc;

        if (m_rxBuffer.size() < frameLen) break;  // 数据不足，等待

        QByteArray frameData = m_rxBuffer.mid(5, dlc);
        m_rxBuffer.remove(0, frameLen);  // 移除已消费数据

        handleCanFrameData(frameData, canId);
    }
}
```

## can_converter.cpp 帧解析

`processFrame()` 根据 `can_id` 查找字段定义，然后逐字段从 `frameData` 提取字节。

关键约束：
- `byte_start >= len` → 帧不够长，跳过
- `byte_end >= len` → 帧不够长，跳过
- 使用 `updatedMask` 位掩码记录哪些字段被更新

## 调试技巧

```bash
# 确认连接 ESTAB
ss --unix | grep can_dash

# 打印 CAN 数据（每20帧）
static int frame_count = 0;
if (++frame_count % 20 == 0) {
    qDebug() << "CAN: speed=" << m_displayData.value("vehicle_speed").toFloat();
}
```
