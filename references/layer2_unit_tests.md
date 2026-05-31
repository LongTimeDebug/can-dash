# Layer 2 单元测试编写指南

## 测试文件命名

```
tests/test_<module_name>.cpp
```

## 编译链接

CMakeLists.txt：
```cmake
add_executable(test_<name> tests/test_<name>.cpp
    src/layer2/<module>.cpp
    src/layer2/event_bus.cpp
    src/generated/alarm_rule_table.cpp
    src/generated/can_field_table.cpp
    src/generated/seat_belt_table.cpp
    src/generated/indicator_table.cpp
    src/generated/signal_table.cpp
)
target_link_libraries(test_<name> PRIVATE pthread)
```

## 测试规范

1. **纯 C++，无 Qt**：不 include 任何 Qt 头文件
2. **无动态内存**：使用栈上对象和 static buffer
3. **使用 assert**：C 风格 assert，无 gtest 依赖
4. **可独立运行**：`./test_<name>` 返回 0 表示全部通过

## 测试夹具

```cpp
// 通用 EventBus 重置
static std::vector<std::string> published_keys;
static void clear_events() { published_keys.clear(); }

int main() {
    printf("=== Test Name ===\n");
    // ...
}
```

## 示例：测试 EventBus

```cpp
#include <cassert>
#include <vector>
#include "../src/layer2/event_bus.h"

static std::vector<std::string> received;

int main() {
    printf("=== EventBus Test ===\n");

    EventBus::instance().subscribe("test", [](const Event& e) {
        received.push_back(e.key);
    });

    EventBus::instance().publish({"test", 1.0f, 0.0f, 0, nullptr});
    assert(received.size() == 1);
    assert(received[0] == "test");

    printf("  ✓ Test passed\n");
    printf("\nAll tests passed.\n");
    return 0;
}
```

## 示例：测试 CanSignalMonitor

```cpp
#include <cassert>
#include "../src/layer2/can_signal_monitor.h"

static float last_value = 0.0f;
static SignalQuality last_quality = SIGNAL_NEVER_RECEIVED;

int main() {
    printf("=== CanSignalMonitor Test ===\n");

    MonitorCallbacks cb;
    cb.onValueUpdated = [&](const char* name, float v, void*) { last_value = v; };
    cb.onQualityChanged = [&](const char* name, SignalQuality q, void*) { last_quality = q; };

    CanSignalMonitor monitor(cb);
    SignalMonitorDef def = { .name = "bat_volt", .can_id = 0x186040F3,
        .min_value = 0, .max_value = 500, .timeout_ms = 1000,
        .smoothing = true, .smoothing_window = 5, .max_delta = 50.0f };
    monitor.init(&def, 1);

    monitor.onCanFrame(0x186040F3, 350.0f);
    assert(last_quality == SIGNAL_GOOD);
    assert(last_value > 340.0f && last_value < 360.0f);

    printf("  ✓ Test passed\n");
    return 0;
}
```

## 运行测试

```bash
make test
# 或单独运行
./build/test_<name>
```

## 测试覆盖要点

- 初始状态（构造函数后）
- 边界值（0，最大值，超限）
- 状态机转换路径
- 超时/错误路径
- 并发/乱序事件

## CanSignalMonitor 测试注意

平滑值首次帧平均值行为：
```cpp
// 窗口=5，首次值=350.0 → smoothed = 350/5 = 70.0
// 因为 history 初始全为 0.0f
assert(monitor.getSmoothedValue("bat_volt") > 69.0f &&
       monitor.getSmoothedValue("bat_volt") < 71.0f);
```
