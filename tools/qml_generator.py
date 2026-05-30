#!/usr/bin/env python3
"""
QML 代码生成器
从 display_layout.yaml 生成 DashboardMain.qml

Usage:
    python tools/qml_generator.py
"""

import yaml
from pathlib import Path

CONFIG_DIR = Path(__file__).parent.parent / "config"
UI_DIR = Path(__file__).parent.parent / "src" / "ui"


def load_yaml(name: str):
    path = CONFIG_DIR / name
    with open(path) as f:
        return yaml.safe_load(f)


TYPE_MAP = {
    "gauge": "GaugeControl",
    "bar": "BarIndicator",
    "light": "WarningLight",
    "text": "TextDisplay",
    "image": "ImageDisplay",
    "custom": "Item",
}


def generate_qml(layout_data: dict) -> str:
    display = layout_data.get("display", {})
    pages = display.get("pages", [])
    resolution = display.get("resolution", {"width": 800, "height": 480})
    bg_color = display.get("background", "#0A0A0A")
    theme = display.get("theme", "dark")

    lines = [
        "// ⚠️ 此文件由 tools/qml_generator.py 自动生成",
        "// ⚠️ 请勿手动修改，修改请改 config/display_layout.yaml",
        "",
        f"import QtQuick {2 if resolution.get('width', 800) >= 1024 else 1}",
        "import QtQuick.Controls 2.5",
        "import QtQuick.Layouts 1.3",
        "",
        "Item {",
        f"    id: root",
        f"    width: {resolution['width']}",
        f"    height: {resolution['height']}",
        f"    visible: true",
        f"    clip: true",
        f"    property bool isMoving: dashboard.isMoving",
        f"    property string theme: \"{theme}\"",
        "",
        "    // 背景",
        "    Rectangle {",
        f"        anchors.fill: parent",
        f"        color: \"{bg_color}\"",
        "    }",
        "",
    ]

    for page in pages:
        page_id = page.get("id", "main")
        layers = page.get("layers", [])

        lines.append(f"    // === Page: {page_id} ===")

        for i, layer in enumerate(layers):
            comp_type = layer.get("type", "text")
            comp_id = layer.get("id", f"layer_{i}")
            pos = layer.get("position", {})
            size = layer.get("size", {})
            bindings = layer.get("bindings", {})
            config = layer.get("config", {})

            qml_type = TYPE_MAP.get(comp_type, "Text")

            lines.append(f"    {qml_type} {{")
            lines.append(f"        id: {comp_id}")
            lines.append(f"        x: {pos.get('x', 0)}")
            lines.append(f"        y: {pos.get('y', 0)}")
            lines.append(f"        width: {size.get('width', 100)}")
            lines.append(f"        height: {size.get('height', 50)}")

            if qml_type == "GaugeControl":
                lines.append(f"        minValue: {config.get('min', 0)}")
                lines.append(f"        maxValue: {config.get('max', 260)}")
                lines.append("        value: dashboard[\"speed\"] || 0")
                lines.append(f"        unit: \"{config.get('unit', 'km/h')}\"")

            elif qml_type == "BarIndicator":
                lines.append(f"        minValue: {config.get('min', 0)}")
                lines.append(f"        maxValue: {config.get('max', 100)}")
                lines.append("        value: dashboard[\"bat_soc\"] || 0")
                lines.append(f"        direction: \"{config.get('direction', 'bottom_up')}\"")

            elif qml_type == "WarningLight":
                active = bindings.get("active", "false")
                flash = bindings.get("flash", "false")
                lines.append(f"        active: {active}")
                lines.append(f"        flash: {flash}")
                lines.append(f"        imageOn: \"qrc:/images/{config.get('image_on', '')}\"")
                lines.append(f"        imageOff: \"qrc:/images/{config.get('image_off', '')}\"")

            elif qml_type == "TextDisplay":
                fmt = config.get("format", "{value}")
                lines.append("        text: dashboard[\"bat_volt\"]")
                lines.append(f"        format: \"{fmt}\"")
                lines.append(f"        fontSize: {config.get('font_size', 24)}")
                lines.append(f"        color: \"{config.get('color', '#FFFFFF')}\"")

            lines.append("    }")
            lines.append("")

    lines.append("}")  # root Item
    return "\n".join(lines)


def main():
    layout_data = load_yaml("display_layout.yaml")
    content = generate_qml(layout_data)

    out_path = UI_DIR / "DashboardMain_generated.qml"
    with open(out_path, "w") as f:
        f.write(content)

    print(f"✓ 生成 {out_path}")
    print("提示: 将 DashboardMain_generated.qml 重命名为 DashboardMain.qml 或在 DashboardMain.qml 中 import")


if __name__ == "__main__":
    main()
