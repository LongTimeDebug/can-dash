// transport_factory.cpp
#include "transport_factory.h"
#include "sim_socket_transport.h"
#include "socketcan_transport.h"

#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <getopt.h>

namespace candash {
namespace transport {

void TransportFactory::printUsage(const char* progname) {
    std::fprintf(stderr,
        "Usage: %s [options]\n"
        "  --transport=sim|socketcan  CAN transport (default: sim)\n"
        "  --can-if=<name>            SocketCAN interface (default: can0)\n"
        "  -h, --help                 Show this help\n"
        "\n"
        "Examples:\n"
        "  %s --transport=sim                  # PC sim with can_sim/engine.py\n"
        "  %s --transport=socketcan --can-if=can0   # Real hardware\n"
        "  %s --transport=socketcan --can-if=vcan0  # Virtual CAN (vcan module)\n",
        progname, progname, progname, progname);
}

TransportConfig TransportFactory::parseArgs(int argc, char** argv) {
    TransportConfig cfg;
    static const struct option long_opts[] = {
        {"transport", required_argument, nullptr, 't'},
        {"can-if",    required_argument, nullptr, 'i'},
        {"help",      no_argument,       nullptr, 'h'},
        {nullptr,     0,                 nullptr,  0 }
    };

    // 重置 getopt（防多进程测试时残留）
    optind = 1;
    int ch;
    while ((ch = getopt_long(argc, argv, "t:i:h", long_opts, nullptr)) != -1) {
        switch (ch) {
        case 't':
            if (std::strcmp(optarg, "sim") == 0) {
                cfg.type = TransportType::kSimSocket;
            } else if (std::strcmp(optarg, "socketcan") == 0) {
                cfg.type = TransportType::kSocketCan;
            } else {
                std::fprintf(stderr, "Unknown transport: '%s'\n", optarg);
                printUsage(argv[0]);
                std::exit(2);
            }
            break;
        case 'i':
            std::strncpy(cfg.can_if_name, optarg, sizeof(cfg.can_if_name) - 1);
            cfg.can_if_name[sizeof(cfg.can_if_name) - 1] = '\0';
            break;
        case 'h':
        default:
            printUsage(argv[0]);
            std::exit(0);
        }
    }
    return cfg;
}

std::unique_ptr<ICanTransport> TransportFactory::create(const TransportConfig& cfg) {
    switch (cfg.type) {
    case TransportType::kSimSocket:
        return std::unique_ptr<ICanTransport>(new SimSocketTransport());
    case TransportType::kSocketCan:
        return std::unique_ptr<ICanTransport>(new SocketCanTransport(cfg.can_if_name));
    }
    return nullptr;  // 不应到达
}

}  // namespace transport
}  // namespace candash
