# yaml_to_c.py bug 模式与修复

## fix_generated.py 后处理脚本

yaml_to_c.py 生成的代码有多种模板 bug，必须通过 CMake post-processing 修复：

```cmake
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_command(OUTPUT ${GENERATED_DIR}/.generated_stamp
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
        COMMAND ${Python3_EXECUTABLE} ${FIX_TEMPLATE_SCRIPT} ${GENERATED_DIR}
        DEPENDS ${CMAKE_SOURCE_DIR}/config/*.yaml
                ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
                ${FIX_TEMPLATE_SCRIPT}
        COMMENT "Running yaml_to_c.py and fixing templates"
        VERBATIM)
    add_custom_target(run_yaml_to_c ALL DEPENDS ${GENERATED_DIR}/.generated_stamp)
    add_dependencies(can-dash run_yaml_to_c)
endif()
```

## 已知 Bug 模式

### 1. `{{` / `}}` 转义错误

yaml_to_c.py 的 triple-quoted 模板中：
- Python `f"{{x}}"` 输出 `{x}`（正确）
- Python `f"{{x}}"` 配合模板外的 `{{` → `{{` → `{`（错误）

**fix_generated.py 修复**：`{{` → `{`，`}}` → `}`

### 2. `420f` 整数字面量

C++ 需要 `420.0f`，不是 `420f`。

**fix_generated.py 修复**：`420f` → `420.0f`，`2..0f` → `2.0f`

### 3. `ACTION_ACTION_` 重复前缀

action_type 变量值已含 `ACTION_` 前缀，代码又加了一遍。

**fix_generated.py 修复**：`ACTION_ACTION_` → `ACTION_`

### 4. seat_belt_table 嵌套大括号

```python
# 错误（已永久修复）
config_str = f"    {{{trigger.get('speed_threshold', 5.0)}f, ..."

# 正确
config_str = f"    {trigger.get('speed_threshold', 5.0)}f, ..."
```

**注意**：fix_generated.py 的正则替换**无法检测此 bug**，必须修复 yaml_to_c.py 源码。

### 5. typedef struct 在 C++ 中冲突

**fix_generated.py 修复**：仅 .h 文件中 `typedef struct {\n...} NAME;` → `struct NAME {\n...};`
