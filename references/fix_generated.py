# fix_generated.py 模板后处理脚本

## 功能说明

`tools/fix_generated.py` 是 yaml_to_c.py 的 post-processing 步骤，用于修复生成的 C++ 代码中的模板语法错误。

## 处理的 Bug

### 1. `{{` / `}}` 转义修复

Python f-string 中 `{{` → `{`，`}}` → `}`。yaml_to_c.py 模板混用时会产生错误的双括号。

```python
# 修复前（错误）
const int FOOBAR = {{42}};  // 生成: const int FOOBAR = {42};

# 修复后
const int FOOBAR = 42;  // 生成: const int FOOBAR = 42;
```

### 2. 整数字面量 float 后缀

C++ 要求 `420.0f`，`420f` 是语法错误。

```python
# 修复: 420f → 420.0f, 2..0f → 2.0f
```

### 3. `ACTION_ACTION_` 重复前缀

alarm_rule_action_type 枚举值已含 `ACTION_` 前缀，变量名又加了一遍。

### 4. typedef struct vs struct 冲突

C++ 中 `typedef struct Foo Foo;` 与 `struct Foo;` 前向声明冲突。

**仅修复 .h 文件**，.cpp 文件不受影响。

## 运行方式

```bash
python tools/fix_generated.py src/generated
```

## CMake 集成

```cmake
find_package(Python3 COMPONENTS Interpreter REQUIRED)
add_custom_command(
    OUTPUT ${GENERATED_DIR}/.generated_stamp
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/fix_generated.py ${GENERATED_DIR}
    DEPENDS ${CMAKE_SOURCE_DIR}/config/*.yaml
            ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
            ${CMAKE_SOURCE_DIR}/tools/fix_generated.py
    COMMENT "Generating and fixing C++ from YAML"
    VERBATIM)
add_custom_target(run_yaml_to_c ALL DEPENDS ${GENERATED_DIR}/.generated_stamp)
add_dependencies(can-dash run_yaml_to_c)
```

## 验证修复

```bash
# 编译测试
/usr/bin/c++ -std=c++17 -I src/generated -c src/generated/seat_belt_table.cpp -o /tmp/test.o

# 语法检查
grep -c "{{" src/generated/*.cpp src/generated/*.h  # 应为 0
grep -c "420f" src/generated/*.cpp src/generated/*.h  # 应为 0
```
