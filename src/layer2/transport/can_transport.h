// can_transport.h
// CAN 总线传输层抽象接口（车规：硬件替换不影响业务逻辑）
//
// 目的：把 "CAN 帧从哪来" 抽象掉，业务层（AlarmRuntime/IndicatorRuntime）
//       只关心 "下一个帧是什么"，不关心是 socket/socketcan/Kvaser。
//
// 当前实现：
//   - SimSocketTransport：Unix 域 socket，python-can engine.py 仿真
//   - SocketCanTransport：Linux 内核 SocketCAN（can0/can1/...）
// 未来可扩展：
//   - KvaserTransport：libkvrlib
//   - PeakTransport：libpcan
//   - VectorTransport：XL-API
#pragma once

#include <cstdint>
#include <cstddef>

namespace candash {
namespace transport {

// CAN 数据链路层最大数据长度（CAN 2.0 标准帧）
constexpr size_t kCanMaxDlc = 8;

// 传输层接口（纯虚）
// 设计原则：
//   - 无动态内存分配（车规 MISRA-C:2012 Rule 21.3）
//   - 同步阻塞 readFrame + timeout（简单，无回调地狱）
//   - 错误通过返回值传递，不抛异常
class ICanTransport {
public:
    virtual ~ICanTransport() = default;

    // 打开传输通道。返回 true 表示成功，false 表示失败（errno 保留）。
    virtual bool open() = 0;

    // 关闭传输通道。安全多次调用。
    virtual void close() = 0;

    // 阻塞读取一帧 CAN。
    //   can_id  : 帧 ID（不含 EFF/ERR/RTR flag，由实现负责 mask）
    //   dlc     : 数据长度（0..8）
    //   data    : 数据缓冲区，至少 8 字节
    //   data_size: data 容量（>=8 才是合法 CAN 2.0 帧）
    //   timeout_ms : 超时时间，-1 永久阻塞，0 非阻塞，>0 阻塞 N 毫秒
    // 返回：
    //   true  — 成功读到一帧
    //   false — 超时（timeout_ms 内无帧）/ 远端关闭 / 错误
    virtual bool readFrame(uint32_t& can_id, uint8_t& dlc,
                           uint8_t* data, size_t data_size,
                           int timeout_ms) = 0;

    // 是否处于 "打开" 状态（已 open 且无致命错误）
    virtual bool isOpen() const = 0;

    // 是否处于 "就绪" 状态（数据源已就绪，可写入共享内存）
    //   - sim-socket : 客户端已连接
    //   - socketcan  : 打开即就绪（设备总是 ready）
    virtual bool isReady() const = 0;

    // 用于日志的可读名称（如 "sim-socket", "socketcan:can0"）
    virtual const char* name() const = 0;
};

}  // namespace transport
}  // namespace candash
