# CAN-Dash 多智能体架构设计

## 1. 概述

CAN-Dash 多智能体系统将汽车仪表盘开发拆分为多个专业角色，通过 Hermes Agent Profile 实现隔离执行、任务路由和并行开发。

**设计目标**：
- 需求 → 调研 → 文档 → 实现 → 发布 全链路可追踪
- 专业 Agent 做专业事，减少上下文切换
- 架构一致性由事前审核保证，而非事后补救

**参与角色**：

| 角色 | 类型 | 运行方式 |
|------|------|----------|
| default（协调者） | 协调者 | 本会话 |
| requirements-agent | 调研员 | 独立会话 |
| architect-agent | 架构师 | 独立会话 |
| config-agent | 配置工程师 | 独立会话 |
| ui-agent | UI 工程师 | 独立会话 |
| backend-agent | C++ 工程师 | 独立会话 |
| release-agent | 发布工程师 | 独立会话 |

---

## 2. Agent 职责定义

### 2.1 default（协调者）

**职责**：只做协调，不做实现。
- 接收用户自然语言需求
- 决定分发给哪个 Agent
- 追踪进度（`hermes kanban ls`）
- 审核其他 Agent 的交付
- 更新 ARCHITECTURE.md

**不做**：直接写代码 / YAML / QML / 直接运行 cmake

### 2.2 requirements-agent（需求调研）

**职责**：
- 接收自然语言需求，使用 MiniMax MCP web search 调研行业标准
- 生成 SRS 格式需求文档（`docs/requirements/REQ-*.md`）
- 维护需求总索引（`docs/requirements/INDEX.md`）

**交付物**：
- `docs/requirements/REQ-<CATEGORY>-<NNN>.md`
- `docs/requirements/INDEX.md` 更新

**边界**：调研完成后交付，不负责向其他 Agent 分发任务。

### 2.3 architect-agent（架构审计）

**职责**：
- **事前审核**：对涉及架构重大变更的实施任务，先审核再实施
- **事后审计**：扫描 ARCHITECTURE.md / references/ 与代码一致性
- **维护 ADR**：架构决策记录（references/adr/*.md）

**事前审核触发条件**（4类）：
| 变更类型 | 举例 |
|----------|------|
| 新增进程 / 改变进程边界 | 在 can-dash 和 can-processor 之间移动代码 |
| 修改 Layer 依赖关系 | Layer N → Layer N+1 反向依赖 |
| 修改 IPC 机制 | 共享内存路径、Unix Socket 路径 |
| 修改 display_key | can_ids.yaml / alarm_rules.yaml 中新增或修改 display_key |

**审核结论**：给出「可以实施」或「需要修改」，结论写在 kanban 评论中。

### 2.4 config-agent（YAML 配置）

**职责**：修改 YAML 配置文件，生成 C 代码。

**修改范围**：
- `config/alarm_rules.yaml`
- `config/can_ids.yaml`
- `config/indicators.yaml`
- `config/display_layout.yaml`
- `config/can_signal_status.yaml`

**交付物**：修改后的 YAML + 生成的 `src/generated/` C 代码

**事前审核**：display_key 新增/修改必须先报 architect-agent 审核。

### 2.5 ui-agent（QML 界面）

**职责**：开发和修改 QML 界面组件。

**修改范围**：`src/ui/*.qml`

**交付物**：修改后的 QML 文件

**事前审核**：修改 DashboardBackend displayData 绑定方式 / 新增窗口必须先报 architect-agent 审核。

### 2.6 backend-agent（C++ Runtime）

**职责**：Layer 2 Runtime + can_converter + EventBus + 共享内存操作。

**修改范围**：
- `src/layer2/*.cpp`
- `src/layer1/display_data.h`
- `can-processor/main.cpp`

**交付物**：修改后的 C++ 文件 + 单元测试通过

**事前审核**：新增进程 / 修改 DisplayData 结构 / 修改共享内存路径必须先报 architect-agent 审核。

### 2.7 release-agent（版本发布）

**职责**：版本决策 → Git Tag → GitHub Release。

**发布前检查**：
1. `hermes kanban ls` — 无 running/todo 状态任务
2. `git status` — 工作区干净
3. 距上次 commit 超过 3 天（由 release-check cron 触发）

**发布流程**（见 `~/.hermes/profiles/release-agent/skills/software-development/github-release-agent/SKILL.md`）：
1. 确认版本号（参考版本决策规则）
2. 生成 Changelog（从 git log + kanban done 任务提取）
3. `git tag -a v<x.y.z> && git push origin <tag>`
4. `gh release create` — 含 changelog
5. 验证 `gh release view <tag>`

**CI 说明**：Qt 模块在 GitHub Actions 环境中会报错，CI 结果仅供参考，不影响发布决策。

---

## 3. 工作流

### 3.1 需求 → 实现完整流程

```
用户提出需求（自然语言）
    │
    ▼  kanban task 分配给 requirements-agent
requirements-agent
    │  mcp_token_plan_web_search（MiniMax）调研
    │  生成 REQ-*.md（SRS 格式）
    │  更新 INDEX.md（owner 列标记 pending）
    │  kanban_complete 交付
    │
    ├──→ architect-agent（事后审计，异步）
    │
    ▼  cron job 读取 INDEX.md，分发实现任务（每30分钟）
    │
    ├──→ config-agent（YAML 配置类需求）
    │      │
    │      ├──→ architect-agent（事前审核：display_key 变更）
    │      │
    │      ▼  kanban_complete
    │
    ├──→ ui-agent（QML 界面类需求）
    │      │
    │      ├──→ architect-agent（事前审核：displayData 绑定变更）
    │      │
    │      ▼  kanban_complete
    │
    └──→ backend-agent（C++ / Layer 2 类需求）
           │
           ├──→ architect-agent（事前审核：进程边界 / IPC / DisplayData）
           │
           ▼  kanban_complete

（每2小时检查一次）
    │
    ▼  release-check cron 发现发布时机成熟
release-agent
    │  版本决策 + Git Tag + gh release create
    ▼  发布完成
```

### 3.2 架构变更事前审核流程

```
Agent 收到任务 → 判定是否涉及4类变更
    │
    ├── 否 → 直接实施 → kanban_complete
    │
    └── 是 → 发 kanban task 给 architect-agent（标注 pre-review）
             │
             ▼ architect-agent 审核（检查清单 A/B/C）
             │
             ├──「可以实施」→ 评论告知 Agent → Agent 实施 → kanban_complete
             │
             └──「需要修改」→ 评论说明原因 → Agent 修改需求描述 → 重新审核
```

架构检查清单：

```bash
# A. 进程边界检查
grep -rn "can_converter\|alarm_runtime\|indicator_runtime" src/main.cpp
# 结果应为空 → can-dash 进程不含 Layer 2

# B. Layer 2 无 Qt 检查
grep -rn "include.*Qt\|#include.*<Q" src/layer2/
# 结果应为空

# C. 文档与代码一致性
# ARCHITECTURE.md 描述的进程数 = 实际编译入口数
```

---

## 4. 自动化调度

### 4.1 Kanban 依赖链升级（cron: 30分钟）

**脚本**：`~/.hermes/scripts/kanban-dispatch-cron.sh`

**逻辑**：
```
检查 t_94f64b89 状态
    └── 如果是 todo 且 parent（t_bdab833d + t_d82ddd46）均为 done
            └── hermes kanban start t_94f64b89（升级为 ready）
                   └── dispatcher 自动 pickup → 分配给 default（协调者）
                          └── 协调者读取 INDEX.md 分发实现任务
```

### 4.2 发布时机检查（cron: 每2小时）

**脚本**：`~/.hermes/scripts/release-check-cron.sh`

**触发条件（全部满足）**：
- kanban 上无 running/todo 状态任务
- `git status` 工作区干净
- 距上次 commit ≥ 3 天

**动作**：自动创建 kanban task 分配给 release-agent，body 包含发布检查结果。

---

## 5. Kanban 看板结构

### 5.1 任务类型

| 前缀 | 类型 | 归属 |
|------|------|------|
| REQ-* | 需求调研 | requirements-agent |
| ARCH-* | 架构审计 | architect-agent |
| CFG-* | YAML 配置 | config-agent |
| UI-* | QML 界面 | ui-agent |
| BE-* | C++ 后端 | backend-agent |
| REL-* | 版本发布 | release-agent |
| DISPATCH-* | 分发协调 | default |

### 5.2 生命周期

```
todo ──→ running ──→ done ──→ kanban_complete
              ↑
         （parent done 后升级）
```

**kanban_complete 调用方式**：
```bash
hermes kanban complete <id> --summary "修复了 can_converter.cpp DisplayData 写入"
```

### 5.3 依赖关系

`DISPATCH-01`（t_94f64b89）依赖：
- `REQ-01`（t_bdab833d）— 混动调研
- `REQ-02`（t_d82ddd46）— 完整需求基线

两者 done → `DISPATCH-01` 自动/手动升级 → 读取 INDEX.md → 分发实现任务。

---

## 6. 质量保证

### 6.1 事前审核防线上移

architect-agent 的事前审核将重大架构问题拦截在实施前，避免事后返工。

| 问题类型 | 拦截时机 | 未拦截后果 |
|----------|----------|------------|
| display_key 不一致 | 事前审核 | QML 显示数据全错 |
| 进程边界模糊 | 事前审核 | 编译失败或数据流断裂 |
| Layer 反向依赖 | 事后审计 | Qt 依赖进入 Layer 2 |
| IPC 路径变更 | 事前审核 | 双进程通信中断 |

### 6.2 交付审核

协调者（default）收到 `kanban_complete` 时检查：
1. `hermes kanban show <id>` — 确认 summary 内容
2. 是否调用了 `python tools/validate.py`（YAML 变更）
3. `kanban_complete` 是否已调用
4. 询问用户是否满意

### 6.3 CI 策略

- **Qt 构建 CI**（`.github/workflows/ci.yml`）：仅供参考，Qt 模块在 GitHub Actions 环境报错不代表代码有问题
- **单元测试 CI**：必须通过，`ctest --output-on-failure`
- **YAML 验证 CI**：必须通过，`python tools/validate.py`

---

## 7. 独立会话入口

| Agent | 启动命令 |
|-------|----------|
| 需求调研 | `requirements-agent chat` |
| 架构审计 | `architect-agent chat` |
| 配置修改 | `config-agent chat` |
| UI 开发 | `ui-agent chat` |
| C++ 后端 | `backend-agent chat` |
| 版本发布 | `release-agent chat` |

所有 Agent 共享同一份代码（`/home/cjl/can-dash`），通过 kanban 通信。

---

## 8. Cron 任务一览

| 任务名 | 频率 | 脚本 | 作用 |
|--------|------|------|------|
| `kanban-dispatch-upgrade` | 30分钟 | `kanban-dispatch-cron.sh` | 升级依赖链，分发实现任务 |
| `release-check` | 2小时 | `release-check-cron.sh` | 检查发布时机并通知 release-agent |

---

## 9. 当前运行状态

```
hermes kanban assignees
NAME                  ON DISK   COUNTS
architect-agent       yes       done=1
backend-agent        yes       running=3
config-agent          yes       running=3
default               yes       (idle, coordinator)
requirements-agent    yes       running=2
ui-agent              yes       (idle)
release-agent         yes       (idle)

hermes cron list
NAME                    SCHEDULE   NEXT RUN
kanban-dispatch-upgrade  30m       ~23:05
release-check            2h        ~00:35
```

---

## 10. 版本历史

| 日期 | 版本 | 变更内容 |
|------|------|----------|
| 2026-06-01 | v0.2 | 初始版本：7 Agent + 2 cron + 事前审核机制 |
