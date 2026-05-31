# Layer 2 静态内存模式与 Runtime 编写规范

## 核心原则

Layer 2 **禁止动态内存**（无 new，无 malloc）。所有数据使用：
1. **栈上分配**：函数局部变量
2. **静态数组**：类成员变量（固定大小 buffer）

## 静态数组示例

```cpp
// indicator_runtime.h
static constexpr int MAX_INDICATORS = 32;

struct IndicatorState {
    const IndicatorDef* def;
    bool on;
    bool flash;
    float flashHz;
    uint64_t lastChangeMs;
};

class IndicatorRuntime {
    IndicatorState m_states[MAX_INDICATORS];  // 静态数组
    // ...
};
```

## 静态 Buffer 示例

```cpp
// can_signal_monitor.h
static constexpr int MAX_SIGNALS = 16;
static constexpr int MAX_SIGNAL_HISTORY = 32;

struct SignalState {
    const SignalMonitorDef* def;
    SignalQuality quality;
    float lastValue;
    float smoothedValue;
    float prevValue;
    uint64_t lastUpdateMs;
    bool received;
    int historyCount;
    int historyIndex;
    float* history;  // 指向 m_historyPool 中的槽位
};

class CanSignalMonitor {
    SignalState m_states[MAX_SIGNALS];  // 静态数组
    float m_historyPool[MAX_SIGNALS][MAX_SIGNAL_HISTORY];  // 池
    // ...
};
```

## 移除析构函数中的 delete[]

```cpp
// 错误（违反静态内存原则）
~CanSignalMonitor() {
    for (int i = 0; i < m_count; i++) {
        delete[] m_states[i].history;
    }
}

// 正确（静态 buffer 无需手动释放）
~CanSignalMonitor() {
    // 无需做任何事
}
```

## EventBus 单例

```cpp
class EventBus {
public:
    static EventBus& instance() {
        static EventBus inst;
        return inst;
    }

    void publish(const Event&& e);
    void subscribe(const char* key, std::function<void(const Event&)> cb);
    void subscribeWildcard(const char* pattern, std::function<void(const Event&)> cb);

private:
    EventBus() = default;
    std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> m_subscribers;
};
```

## Runtime 基类模式

```cpp
class Runtime {
public:
    virtual void init() = 0;
    virtual void tick(uint64_t now_ms) = 0;
    virtual ~Runtime() = default;
};
```

## 纯 C 回调结构（跨层解耦）

```cpp
// AlarmCallbacks：纯 C 结构，无 Qt
struct AlarmCallbacks {
    void (*onIndicatorUpdate)(const char* widget_id, bool on, bool flash, float flash_hz, void* user_data);
    void (*onAlarmTextUpdate)(const char* text_zh, const char* text_en, void* user_data);
    void (*onAlarmStateChanged)(const char* alarm_name, bool active, void* user_data);
    void* user_data;
};
```

## yaml_to_c.py 生成的数据表

所有数据表（alarm_rule_table.cpp, can_field_table.cpp 等）使用 const 数组：

```cpp
const CanFieldDef CAN_FIELD_TABLE[] = {
    { .display_key = "bat_volt", .can_id = 0x186040F3, ... },
    // ...
};
const int CAN_FIELD_COUNT = sizeof(CAN_FIELD_TABLE) / sizeof(CAN_FIELD_TABLE[0]);
```

**不要手动修改 generated/ 下的任何文件**，会被 yaml_to_c.py 覆盖。
