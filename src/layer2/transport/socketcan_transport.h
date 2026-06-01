// socketcan_transport.h
// Linux SocketCAN 真实硬件传输（生产环境用）
//
// 适用于：
//   - 内核原生 SocketCAN（CONFIG_CAN=y）
//   - 常见适配器：CANable、PEAK PCAN-USB、KVASER Leaf（带 candriver）
//   - 虚拟 can0（`sudo modprobe vcan && sudo ip link add dev vcan0 type vcan`）
#pragma once

#include "can_transport.h"
#include <cstring>

namespace candash {
namespace transport {

// Linux SocketCAN 传输层
// 编译时要求 Linux 内核（其他平台编译会被 CMake 跳过，见 CMakeLists.txt）
class SocketCanTransport final : public ICanTransport {
public:
    // can_if_name : 接口名（如 "can0", "vcan0", "slcan0"），最长 15 字符
    explicit SocketCanTransport(const char* can_if_name);
    // NVI 模式：析构里调 closeImpl_() 非虚；suppress 跨编译单元推断误报
    // cppcheck-suppress virtualCallInConstructor
    ~SocketCanTransport() override;

    bool open() override;
    void close() override;
    bool readFrame(uint32_t& can_id, uint8_t& dlc,
                   uint8_t* data, size_t data_size,
                   int timeout_ms) override;
    bool isOpen() const override { return sock_fd_ >= 0; }
    bool isReady() const override { return sock_fd_ >= 0; }  // 打开即就绪
    const char* name() const override { return name_; }

private:
    // NVI 模式：析构函数调此（非虚），public close() 也调此
    // 避免 cppcheck / MISRA 警告"析构里调虚函数"
    void closeImpl_();

    char name_[32] = {};  // 例如 "socketcan:can0"
    int  sock_fd_  = -1;
};

}  // namespace transport
}  // namespace candash
