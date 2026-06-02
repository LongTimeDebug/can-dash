// mock_data_binder.h
// IDataBinder 的 mock 实现（用于测试 ShmDataSource 推数据时不依赖 QtDataBinder）

#pragma once

#include "layer3/idata_binder.h"
#include <vector>
#include <mutex>

class MockDataBinder : public IDataBinder {
public:
    void onDataUpdated(const DisplaySnapshot& s) override {
        std::lock_guard<std::mutex> lock(m_mu);
        m_snapshots.push_back(s);
        m_dataCalls++;
    }

    void onHealthChanged(HealthStatus h) override {
        std::lock_guard<std::mutex> lock(m_mu);
        m_healths.push_back(h);
        m_healthCalls++;
    }

    // ─── 测试断言辅助 ───
    size_t snapshotCount() const { std::lock_guard<std::mutex> lock(m_mu); return m_snapshots.size(); }
    size_t healthCount() const { std::lock_guard<std::mutex> lock(m_mu); return m_healths.size(); }
    DisplaySnapshot lastSnapshot() const { std::lock_guard<std::mutex> lock(m_mu); return m_snapshots.empty() ? DisplaySnapshot{} : m_snapshots.back(); }
    HealthStatus lastHealth() const { std::lock_guard<std::mutex> lock(m_mu); return m_healths.empty() ? HEALTH_DISCONNECTED : m_healths.back(); }

private:
    mutable std::mutex m_mu;
    std::vector<DisplaySnapshot> m_snapshots;
    std::vector<HealthStatus> m_healths;
    int m_dataCalls = 0;
    int m_healthCalls = 0;
};
