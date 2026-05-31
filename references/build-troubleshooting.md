# 构建排障参考

## 常见编译错误

### expected unqualified-id before '{' token

**根因**：yaml_to_c.py 模板 bug，`{{` 产生 `{`，导致生成的 C 代码有语法错误。

**解法**：
```bash
python tools/fix_generated.py src/generated
```

### conflicting declaration 'typedef struct Foo Foo'

**根因**：C typedef struct 在 C++ 中冲突。

**解法**：fix_generated.py 将 `typedef struct { ... } NAME;` 替换为 `struct NAME { ... };`（仅 .h 文件）。

### cannot convert '<brace-enclosed initializer list>' to 'Event&&'

**根因**：`EventBus::publish(const Event&& event)` 接收 rvalue，但 `{.key=..., .value=...}` 是 lvalue。

**解法**：将 `void publish(const Event&)` 改为 `void publish(const Event&&)`。

### undefined reference to main

**根因**：GLOB 把 .h 文件也加入了编译队列。

**解法**：只 GLOB .c 和 .cpp：
```cmake
file(GLOB GENERATED_SOURCES CONFIGURE_DEPENDS
    ${GENERATED_DIR}/*.c
    ${GENERATED_DIR}/*.cpp
)
```

### QCoreApplication + QQmlApplicationEngine SIGSEGV

**根因**：`QCoreApplication` 不提供 GUI 上下文。

**解法**：`QCoreApplication` → `QGuiApplication`。

## cmake 反复重新运行

```bash
rm -rf build && mkdir build && cd build && cmake .. && make -j$(nproc)
```

## Qt6 头文件找不到

```bash
dpkg -l qt6-base-dev qt6-declarative-dev
find /usr/include/x86_64-linux-gnu/qt6 -name "qqmlcontext.h"
```

## QML 模块缺失

```bash
sudo apt install qml6-module-qtquick qml6-module-qtquick-controls \
    qml6-module-qtquick-layouts qml6-module-qtqml-workerscript \
    qml6-module-qtquick-templates qml6-module-qtquick-window
```

## rebuild 完整清理

```bash
rm -rf CMakeCache.txt CMakeFiles build/qml/*.qml
cp src/ui/*.qml build/qml/
make -j$(nproc)
```
