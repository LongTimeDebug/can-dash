// sim_socket_transport.cpp
// Unix 域 socket 仿真传输实现
#include "sim_socket_transport.h"

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdio>

extern "C" {
#include "layer1/shm/shm_display.h"
}

namespace candash {
namespace transport {

SimSocketTransport::SimSocketTransport() {
    // 默认使用编译时/环境变量配置的全局路径
    const char* p = ::socket_get_path();
    if (p != nullptr) {
        std::strncpy(path_, p, sizeof(path_) - 1);
    }
    owns_path_ = true;  // open() 时 unlink 全局路径
}

SimSocketTransport::SimSocketTransport(const char* socket_path) {
    if (socket_path != nullptr) {
        std::strncpy(path_, socket_path, sizeof(path_) - 1);
    }
    owns_path_ = false;  // 测试路径：open() 不 unlink
}

SimSocketTransport::~SimSocketTransport() {
    close();
}

bool SimSocketTransport::open() {
    if (path_[0] == '\0') {
        std::fprintf(stderr, "[SimSocket] no socket path configured\n");
        return false;
    }
    if (owns_path_) {
        ::unlink(path_);
    }

    listen_fd_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_fd_ < 0) {
        std::perror("[SimSocket] socket() failed");
        return false;
    }

    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, path_, sizeof(addr.sun_path) - 1);

    if (::bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("[SimSocket] bind() failed");
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    if (::listen(listen_fd_, 3) < 0) {
        std::perror("[SimSocket] listen() failed");
        ::close(listen_fd_);
        listen_fd_ = -1;
        return false;
    }
    std::printf("[SimSocket] Listening on %s\n", path_);
    return true;
}

void SimSocketTransport::close() {
    if (client_fd_ >= 0) {
        ::close(client_fd_);
        client_fd_ = -1;
        client_connected_ = false;
        rx_len_ = 0;
    }
    if (listen_fd_ >= 0) {
        ::close(listen_fd_);
        listen_fd_ = -1;
    }
}

int SimSocketTransport::acceptClient_() {
    if (client_fd_ >= 0) {
        ::close(client_fd_);
        client_fd_ = -1;
        client_connected_ = false;
        rx_len_ = 0;
    }
    client_fd_ = ::accept(listen_fd_, nullptr, nullptr);
    if (client_fd_ < 0) {
        std::perror("[SimSocket] accept() failed");
        return -1;
    }
    client_connected_ = true;
    rx_len_ = 0;
    std::printf("[SimSocket] Client connected\n");
    return 0;
}

bool SimSocketTransport::tryParseFrame_(uint32_t& out_can_id, uint8_t& out_dlc,
                                       uint8_t* out_data, size_t out_size) {
    if (client_fd_ < 0) return false;

    // 尝试从 socket 读更多字节
    uint8_t buf[64];
    const ssize_t r = ::read(client_fd_, buf, sizeof(buf));
    if (r <= 0) {
        // 客户端断开
        ::close(client_fd_);
        client_fd_ = -1;
        client_connected_ = false;
        rx_len_ = 0;
        return false;
    }

    // 拼接到 rx_buf_
    for (ssize_t i = 0; i < r; ++i) {
        if (rx_len_ < static_cast<int>(sizeof(rx_buf_))) {
            rx_buf_[static_cast<size_t>(rx_len_++)] = buf[static_cast<size_t>(i)];
        }
    }

    // 解析一帧：[4B id][1B dlc][N bytes data]
    if (rx_len_ < 5) return false;

    const uint8_t dlc = rx_buf_[4];
    const int frame_len = 5 + static_cast<int>(dlc);
    if (rx_len_ < frame_len) return false;

    // 边界检查
    if (dlc > kCanMaxDlc) {
        // dlc 非法：跳过这 5 字节，重新尝试（防僵死）
        std::memmove(rx_buf_, &rx_buf_[5], static_cast<size_t>(rx_len_ - 5));
        rx_len_ -= 5;
        return false;
    }
    if (out_size < dlc) {
        // 调用方缓冲不够，丢弃
        std::memmove(rx_buf_, &rx_buf_[frame_len], static_cast<size_t>(rx_len_ - frame_len));
        rx_len_ -= frame_len;
        return false;
    }

    out_can_id =  static_cast<uint32_t>(rx_buf_[0])
               | (static_cast<uint32_t>(rx_buf_[1]) << 8)
               | (static_cast<uint32_t>(rx_buf_[2]) << 16)
               | (static_cast<uint32_t>(rx_buf_[3]) << 24);
    out_dlc = dlc;
    std::memcpy(out_data, &rx_buf_[5], dlc);

    // 从缓冲中移除已消费数据
    if (rx_len_ > frame_len) {
        std::memmove(rx_buf_, &rx_buf_[frame_len], static_cast<size_t>(rx_len_ - frame_len));
    }
    rx_len_ -= frame_len;
    return true;
}

bool SimSocketTransport::readFrame(uint32_t& can_id, uint8_t& dlc,
                                   uint8_t* data, size_t data_size,
                                   int timeout_ms) {
    if (listen_fd_ < 0) return false;

    // poll 同时关注 listen_fd（接受新连接）和 client_fd（读数据）
    struct pollfd pfds[2];
    pfds[0].fd = listen_fd_;
    pfds[0].events = POLLIN;
    const int nfds = (client_fd_ >= 0) ? 2 : 1;
    if (client_fd_ >= 0) {
        pfds[1].fd = client_fd_;
        pfds[1].events = POLLIN;
    }

    const int n = ::poll(pfds, nfds, timeout_ms);
    if (n < 0) return false;

    // 新客户端连接
    if (pfds[0].revents & POLLIN) {
        if (acceptClient_() < 0) return false;
    }

    // 已连接客户端有数据
    if (client_fd_ >= 0 && (pfds[1].revents & POLLIN)) {
        return tryParseFrame_(can_id, dlc, data, data_size);
    }
    return false;
}

}  // namespace transport
}  // namespace candash
