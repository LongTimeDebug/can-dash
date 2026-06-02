// idata_binder.h
// 数据绑定器抽象接口（不依赖具体 UI 框架）
//
// 设计目标：
// - Binder 只负责"把数据传给 UI"，不知道数据从哪来
// - 接收 DisplaySnapshot，更新 UI 状态
// - 接收 HealthStatus 变化，更新 UI 健康指示
//
// 实现：
// - QtDataBinder（src/layer3/qt_data_binder.cpp）：Q_PROPERTY + QML Connections
// - KanziDataBinder（未来）：kanzi::Property 绑定 + DataSource 集成
// - MockDataBinder（tests/mock_data_binder.cpp）：测试用，记录 onDataUpdated 调用

#pragma once

#include "display_data_types.h"

class IDataBinder {
public:
    virtual ~IDataBinder() = default;

    // 数据更新（DataSource 每帧/每变化调用一次）
    virtual void onDataUpdated(const DisplaySnapshot& snapshot) = 0;

    // 健康状态变化（仅在状态切换时调用）
    virtual void onHealthChanged(HealthStatus new_health) = 0;
};
