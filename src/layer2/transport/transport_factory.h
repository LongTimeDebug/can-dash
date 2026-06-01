// transport_factory.h
// 传输层工厂（根据 CLI 参数选择 transport 实现）
#pragma once

#include "can_transport.h"
#include <memory>

namespace candash {
namespace transport {

// 支持的传输类型
enum class TransportType {
    kSimSocket,    // Unix 域 socket（PC 仿真，python-can engine.py）
    kSocketCan,    // Linux SocketCAN（生产环境，can0/vcan0）
};

// 工厂配置
struct TransportConfig {
    TransportType type = TransportType::kSimSocket;
    char can_if_name[16] = "can0";  // 仅 socketcan 使用
};

// CLI 参数解析 + 工厂创建
// 返回 nullptr 表示参数错误（usage 已打印）
class TransportFactory {
public:
    // 解析 argv（修改之）。返回 Config，失败时 usage 打印到 stderr。
    // 支持的选项：
    //   --transport=sim|socketcan
    //   --can-if=can0
    static TransportConfig parseArgs(int argc, char** argv);

    // 根据配置创建 transport 实例
    // 注意：失败时返回 nullptr（应检查 errno 上下文）
    static std::unique_ptr<ICanTransport> create(const TransportConfig& cfg);

    // 打印 usage 到 stdout
    static void printUsage(const char* progname);
};

}  // namespace transport
}  // namespace candash
