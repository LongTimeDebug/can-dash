// mock_data_source.h
// IDataSource 的 mock 实现（用于测试 QtDataBinder 和 DashboardBackend 胶水层）

#pragma once

#include "layer3/idata_source.h"
#include <mutex>

class MockDataSource : public IDataSource {
public:
    bool start() override { m_running = true; return true; }
    void stop() override { m_running = false; }
    bool isRunning() const override { return m_running; }
    DisplaySnapshot snapshot() const override { std::lock_guard<std::mutex> lock(m_mu); return m_snapshot; }
    HealthStatus health() const override { std::lock_guard<std::mutex> lock(m_mu); return m_snapshot.health; }
    void setUpdateCallback(UpdateCallback cb) override { std::lock_guard<std::mutex> lock(m_mu); m_updateCb = std::move(cb); }
    void setHealthCallback(HealthCallback cb) override { std::lock_guard<std::mutex> lock(m_mu); m_healthCb = std::move(cb); }

    // ─── 测试辅助方法 ───
    void pushSnapshot(const DisplaySnapshot& s) {
        std::lock_guard<std::mutex> lock(m_mu);
        m_snapshot = s;
        if (m_updateCb) m_updateCb(s);
    }

    void pushHealth(HealthStatus h) {
        std::lock_guard<std::mutex> lock(m_mu);
        m_snapshot.health = h;
        if (m_healthCb) m_healthCb(h);
    }

    int updateCallCount() const { return m_updateCalls; }
    int healthCallCount() const { return m_healthCalls; }

private:
    mutable std::mutex m_mu;
    bool m_running = false;
    DisplaySnapshot m_snapshot{};
    UpdateCallback m_updateCb;
    HealthCallback m_healthCb;
    int m_updateCalls = 0;
    int m_healthCalls = 0;
};
