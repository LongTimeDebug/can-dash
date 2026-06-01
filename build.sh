#!/bin/bash
# CAN-Dash 构建脚本

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

echo "=== 1. 生成 C 代码 ==="
# Auto-detect python (优先 python3.11，回退 python3)
PYTHON=$(command -v python3.11 || command -v python3)
if [ -z "$PYTHON" ]; then
    echo "ERROR: python3 not found" >&2
    exit 1
fi
"$PYTHON" tools/yaml_to_c.py
"$PYTHON" tools/fix_generated.py src/generated

echo "=== 2. CMake 配置 ==="
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake ..

echo "=== 3. 编译 ==="
make -j$(nproc)

echo "=== 4. 复制 QML 文件 ==="
mkdir -p qml
cp -r ../src/ui/*.qml qml/ 2>/dev/null || true

echo ""
echo "编译完成！运行：./run.sh"
