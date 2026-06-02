# DBC 集成（PR3）

## 概述

can-dash 支持 DBC（CAN database）文件作为 CAN 信号定义的 **车规标准输入**。
DBC 是 Vector CANdb++ / CANoe 等行业工具的原生格式，OEM 一般以 DBC 提供信号定义。

## 工作流

```
OEM DBC 文件 (config/candash.dbc)
    ↓ tools/dbc_to_yaml.py --validate
can_ids.yaml (项目内部格式)
    ↓ tools/yaml_to_c.py
src/generated/can_field_table.cpp (C 表，编译进 can-processor)
    ↓ can-processor 启动
DisplayData (运行时)
```

## 文件

| 文件 | 作用 |
|------|------|
| `config/candash.dbc` | 标准 DBC 文件 (18 个 message, ~30 signals) |
| `tools/dbc_to_yaml.py` | DBC ↔ YAML 转换 / 验证工具 |
| `tests/test_dbc_to_yaml.py` | 8 个单元测试，覆盖解析/转换/解码 |

## 用法

### 验证 DBC ↔ YAML 一致性
```bash
python3 tools/dbc_to_yaml.py --validate config/candash.dbc config/can_ids.yaml
```

### 从 OEM DBC 生成 can_ids.yaml
```bash
python3 tools/dbc_to_yaml.py path/to/oem.dbc config/can_ids.yaml
# 然后运行: python3 tools/yaml_to_c.py
```

### 解码真实 CAN 帧（端到端）
```python
import cantools
db = cantools.database.load_file("config/candash.dbc")
vcpu = db.get_message_by_name("VCPU_Frame")
raw = bytes([0x00, 125, 0x00, 0x75, 0x03, 0, 0, 0])
decoded = vcpu.decode(raw)
print(decoded)  # {'vehicle_speed': 88.5, 'brake': 50.0, ...}
```

## 已知限制

1. **29-bit 扩展 ID**: DBC 标准格式（Vector CANdb++）下，cantools 解析器不支持
   超过 11-bit 的 CAN ID。原 `can_ids.yaml` 中的 `0x186040F3` (BMS_Frame) 是扩展 ID，
   在 `candash.dbc` 中以注释形式记录，实际信号定义分散到其他 message。
   真实车机的 DBC 通常也是这种约定。

2. **Motorola (big-endian) 字节序**: 转换器目前仅完整支持 little-endian (Intel)
   格式的 signal。Motorola 格式的 start_bit 处理是简化的。生产用 DBC 通常混合两种
   格式，本项目目前没有 Motorola signal 需求。

3. **VAL_TABLE 枚举映射**: DBC 的 `VAL_` 定义（如 `energy_mode 0 "EV" 1 "HYBRID"`）
   目前在转换过程中保留为注释，未自动生成 QML 显示用的字符串映射。
   后续可在 DBC parser 中扩展。

## CI 集成

`.github/workflows/ci.yml` 新增 `validate-dbc` job：
- 解析 candash.dbc
- 运行 8 个 DBC 单元测试
- 验证 DBC ↔ YAML 信号集一致

不通过会 fail PR（基于 validate_dbc_yaml 的返回值）。
