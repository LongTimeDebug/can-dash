// sim_socket_transport.h
// Unix 域 socket 仿真传输（兼容现有 can_sim/engine.py）
#pragma once

#include "can_transport.h"

namespace candash {
namespace transport {

// 通过 Unix 域 socket 接收 python-can engine.py 发送的 CAN 帧
// 协议：[4 字节 can_id LE][1 字节 dlc][N 字节 data]
// 与 can_sim/engine.py 中 _send_frame() 完全兼容。
class SimSocketTransport final : public ICanTransport {
public:
    // 默认构造函数：使用编译时/环境变量配置的 socket 路径
    SimSocketTransport();
    // 测试用：显式指定 socket 路径（避免污染 /tmp/can_processor_socket）
    explicit SimSocketTransport(const char* socket_path);
    ~SimSocketTransport() override;

    bool open() override;
    void close() override;
    bool readFrame(uint32_t& can_id, uint8_t& dlc,
                   uint8_t* data, size_t data_size,
                   int timeout_ms) override;
    bool isOpen() const override { return listen_fd_ >= 0; }
    bool isReady() const override { return client_fd_ >= 0; }
    const char* name() const override { return "sim-socket"; }

private:
    // 接受新客户端连接（设置 client_fd_）
    // 返回 -1 错误，0 成功
    int acceptClient_();

    // 从 client_fd_ 读入数据到 rx_buf_，解析出完整帧
    // 找到完整帧时填充 out_can_id/out_dlc/out_data 并返回 true
    // 否则返回 false（数据不足/超时/错误）
    bool tryParseFrame_(uint32_t& out_can_id, uint8_t& out_dlc,
                        uint8_t* out_data, size_t out_size);

    int listen_fd_  = -1;   // 监听 socket
    int client_fd_  = -1;   // 当前客户端 socket（-1 表示无）
    char path_[108] = {};   // socket 路径（Unix 域 socket 最大长度）
    uint8_t rx_buf_[256] = {};  // 静态接收缓冲（车规：无动态分配）
    int rx_len_     = 0;    // rx_buf_ 中已用字节数
    bool client_connected_ = false;
    bool owns_path_ = false;  // 是否在 open() 中 unlink(path) — 测试时 false 避免误删共享路径
};

}  // namespace transport
}  // namespace candash
