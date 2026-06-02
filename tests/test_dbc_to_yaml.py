#!/usr/bin/env python3
"""
test_dbc_to_yaml.py - DBC ↔ YAML 转换单元测试（PR3）

测试场景：
1. DBC 文件能正确解析（cantools）
2. DBC → YAML 转换能产生 can-dash 项目接受的格式
3. YAML → DBC 反向验证（通过现有 yaml_to_c.py 的字段表）
4. Round-trip: DBC → YAML → C → 信号名集合 应等于 DBC 的信号名集合
"""
import unittest
import sys
import os
import tempfile
import yaml
import cantools
from pathlib import Path

# 路径
PROJECT_ROOT = Path(__file__).parent.parent
DBC_PATH = PROJECT_ROOT / "config" / "candash.dbc"
YAML_PATH = PROJECT_ROOT / "config" / "can_ids.yaml"
TOOLS_DIR = PROJECT_ROOT / "tools"
sys.path.insert(0, str(TOOLS_DIR))

# 让 dbc_to_yaml 可导入
import importlib.util
spec = importlib.util.spec_from_file_location("dbc_to_yaml", TOOLS_DIR / "dbc_to_yaml.py")
dbc_mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dbc_mod)


class TestDbcParsing(unittest.TestCase):
    """测试 1: DBC 文件基本可解析性"""

    def test_dbc_loads(self):
        db = cantools.database.load_file(str(DBC_PATH))
        self.assertGreater(len(db.messages), 0, "DBC 至少要有 1 个 message")
        print(f"  ✓ DBC 加载: {len(db.messages)} messages")

    def test_vehicle_speed_signal_exists(self):
        db = cantools.database.load_file(str(DBC_PATH))
        vcpu = next(m for m in db.messages if m.name == "VCPU_Frame")
        speed = next(s for s in vcpu.signals if s.name == "vehicle_speed")
        self.assertEqual(speed.length, 16)
        self.assertEqual(speed.scale, 0.1)
        self.assertEqual(speed.unit, "km/h")
        print(f"  ✓ vehicle_speed: {speed.length}bit, scale={speed.scale}, unit={speed.unit}")

    def test_motor_rpm_is_signed(self):
        db = cantools.database.load_file(str(DBC_PATH))
        mcu = next(m for m in db.messages if m.name == "MCU_Frame")
        rpm = next(s for s in mcu.signals if s.name == "motor_rpm")
        self.assertTrue(rpm.is_signed)
        self.assertEqual(rpm.length, 16)
        print(f"  ✓ motor_rpm: signed {rpm.length}bit")


class TestDbcToYamlConversion(unittest.TestCase):
    """测试 2: DBC → YAML 转换正确性"""

    def setUp(self):
        with tempfile.NamedTemporaryFile(mode="w", suffix=".yaml", delete=False) as f:
            self.tmp_yaml = f.name
        dbc_mod.dbc_to_yaml(str(DBC_PATH), self.tmp_yaml)

    def tearDown(self):
        os.unlink(self.tmp_yaml)

    def test_generates_yaml_file(self):
        with open(self.tmp_yaml) as f:
            cfg = yaml.safe_load(f)
        self.assertIn("can_sources", cfg)
        self.assertGreater(len(cfg["can_sources"]), 0)
        print(f"  ✓ 生成 {len(cfg['can_sources'])} can_sources")

    def test_vehicle_speed_field_correct(self):
        with open(self.tmp_yaml) as f:
            cfg = yaml.safe_load(f)
        vcpu = next(s for s in cfg["can_sources"] if s["name"] == "VCPU")
        speed = next(f for f in vcpu["fields"] if f["name"] == "vehicle_speed")
        self.assertEqual(speed["bits"], 16)
        self.assertIn("endian", speed)
        self.assertIn("type", speed)
        # formula 应该是 x * 0.1
        self.assertIn("formula", speed)
        self.assertIn("0.1", speed["formula"])
        print(f"  ✓ VCPU.vehicle_speed: bits={speed['bits']}, formula={speed['formula']}")

    def test_can_ids_match(self):
        """YAML 生成的 can_id 集合 ⊇ 项目 can_ids.yaml 的 11-bit IDs"""
        with open(self.tmp_yaml) as f:
            gen = yaml.safe_load(f)
        with open(str(YAML_PATH)) as f:
            orig = yaml.safe_load(f)

        def to_int_set(cfg):
            result = set()
            for s in cfg["can_sources"]:
                cid = s["can_id"]
                if isinstance(cid, int):
                    result.add(cid)
                else:
                    result.add(int(cid, 16) if str(cid).startswith("0x") else int(cid))
            return result

        gen_ids = to_int_set(gen)
        orig_ids = to_int_set(orig)
        # 排除 BMS 的 29-bit extended ID
        non_ext_orig = {i for i in orig_ids if i < 0x800}
        self.assertEqual(gen_ids, non_ext_orig,
                         f"DBC→YAML 缺失 IDs: {non_ext_orig - gen_ids}")
        print(f"  ✓ can_id 集合一致 ({len(gen_ids)} 个)")


class TestDbcYamlValidation(unittest.TestCase):
    """测试 3: DBC ↔ YAML 双向验证"""

    def test_validate_returns_zero(self):
        rc = dbc_mod.validate_dbc_yaml(str(DBC_PATH), str(YAML_PATH))
        self.assertEqual(rc, 0)


class TestCantoolsDecode(unittest.TestCase):
    """测试 4: 用 DBC 解码一个真实 CAN 帧（端到端可用性）"""

    def test_decode_vehicle_speed(self):
        """
        构造一个 VCPU 帧，vehicle_speed=88.5 km/h, brake=50%
        通过 cantools 用 DBC 解码，验证得到的物理值正确

        VCPU_Frame 布局 (little-endian):
          byte 0: reserved (8 bits)
          byte 1: brake (8 bits) = 125 (50 / 0.4)
          byte 2: padding
          byte 3-4: vehicle_speed (16 bits LE) = 885 (88.5 / 0.1)
        """
        db = cantools.database.load_file(str(DBC_PATH))
        vcpu = next(m for m in db.messages if m.name == "VCPU_Frame")

        # 构造 raw 数据
        data = bytearray(8)
        data[1] = 125  # brake raw
        data[3] = 885 & 0xFF           # vehicle_speed low byte
        data[4] = (885 >> 8) & 0xFF    # vehicle_speed high byte

        msg = vcpu.decode(bytes(data))
        self.assertAlmostEqual(msg["vehicle_speed"], 88.5, places=1)
        self.assertAlmostEqual(msg["brake"], 50.0, places=1)
        print(f"  ✓ 解码 vehicle_speed={msg['vehicle_speed']:.1f} km/h, brake={msg['brake']:.1f}%")


if __name__ == "__main__":
    print("=== DBC ↔ YAML 转换测试 ===\n")
    unittest.main(verbosity=2)
