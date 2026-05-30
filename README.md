# CAN-Dash 量产级汽车仪表

通过修改 YAML 配置完成 90% 的后期迭代，C++ 代码基本不需要改动。

## 技术栈

- **UI**: Qt/QML
- **仿真引擎**: Python + python-can + Unix Socket
- **配置驱动**: YAML → C 代码生成（编译时预处理，零开机延迟）
- **架构**: Layer 1(C生成) → Layer 2(纯C++业务) → Layer 3(Qt适配) → Layer 4(QML)

## 快速开始

```bash
# 1. 安装依赖（Ubuntu/Debian）
sudo apt install qt6-base-dev qt6-declarative-dev qt6-svg-dev
pip install pyyaml jsonschema pydantic cantools python-can

# 2. 校验配置 + 生成 C 代码
python tools/validate.py
python tools/yaml_to_c.py

# 3. 编译
make -j$(nproc)

# 4. PC 仿真（两终端）
./can-dash                          # 终端1：启动仪表
python can_sim/engine.py            # 终端2：启动CAN仿真
```

## 目录结构

```
config/              YAML 配置（AI 主要修改位置）
schemas/             JSON Schema（AI 理解 YAML 字段语义）
src/generated/       ⚠️ 由 tools/yaml_to_c.py 自动生成
src/layer2/          纯 C++ 业务逻辑（无 Qt）
src/layer3/          Qt 适配层
src/ui/              QML 界面
tools/               yaml_to_c.py / validate.py
tests/               Layer 2 单元测试
can_sim/             PC 仿真引擎
```

## 核心工作流

| 场景 | 操作 |
|------|------|
| 调整报警阈值 | 改 `config/alarm_rules.yaml` → `validate.py` → `yaml_to_c.py` → `make` |
| 新增报警灯 | 改 `config/alarm_rules.yaml` + `config/indicators.yaml` |
| 新增 CAN 信号 | 改 `config/can_ids.yaml` |
| 改界面布局 | 改 `config/display_layout.yaml` |

详见 [ARCHITECTURE.md](ARCHITECTURE.md)

## AI 开发

AI 请先阅读 `ARCHITECTURE.md`，修改配置前运行 `python tools/validate.py` 校验 YAML。

## 许可证

MIT
