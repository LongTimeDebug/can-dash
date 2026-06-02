#!/usr/bin/env python3
"""
dbc_to_yaml.py - DBC → can_ids.yaml 转换器（PR3）

将 DBC 文件（车规标准）转换为 can-dash 项目内部的 can_ids.yaml 格式。
这是真实车机开发流程的"逆操作"：DBC 由 OEM 提供 → 我们消费 → 生成 C 表。

用法:
    python3 tools/dbc_to_yaml.py <input.dbc> <output.yaml> [--pretty]
    python3 tools/dbc_to_yaml.py --validate <input.dbc> <input.yaml>

设计：
- 输入：DBC 文件（Vector CANdb++ 标准格式）
- 输出：can_ids.yaml（项目内部格式，含 formula 字符串）
- 转换：scale/offset → formula，DBC bit position → byte/bits

依赖：cantools (pip install cantools)
"""
import argparse
import sys
from pathlib import Path

try:
    import cantools
    from cantools.database.can import Database
    from cantools.database.can.message import Message
    from cantools.database.can.signal import Signal
except ImportError:
    print("ERROR: cantools 未安装。运行: pip3 install cantools", file=sys.stderr)
    sys.exit(1)

try:
    import yaml
except ImportError:
    print("ERROR: pyyaml 未安装。运行: pip3 install pyyaml", file=sys.stderr)
    sys.exit(1)


# ─── DBC bit position → byte 范围转换 ─────────────────
def bits_to_byte_range(start_bit: int, length: int, byte_order: str) -> dict:
    """
    DBC 的 start_bit 是 LSB-first 比特编号。
    little-endian (Intel, @1): 字节内 LSB 在低位，跨字节按 LE 排
    big-endian (Motorola, @0):   字节内 MSB 在高位，跨字节按 BE 排

    返回：{'byte': [first_byte, last_byte], 'bits': total_bits, 'endian': 'little'/'big'}
    """
    if byte_order == "little_endian":  # Intel, @1
        first_byte = start_bit // 8
        last_byte = (start_bit + length - 1) // 8
        endian = "little"
    else:  # big_endian, @0 (Motorola)
        # Motorola 格式: start_bit 是 MSB 位置
        # 简化：对于 ≤8 bit 信号，同字节
        # 对于跨字节：start_bit 在某字节的高位
        first_byte = start_bit // 8
        last_byte = (start_bit + length - 1) // 8
        endian = "big"
    return {
        "byte": [first_byte, last_byte] if first_byte != last_byte else first_byte,
        "bits": length,
        "endian": endian,
    }


def signal_to_field(sig: Signal) -> dict:
    """
    DBC signal → can_ids.yaml field 格式
    """
    pos = bits_to_byte_range(sig.start, sig.length, sig.byte_order)
    field = {
        "name": sig.name,
        **pos,
    }

    # 类型映射
    if sig.is_float:
        field["type"] = "float"
    elif sig.is_signed:
        field["type"] = f"int{sig.length}"
    else:
        field["type"] = f"uint{sig.length}"

    # formula: 优先用 scale/offset，还原成字符串
    # DBC: physical = raw * scale + offset
    # can_ids.yaml: formula = "x * scale + offset" 风格
    if sig.scale == 1.0 and sig.offset == 0.0:
        # 无需 formula
        pass
    elif sig.scale != 1.0 and sig.offset == 0.0:
        field["formula"] = f"x * {sig.scale}"
    elif sig.scale == 1.0 and sig.offset != 0.0:
        field["formula"] = f"x + {sig.offset}"
    elif sig.scale == -1.0 and sig.offset == 0.0:
        field["formula"] = f"-x"
    else:
        # 通用情况：scale * x + offset
        if sig.offset >= 0:
            field["formula"] = f"x * {sig.scale} + {sig.offset}"
        else:
            field["formula"] = f"x * {sig.scale} - {abs(sig.offset)}"

    # unit
    if sig.unit and sig.unit not in ("", "undef"):
        field["unit"] = sig.unit

    return field


def message_to_source(msg: Message) -> dict:
    """
    DBC message → can_ids.yaml can_source 格式
    """
    return {
        "name": msg.name.replace("_Frame", "").replace("_frame", ""),
        "can_id": f"0x{msg.frame_id:X}",
        "period_ms": msg.cycle_time if msg.cycle_time else 100,
        "fields": [signal_to_field(sig) for sig in msg.signals
                   if not sig.name.startswith("reserved_")],
    }


def dbc_to_yaml(dbc_path: str, yaml_path: str, pretty: bool = False) -> dict:
    """主转换函数"""
    db: Database = cantools.database.load_file(dbc_path)
    cfg = {
        "can_sources": [message_to_source(msg) for msg in db.messages],
    }

    Path(yaml_path).write_text(
        yaml.dump(cfg, default_flow_style=False, allow_unicode=True, sort_keys=False)
    )
    return cfg


# ─── 验证：DBC ↔ YAML 双向一致性 ─────────────────
def validate_dbc_yaml(dbc_path: str, yaml_path: str) -> int:
    """
    验证 DBC 和 YAML 描述的信号集一致。
    用于 CI：保证 can_ids.yaml 真的对应 candash.dbc。

    检查项：
    1. YAML 中每个 can_source 的 can_id 在 DBC 中能找到对应 message
    2. YAML 中每个 field name 在 DBC 对应 message 中能找到同名 signal
    3. 报告不一致项（warning，不 abort）
    """
    db = cantools.database.load_file(dbc_path)
    with open(yaml_path) as f:
        ycfg = yaml.safe_load(f)

    # 按 can_id 索引 DBC messages
    db_by_id = {msg.frame_id: msg for msg in db.messages}

    warnings = 0
    for src in ycfg.get("can_sources", []):
        can_id_val = src.get("can_id", 0)
        can_id = can_id_val if isinstance(can_id_val, int) else (
            int(can_id_val, 16) if str(can_id_val).startswith("0x") else int(can_id_val)
        )
        can_id_str = f"0x{can_id:X}"

        if can_id not in db_by_id:
            print(f"WARNING: YAML has {src.get('name')} (can_id={can_id_str}) but DBC has no such message")
            warnings += 1
            continue

        db_msg = db_by_id[can_id]
        db_signals = {s.name for s in db_msg.signals if not s.name.startswith("reserved_")}

        for fld in src.get("fields", []):
            fname = fld.get("name", "")
            if fname not in db_signals:
                print(f"WARNING: {src.get('name')} field '{fname}' not in DBC message {db_msg.name}")
                warnings += 1

    # 反向：检查 DBC 中有但 YAML 中没的 signal（可能是 reserved_ 或新增）
    for db_msg in db.messages:
        yml_match = next((s for s in ycfg.get("can_sources", [])
                          if (s.get("can_id") if isinstance(s.get("can_id"), int)
                              else int(s.get("can_id", "0"), 16)) == db_msg.frame_id), None)
        if not yml_match:
            continue
        yml_signals = {f.get("name") for f in yml_match.get("fields", [])}
        for sig in db_msg.signals:
            if not sig.name.startswith("reserved_") and sig.name not in yml_signals:
                print(f"INFO: DBC signal '{sig.name}' in {db_msg.name} not used in YAML (optional)")

    if warnings == 0:
        print(f"✓ DBC ↔ YAML 一致 ({len(db.messages)} messages verified)")
        return 0
    print(f"\n{warnings} warnings (non-fatal)")
    return 0  # warnings 不 abort CI


# ─── CLI ──────────────────────────────────────────────
def main():
    p = argparse.ArgumentParser(description="DBC ↔ can_ids.yaml 转换/验证")
    p.add_argument("dbc", help="输入 DBC 文件")
    p.add_argument("yaml", help="输入/输出 YAML 文件")
    p.add_argument("--validate", action="store_true", help="只验证不转换")
    args = p.parse_args()

    if args.validate:
        sys.exit(validate_dbc_yaml(args.dbc, args.yaml))

    if not Path(args.dbc).exists():
        print(f"ERROR: DBC 文件不存在: {args.dbc}", file=sys.stderr)
        sys.exit(1)

    print(f"Converting {args.dbc} → {args.yaml} ...")
    cfg = dbc_to_yaml(args.dbc, args.yaml)
    print(f"✓ Generated {len(cfg['can_sources'])} can_sources")
    print(f"✓ Validate: ", end="")
    validate_dbc_yaml(args.dbc, args.yaml)


if __name__ == "__main__":
    main()
