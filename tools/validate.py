#!/usr/bin/env python3
"""
YAML 配置校验器
运行时机：yaml_to_c.py 之前必须运行

检查内容：
1. JSON Schema 校验（字段类型/格式/枚举值）
2. 跨文件引用校验（display_key / widget id / can_id 等）

Usage:
    python tools/validate.py
"""

import json
import sys
import re
import yaml
from pathlib import Path
from typing import Any, Dict, List, Optional
from dataclasses import dataclass, field

CONFIG_DIR = Path(__file__).parent.parent / "config"
SCHEMA_DIR = Path(__file__).parent.parent / "schemas"


@dataclass
class ValidationError:
    file: str
    path: str
    message: str
    suggestion: Optional[str] = None


@dataclass
class ValidationResult:
    errors: List[ValidationError] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)

    def add_error(self, file: str, path: str, msg: str, suggestion: str = None):
        self.errors.append(ValidationError(file, path, msg, suggestion))

    def add_warning(self, msg: str):
        self.warnings.append(msg)

    def ok(self) -> bool:
        return len(self.errors) == 0


def load_yaml(name: str) -> Dict:
    path = CONFIG_DIR / name
    if not path.exists():
        return {}
    with open(path) as f:
        return yaml.safe_load(f)


def validate_alarm_rules(result: ValidationResult):
    """校验 alarm_rules.yaml"""
    data = load_yaml("alarm_rules.yaml")
    can_data = load_yaml("can_ids.yaml")
    indicator_data = load_yaml("indicators.yaml")

    # 提取 can_ids.yaml 中定义的字段名
    defined_keys = set()
    for src in can_data.get("can_sources", []):
        for f in src.get("fields", []):
            defined_keys.add(f["name"])

    # 提取 indicators.yaml 中定义的 widget id
    defined_widgets = set()
    for ind in indicator_data.get("indicators", []):
        defined_widgets.add(ind["id"])

    alarm_rules = data.get("alarm_rules", [])
    if not alarm_rules:
        result.add_warning("alarm_rules.yaml: alarm_rules 列表为空")

    for rule in alarm_rules:
        name = rule.get("name", "<unnamed>")

        # display_key 校验
        dk = rule.get("display_key")
        if dk and dk not in defined_keys:
            result.add_error(
                "alarm_rules.yaml",
                f"alarm_rules[{name}].display_key",
                f"display_key='{dk}' 在 can_ids.yaml 中未定义",
                f"可用值: {sorted(defined_keys)}"
            )

        # condition 格式校验
        cond = rule.get("condition", "")
        if not re.match(r"^value\s*(>=|<=|==|!=|>|<)\s*-?[0-9.]+$", cond.strip()):
            result.add_error(
                "alarm_rules.yaml",
                f"alarm_rules[{name}].condition",
                f"condition 格式错误: '{cond}'",
                "正确格式: 'value > 420' 或 'value < 10'"
            )

        # priority 枚举校验
        priority = rule.get("priority")
        if priority not in ("high", "medium", "low"):
            result.add_error(
                "alarm_rules.yaml",
                f"alarm_rules[{name}].priority",
                f"priority='{priority}' 必须是 high/medium/low 之一",
            )

        # actions 校验
        actions = rule.get("actions", [])
        if not actions:
            result.add_error(
                "alarm_rules.yaml",
                f"alarm_rules[{name}].actions",
                f"报警 '{name}' 没有定义任何 actions",
            )

        for j, action in enumerate(actions):
            action_type = action.get("type")
            if action_type not in ("indicator_light", "alarm_text", "alarm_image", "sound"):
                result.add_error(
                    "alarm_rules.yaml",
                    f"alarm_rules[{name}].actions[{j}].type",
                    f"type='{action_type}' 必须是 indicator_light/alarm_text/alarm_image/sound 之一",
                )

            if action_type == "indicator_light":
                widget = action.get("widget")
                if widget and widget not in defined_widgets:
                    result.add_error(
                        "alarm_rules.yaml",
                        f"alarm_rules[{name}].actions[{j}].widget",
                        f"widget='{widget}' 在 indicators.yaml 中未定义",
                        f"可用值: {sorted(defined_widgets)}"
                    )
                if not action.get("flash") and not action.get("widget"):
                    result.add_warning(
                        f"alarm_rules[{name}].actions[{j}]: indicator_light 建议设置 flash"
                    )

            if action_type == "alarm_text":
                if not action.get("text_zh"):
                    result.add_error(
                        "alarm_rules.yaml",
                        f"alarm_rules[{name}].actions[{j}].text_zh",
                        "alarm_text 类型必须设置 text_zh"
                    )
                color = action.get("color", "#FF4400")
                if not re.match(r"^#[0-9A-Fa-f]{6}$", color):
                    result.add_error(
                        "alarm_rules.yaml",
                        f"alarm_rules[{name}].actions[{j}].color",
                        f"color='{color}' 格式错误",
                        "正确格式: #RRGGBB 如 #FF4400"
                    )


def validate_can_ids(result: ValidationResult):
    """校验 can_ids.yaml"""
    data = load_yaml("can_ids.yaml")
    can_sources = data.get("can_sources", [])
    if not can_sources:
        result.add_warning("can_ids.yaml: can_sources 列表为空")

    defined_names = set()
    for src in can_sources:
        name = src.get("name", "<unnamed>")
        can_id = src.get("can_id")

        if not can_id:
            result.add_error("can_ids.yaml", f"can_sources[{name}].can_id", "can_id 未定义")
            continue

        # 检查 can_id 唯一性
        if can_id in defined_names:
            result.add_error("can_ids.yaml", f"can_sources[{name}].can_id",
                           f"can_id 0x{can_id:X} 重复定义")
        defined_names.add(can_id)

        fields = src.get("fields", [])
        field_names = set()
        for f in fields:
            fname = f.get("name")
            if not fname:
                result.add_error("can_ids.yaml", f"can_sources[{name}].fields",
                               "field.name 未定义")
                continue

            if fname in field_names:
                result.add_error("can_ids.yaml", f"can_sources[{name}].fields[{fname}].name",
                               f"字段名 '{fname}' 重复定义")
            field_names.add(fname)

            # byte 范围校验
            byte = f.get("byte")
            if byte is None:
                result.add_error("can_ids.yaml", f"can_sources[{name}].fields[{fname}].byte",
                               "byte 未定义")
            elif isinstance(byte, list):
                if len(byte) != 2 or byte[0] > byte[1] or byte[1] > 7:
                    result.add_error("can_ids.yaml", f"can_sources[{name}].fields[{fname}].byte",
                                   f"byte 范围错误: {byte}，应为 [start, end] 且 end <= 7")
            elif not isinstance(byte, int):
                result.add_error("can_ids.yaml", f"can_sources[{name}].fields[{fname}].byte",
                               f"byte 类型错误: {type(byte).__name__}，应为 int 或 [int,int]")


def validate_seat_belt(result: ValidationResult):
    """校验 seat_belt.yaml"""
    data = load_yaml("seat_belt.yaml")
    can_data = load_yaml("can_ids.yaml")

    defined_can_ids = {src["can_id"] for src in can_data.get("can_sources", [])}

    seat_belt = data.get("seat_belt", {})
    positions = seat_belt.get("positions", [])
    if not positions:
        result.add_warning("seat_belt.yaml: positions 列表为空")

    trigger = seat_belt.get("trigger", {})
    if "speed_threshold" not in trigger:
        result.add_warning("seat_belt.yaml: trigger.speed_threshold 未定义，使用默认值 5.0")

    for pos in positions:
        pid = pos.get("id", "<unnamed>")

        # CAN ID 跨文件引用校验
        can_ids = pos.get("can_ids", {})
        for key, val in can_ids.items():
            if isinstance(val, dict):
                cid = val.get("id", 0)
            else:
                cid = val
            if cid and cid not in defined_can_ids:
                result.add_error(
                    "seat_belt.yaml",
                    f"seat_belt.positions[{pid}].can_ids.{key}",
                    f"CAN ID 0x{cid:X} 在 can_ids.yaml 中未定义"
                )


def validate_indicators(result: ValidationResult):
    """校验 indicators.yaml"""
    data = load_yaml("indicators.yaml")
    indicators = data.get("indicators", [])
    if not indicators:
        result.add_warning("indicators.yaml: indicators 列表为空")

    ids = set()
    for ind in indicators:
        iid = ind.get("id")
        if not iid:
            result.add_error("indicators.yaml", "indicators[?].id", "id 未定义")
            continue
        if iid in ids:
            result.add_error("indicators.yaml", f"indicators[{iid}].id", f"id '{iid}' 重复定义")
        ids.add(iid)


def validate_display_layout(result: ValidationResult):
    """校验 display_layout.yaml"""
    data = load_yaml("display_layout.yaml")
    layout = data.get("display", {})
    if not layout:
        result.add_warning("display_layout.yaml: display 节点为空")

    pages = layout.get("pages", [])
    for page in pages:
        components = page.get("layers", page.get("components", []))
        for comp in components:
            comp_id = comp.get("id")
            if not comp_id:
                result.add_error("display_layout.yaml", "components[?].id", "component id 未定义")


def validate_cross_references(result: ValidationResult):
    """跨文件引用校验"""
    alarm_data = load_yaml("alarm_rules.yaml")
    indicator_data = load_yaml("indicators.yaml")

    defined_widgets = {ind["id"] for ind in indicator_data.get("indicators", [])}
    used_widgets = set()

    for rule in alarm_data.get("alarm_rules", []):
        for action in rule.get("actions", []):
            if action.get("type") == "indicator_light":
                w = action.get("widget")
                if w:
                    used_widgets.add(w)

    undefined = used_widgets - defined_widgets
    for w in undefined:
        result.add_error(
            "alarm_rules.yaml",
            f"actions.widget='{w}'",
            f"widget '{w}' 在 alarm_rules 中使用但未在 indicators.yaml 中定义",
            f"请在 indicators.yaml 中添加 id: '{w}' 的定义"
        )


def validate_all() -> ValidationResult:
    result = ValidationResult()
    validate_alarm_rules(result)
    validate_can_ids(result)
    validate_seat_belt(result)
    validate_indicators(result)
    validate_display_layout(result)
    validate_cross_references(result)
    return result


def main():
    print("=" * 60)
    print("CAN-Dash YAML 配置校验")
    print("=" * 60)

    result = validate_all()

    # 打印警告
    for w in result.warnings:
        print(f"  ⚠️  {w}")

    # 打印错误
    if result.errors:
        print(f"\n❌ 校验失败，共 {len(result.errors)} 个错误:\n")
        for err in result.errors:
            print(f"  📄 {err.file}")
            print(f"     路径: {err.path}")
            print(f"     错误: {err.message}")
            if err.suggestion:
                print(f"     建议: {err.suggestion}")
            print()

        print("请修复上述错误后重新运行 validate.py")
        print("确认无误后运行: python tools/yaml_to_c.py")
        sys.exit(1)
    else:
        print(f"\n✅ 校验通过（{len(result.warnings)} 个警告）")
        print("下一步: python tools/yaml_to_c.py")
        sys.exit(0)


if __name__ == "__main__":
    main()
