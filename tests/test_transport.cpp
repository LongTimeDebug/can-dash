// tests/test_transport.cpp
// Unit tests for CAN transport layer abstraction (sim-socket + socketcan + factory).
//
// Coverage:
//   - TransportFactory::parseArgs  : CLI 参数解析
//   - TransportFactory::create    : 工厂产出正确类型
//   - SimSocketTransport          : 端到端 round-trip（自建 client 发帧）
//   - SocketCanTransport          : 接口不存在时优雅失败
//
// Why this matters: 传输层是 can-processor 的"硬件边界"，车规要求
// 替换传输（如从 sim 切到真车 CAN）不应改动业务层代码。本测试锁住
// 抽象接口的契约。

#include "layer2/transport/can_transport.h"
#include "layer2/transport/sim_socket_transport.h"
#include "layer2/transport/socketcan_transport.h"
#include "layer2/transport/transport_factory.h"

#include <cstdio>
#include <cstring>
#include <cassert>
#include <cstdlib>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <poll.h>

using namespace candash::transport;

// 测试用断言宏：Release 模式下也必须 abort（不依赖 NDEBUG）
#define TEST_ASSERT(cond)                                                          \
    do {                                                                            \
        if (!(cond)) {                                                              \
            std::fprintf(stderr, "ASSERT FAIL: %s  at  %s:%d\n",                    \
                         #cond, __FILE__, __LINE__);                                \
            std::abort();                                                           \
        }                                                                           \
    } while (0)

namespace {

// ── 测试 1: TransportFactory::parseArgs ─────────────────────
void test_factory_parse_sim() {
    char arg0[] = "test_transport";
    char arg1[] = "--transport=sim";
    char* argv[] = {arg0, arg1, nullptr};
    const TransportConfig cfg = TransportFactory::parseArgs(2, argv);
    TEST_ASSERT(cfg.type == TransportType::kSimSocket);
    std::printf("  parseArgs --transport=sim -> kSimSocket (OK)\n");
}

void test_factory_parse_socketcan() {
    char arg0[] = "test_transport";
    char arg1[] = "--transport=socketcan";
    char arg2[] = "--can-if=vcan0";
    char* argv[] = {arg0, arg1, arg2, nullptr};
    const TransportConfig cfg = TransportFactory::parseArgs(3, argv);
    TEST_ASSERT(cfg.type == TransportType::kSocketCan);
    TEST_ASSERT(std::strcmp(cfg.can_if_name, "vcan0") == 0);
    std::printf("  parseArgs --transport=socketcan --can-if=vcan0 -> "
                "kSocketCan, if=vcan0 (OK)\n");
}

void test_factory_parse_default() {
    char arg0[] = "test_transport";
    char* argv[] = {arg0, nullptr};
    const TransportConfig cfg = TransportFactory::parseArgs(1, argv);
    TEST_ASSERT(cfg.type == TransportType::kSimSocket);
    std::printf("  parseArgs (no args) -> kSimSocket default (OK)\n");
}

// ── 测试 2: TransportFactory::create ────────────────────────
void test_factory_create_sim() {
    TransportConfig cfg;
    cfg.type = TransportType::kSimSocket;
    auto t = TransportFactory::create(cfg);
    TEST_ASSERT(t != nullptr);
    TEST_ASSERT(std::strcmp(t->name(), "sim-socket") == 0);
    TEST_ASSERT(!t->isOpen());
    TEST_ASSERT(!t->isReady());
    std::printf("  create(kSimSocket) -> '%s', not open, not ready (OK)\n",
                t->name());
}

void test_factory_create_socketcan() {
    TransportConfig cfg;
    cfg.type = TransportType::kSocketCan;
    std::strncpy(cfg.can_if_name, "vcan_test", sizeof(cfg.can_if_name) - 1);
    auto t = TransportFactory::create(cfg);
    TEST_ASSERT(t != nullptr);
    TEST_ASSERT(std::strcmp(t->name(), "socketcan:vcan_test") == 0);
    std::printf("  create(kSocketCan) -> '%s' (OK)\n", t->name());
}

// ── 测试 3: SimSocketTransport 端到端 round-trip ─────────────
// 创建 server transport + 客户端 raw socket，发一帧，验证 server 收到
void test_sim_socket_roundtrip() {
    const char* test_path = "/tmp/candash_test_transport.sock";
    ::unlink(test_path);  // 清理残留

    // 1) 启动 server
    SimSocketTransport server(test_path);
    TEST_ASSERT(server.open());
    TEST_ASSERT(server.isOpen());
    TEST_ASSERT(!server.isReady());  // 客户端未连

    // 2) 客户端连接（用 raw socket API）
    const int client_fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    TEST_ASSERT(client_fd >= 0);
    struct sockaddr_un addr;
    std::memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, test_path, sizeof(addr.sun_path) - 1);
    TEST_ASSERT(::connect(client_fd, reinterpret_cast<struct sockaddr*>(&addr),
                     sizeof(addr)) == 0);

    // 3) 等 server 接受连接（accept 是 poll 触发的）
    uint32_t can_id = 0;
    uint8_t dlc = 0;
    uint8_t data[candash::transport::kCanMaxDlc] = {};
    // readFrame 应该先 accept（poll 检测到 listen_fd POLLIN），然后
    // 尝试从 client 读 — 但此时 client 还没发数据，应该返回 false
    const bool got_nothing = server.readFrame(can_id, dlc, data, sizeof(data), 100);
    TEST_ASSERT(!got_nothing);
    // 但 isReady() 现在应该是 true（client 已连）
    TEST_ASSERT(server.isReady());
    std::printf("  server accepted client connection, isReady()=true (OK)\n");

    // 4) client 发一帧：[can_id=0x123][dlc=3][data=0xAA,0xBB,0xCC]
    const uint8_t frame[8] = {0x23, 0x01, 0x00, 0x00, 0x03, 0xAA, 0xBB, 0xCC};
    const ssize_t w = ::write(client_fd, frame, sizeof(frame));
    TEST_ASSERT(w == static_cast<ssize_t>(sizeof(frame)));

    // 5) server 读帧
    const bool got_frame = server.readFrame(can_id, dlc, data, sizeof(data), 1000);
    TEST_ASSERT(got_frame);
    TEST_ASSERT(can_id == 0x00000123);  // little-endian decode
    TEST_ASSERT(dlc == 3);
    TEST_ASSERT(data[0] == 0xAA);
    TEST_ASSERT(data[1] == 0xBB);
    TEST_ASSERT(data[2] == 0xCC);
    std::printf("  round-trip: can_id=0x%03X dlc=%u data=%02X %02X %02X (OK)\n",
                can_id, dlc, data[0], data[1], data[2]);

    // 6) 测超时：client 不发，readFrame 应该 timeout 返回 false
    const bool timed_out = server.readFrame(can_id, dlc, data, sizeof(data), 50);
    TEST_ASSERT(!timed_out);
    std::printf("  timeout (50ms) returns false (OK)\n");

    // 7) client 断开后 readFrame 应该返回 false
    ::close(client_fd);
    // 等 server poll 检测到 POLLHUP
    for (int i = 0; i < 5; ++i) {
        server.readFrame(can_id, dlc, data, sizeof(data), 50);
        if (!server.isReady()) break;
    }
    TEST_ASSERT(!server.isReady());
    std::printf("  client disconnect -> isReady()=false (OK)\n");

    server.close();
    TEST_ASSERT(!server.isOpen());
    // 测试路径不删，留给下次 unlink — 避免与其他并发测试冲突
    ::unlink(test_path);
}

// ── 测试 4: SocketCanTransport 缺接口时优雅失败 ─────────────
void test_socketcan_missing_iface_fails() {
    SocketCanTransport t("definitely_not_a_can_iface_xyz");
    const bool opened = t.open();
    TEST_ASSERT(!opened);
    TEST_ASSERT(!t.isOpen());
    TEST_ASSERT(!t.isReady());
    std::printf("  socketcan missing iface -> open()=false, isOpen()=false (OK)\n");
}

// ── 测试 5: SocketCanTransport 接口名校验（>15 字符）────────
void test_socketcan_long_iface_name() {
    char long_name[64];
    std::memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    SocketCanTransport t(long_name);
    const bool opened = t.open();
    TEST_ASSERT(!opened);
    std::printf("  socketcan long ifname -> open()=false (OK)\n");
}

}  // namespace

int main() {
    std::printf("test_transport\n");

    std::printf("[Factory: parseArgs]\n");
    test_factory_parse_default();
    test_factory_parse_sim();
    test_factory_parse_socketcan();

    std::printf("[Factory: create]\n");
    test_factory_create_sim();
    test_factory_create_socketcan();

    std::printf("[SimSocketTransport]\n");
    test_sim_socket_roundtrip();

    std::printf("[SocketCanTransport]\n");
    test_socketcan_missing_iface_fails();
    test_socketcan_long_iface_name();

    std::printf("ALL PASS\n");
    return 0;
}
