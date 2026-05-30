cmake_minimum_required(VERSION 3.20)
project(can-dash VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# ─── Qt 6 ───
find_package(Qt6 REQUIRED COMPONENTS Core Qml Quick Svg)

# ─── 项目结构 ───
set(GENERATED_DIR ${CMAKE_SOURCE_DIR}/src/generated)
set(LAYER1_DIR ${CMAKE_SOURCE_DIR}/src/layer1)
set(LAYER2_DIR ${CMAKE_SOURCE_DIR}/src/layer2)
set(LAYER3_DIR ${CMAKE_SOURCE_DIR}/src/layer3)
set(UI_DIR ${CMAKE_SOURCE_DIR}/src/ui)

# ─── 查找生成的 C 代码 ───
file(GLOB GENERATED_SOURCES
    ${GENERATED_DIR}/*.c
    ${GENERATED_DIR}/*.cpp
    ${GENERATED_DIR}/*.h
)

file(GLOB LAYER1_SOURCES
    ${LAYER1_DIR}/*.h
)

file(GLOB LAYER2_SOURCES
    ${LAYER2_DIR}/*.cpp
    ${LAYER2_DIR}/*.h
)

file(GLOB LAYER3_SOURCES
    ${LAYER3_DIR}/*.cpp
    ${LAYER3_DIR}/*.h
)

# ─── 主应用 ───
add_executable(can-dash
    src/main.cpp
    ${LAYER1_SOURCES}
    ${LAYER2_SOURCES}
    ${LAYER3_SOURCES}
    ${GENERATED_SOURCES}
)

target_include_directories(can-dash PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${GENERATED_DIR}
    ${LAYER1_DIR}
    ${LAYER2_DIR}
    ${LAYER3_DIR}
)

target_link_libraries(can-dash PRIVATE
    Qt6::Core
    Qt6::Qml
    Qt6::Quick
    Qt6::Svg
)

# ─── 编译前自动运行 yaml_to_c.py ───
find_package(Python3 COMPONENTS Interpreter)
if(Python3_Interpreter_FOUND)
    add_custom_command(OUTPUT ${GENERATED_DIR}/.generated_stamp
        COMMAND ${Python3_EXECUTABLE} ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
        DEPENDS
            ${CMAKE_SOURCE_DIR}/config/*.yaml
            ${CMAKE_SOURCE_DIR}/tools/yaml_to_c.py
        COMMENT "Running yaml_to_c.py to generate C code from YAML"
        VERBATIM
    )
    add_custom_target(run_yaml_to_c ALL
        DEPENDS ${GENERATED_DIR}/.generated_stamp
    )
    add_dependencies(can-dash run_yaml_to_c)
endif()

# ─── 单元测试 ───
enable_testing()
add_executable(test_can_converter tests/test_can_converter.cpp
    ${LAYER1_SOURCES} ${LAYER2_SOURCES} ${GENERATED_SOURCES})
target_link_libraries(test_can_converter PRIVATE Qt6::Core)
add_test(NAME CanConverterTest COMMAND test_can_converter)

add_executable(test_alarm_runtime tests/test_alarm_runtime.cpp
    ${LAYER1_SOURCES} ${LAYER2_SOURCES} ${GENERATED_SOURCES})
target_link_libraries(test_alarm_runtime PRIVATE Qt6::Core)
add_test(NAME AlarmRuntimeTest COMMAND test_alarm_runtime)
