#!/bin/bash
# CAN-Dash 运行脚本

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/build"

export QT_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/qt6/plugins
export QML2_IMPORT_PATH=/usr/lib/x86_64-linux-gnu/qt6/qml
# VMware 虚拟显卡 OpenGL 不稳定，强制软件渲染
export QT_QUICK_BACKEND=software
export QT_OPENGL=software
# 强制使用 X11 平台（eglfs 旁路 X11 导致无窗口）
export QT_QPA_PLATFORM=xcb
# 禁止插件加载调试输出
export QT_LOGGING_RULES="*.warning=false;qt.core.plugin.warning=false"

cd "$BUILD_DIR" || exit 1

# 启动 can-dash
exec ./can-dash
