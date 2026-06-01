// socketcan_transport.cpp
// Linux SocketCAN 真实硬件传输实现
#include "socketcan_transport.h"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <poll.h>
#include <cstring>
#include <cstdio>
#include <cerrno>

// Linux 内核 CAN 头（仅 Linux 可用）
#if defined(__linux__)
#  include <linux/can.h>
#  include <linux/can/raw.h>
#  define CANDASH_HAVE_SOCKETCAN 1
#else
#  define CANDASH_HAVE_SOCKETCAN 0
#endif

namespace candash {
namespace transport {

SocketCanTransport::SocketCanTransport(const char* can_if_name) {
    if (can_if_name == nullptr || can_if_name[0] == '\0') {
        can_if_name = "can0";
    }
    // 格式："socketcan:<ifname>"，最长 31 字符
    std::snprintf(name_, sizeof(name_), "socketcan:%s", can_if_name);
}

SocketCanTransport::~SocketCanTransport() {
    close();
}

bool SocketCanTransport::open() {
#if CANDASH_HAVE_SOCKETCAN
    // 提取接口名（跳过 "socketcan:" 前缀）
    const char* ifname = std::strchr(name_, ':');
    ifname = (ifname != nullptr) ? ifname + 1 : name_;

    // 接口名长度校验（IFNAMSIZ - 1 = 15）
    if (std::strlen(ifname) >= IFNAMSIZ) {
        std::fprintf(stderr, "[SocketCAN] ifname too long: %s\n", ifname);
        return false;
    }

    sock_fd_ = ::socket(AF_CAN, SOCK_RAW, CAN_RAW);
    if (sock_fd_ < 0) {
        std::fprintf(stderr, "[SocketCAN] socket() failed: %s (errno=%d)\n",
                     std::strerror(errno), errno);
        return false;
    }

    // 查接口 index（不存在则 if_nametoindex 返回 0）
    const unsigned int ifindex = ::if_nametoindex(ifname);
    if (ifindex == 0) {
        std::fprintf(stderr, "[SocketCAN] interface '%s' not found (errno=%d: %s)\n",
                     ifname, errno, std::strerror(errno));
        ::close(sock_fd_);
        sock_fd_ = -1;
        return false;
    }

    struct sockaddr_can addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.can_family  = AF_CAN;
    addr.can_ifindex = static_cast<int>(ifindex);

    if (::bind(sock_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::fprintf(stderr, "[SocketCAN] bind() to '%s' failed: %s (errno=%d)\n",
                     ifname, std::strerror(errno), errno);
        ::close(sock_fd_);
        sock_fd_ = -1;
        return false;
    }

    std::printf("[SocketCAN] Bound to %s (ifindex=%u, sock_fd=%d)\n",
                ifname, ifindex, sock_fd_);
    return true;
#else
    std::fprintf(stderr, "[SocketCAN] not supported on this platform (need Linux)\n");
    return false;
#endif
}

void SocketCanTransport::close() {
    if (sock_fd_ >= 0) {
        ::close(sock_fd_);
        sock_fd_ = -1;
    }
}

bool SocketCanTransport::readFrame(uint32_t& can_id, uint8_t& dlc,
                                   uint8_t* data, size_t data_size,
                                   int timeout_ms) {
#if CANDASH_HAVE_SOCKETCAN
    if (sock_fd_ < 0) return false;
    if (data_size < kCanMaxDlc) {
        std::fprintf(stderr, "[SocketCAN] data buffer too small (%zu < %zu)\n",
                     data_size, kCanMaxDlc);
        return false;
    }

    struct pollfd pfd;
    pfd.fd = sock_fd_;
    pfd.events = POLLIN;

    const int n = ::poll(&pfd, 1, timeout_ms);
    if (n <= 0) return false;  // 超时或错误

    struct can_frame frame;
    const ssize_t r = ::read(sock_fd_, &frame, sizeof(frame));
    if (r != static_cast<ssize_t>(sizeof(frame))) {
        std::fprintf(stderr, "[SocketCAN] short read: %zd/%zu\n", r, sizeof(frame));
        return false;
    }

    // 过滤掉错误帧和 RTR 帧（业务层不处理）
    if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG)) {
        return false;
    }

    // CAN 2.0 标准帧 11 位，扩展帧 29 位
    // can_id 已包含 CAN_EFF_FLAG，业务层不关心，mask 掉即可
    constexpr uint32_t kCanIdMask = 0x1FFFFFFFU;
    can_id = frame.can_id & kCanIdMask;

    // 边界检查
    if (frame.can_dlc > kCanMaxDlc) {
        std::fprintf(stderr, "[SocketCAN] invalid dlc=%u\n", frame.can_dlc);
        return false;
    }
    dlc = frame.can_dlc;
    std::memcpy(data, frame.data, dlc);
    return true;
#else
    (void)can_id; (void)dlc; (void)data; (void)data_size; (void)timeout_ms;
    return false;
#endif
}

}  // namespace transport
}  // namespace candash
