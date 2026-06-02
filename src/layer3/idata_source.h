// idata_source.h
// 数据源抽象接口（不依赖任何 UI 框架）
//
// 设计目标：
// - DataSource 只负责"接收数据"，不知道有 Qt / Kanzi / 任何 UI 概念
// - 推送 DisplaySnapshot 给订阅者（callback）
// - 推 HealthStatus 变化给订阅者
// - 同步接口：snapshot() 拿当前帧（用于 QML 初始化时一次性拉取）
//
// 实现：
// - ShmDataSource（src/layer3/shm_data_source.cpp）：从 /dev/shm/can_display 读取
// - MockDataSource（tests/mock_data_source.cpp）：测试用，手动 setSnapshot
// - 未来：UartDataSource / CanDataSource / EthernetDataSource

#pragma once

#include "display_data_types.h"
#include <functional>

class IDataSource {
public:
    // ─── 回调签名 ───
    using UpdateCallback = std::function<void(const DisplaySnapshot&)>;
    using HealthCallback = std::function<void(HealthStatus)>;

    virtual ~IDataSource() = default;

    // ─── 生命周期 ───
    // 启动数据源（开 shm / 启动定时器 / 调 setCallback）
    // 失败返回 false（如 shm 路径错）
    virtual bool start() = 0;

    // 停止数据源（关 shm / 停定时器）
    virtual void stop() = 0;

    // 是否已启动
    virtual bool isRunning() const = 0;

    // ─── 同步访问 ───
    // 拿当前快照（用于初始化时一次性拉取，避免 QML 第一帧空白）
    virtual DisplaySnapshot snapshot() const = 0;

    // 拿当前健康状态
    virtual HealthStatus health() const = 0;

    // ─── 异步订阅 ───
    // 数据更新回调（每帧/每变化触发）
    virtual void setUpdateCallback(UpdateCallback cb) = 0;

    // 健康状态变化回调（仅在状态切换时触发，避免每帧 emit）
    virtual void setHealthCallback(HealthCallback cb) = 0;
};
