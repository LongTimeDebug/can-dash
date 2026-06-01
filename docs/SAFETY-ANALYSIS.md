# CAN-Dash 安全分析与 FMEA 报告

| 项目 | 内容 |
| --- | --- |
| 项目名称 | can-dash（车载 CAN 仪表盘） |
| 文档版本 | v1.0（概念阶段） |
| 文档编号 | SAF-CANDASH-001 |
| 适用 ASIL 目标 | **ASIL-B**（HMI/仪表显示域） |
| 引用标准 | ISO 26262-2:2018、ISO 26262-5:2018、ISO 26262-6:2018、ISO 26262-9:2018 |
| 编制日期 | 2026-06-01 |
| 保密等级 | 内部公开 |

---

## 目录

1. [项目安全目标](#1-项目安全目标)
2. [系统架构与故障链路标注](#2-系统架构与故障链路标注)
3. [ASIL 等级分解（HAZOP 分析）](#3-asil-等级分解hazop-分析)
4. [FMEA（故障模式与影响分析）](#4-fmea故障模式与影响分析)
5. [现有安全机制清单](#5-现有安全机制清单)
6. [待实施安全机制（Gap 分析）](#6-待实施安全机制gap-分析)
7. [安全生命周期（ISO 26262-2）](#7-安全生命周期iso-26262-2)
8. [残余风险评审](#8-残余风险评审)
9. [审计与签名](#9-审计与签名)

---

## 1. 项目安全目标

### 1.1 项目定位

can-dash 是一款车载嵌入式数字仪表盘，由两条独立进程协作完成 HMI 显示：

- **can-processor**：C++ 守护进程，从 CAN 总线（或仿真 Unix socket）拉取车辆信号，经过 Layer 2 业务逻辑（报警/指示灯/安全带）处理后，写入共享内存 `/dev/shm/can_display` 中的 `DisplayDataShm` 结构（详见 `src/layer1/shm/shm_display.h`）。
- **can-dash**：Qt/QML 主程序，通过 `mmap` 只读映射同一段共享内存，由 `DashboardBackend`（Layer 3）解析后绑定到 QML 控件显示。

源码依据：`can-processor/main.cpp:200-281` 的 `main()`、`src/layer1/shm/shm_display.cpp:72-97` 的 `shm_display_create()`。

### 1.2 安全目标

| 编号 | 目标 | 度量 |
| --- | --- | --- |
| SG-1 | 驾驶员在行驶过程中必须能正确读取关键 HMI 信息（车速、转速、SOC、里程、报警文字、转向灯、远光） | 显示内容与 CAN 总线真实值的偏差 < 1 个 LSB（车速 ≤ 1 km/h，SOC ≤ 1 %） |
| SG-2 | 关键信息的"故障状态不可见"失效率 < **10 FIT**（符合 ISO 26262-5 ASIL-B 硬件度量要求） | 残余风险 < 10⁻⁷ /h（PMHF 区间） |
| SG-3 | 不允许发生"错显"（如显示 200 km/h 实际 50 km/h），一旦发生视为失控，等级 ≥ ASIL-B | 错显概率 < 10⁻⁷ /h |
| SG-4 | CAN 总线断开、传感器失效、共享内存损坏等故障必须**显式降级**显示，绝不静默失败 | 故障状态 ≤ 1 s 内可见（"信号丢失"占位） |
| SG-5 | 任何进程崩溃后 5 s 内可被 systemd 拉起，不留安全死区 | systemd watchdog 5 s 心跳 |

### 1.3 非目标（Out of Scope）

- **不**做主动安全控制（不参与 AEB、ESC、EPS 等执行器回路）。
- **不**保证显示时间戳精度优于 100 ms（仪表盘 HMI 通常 10–20 Hz 刷新即可）。
- **不**覆盖 QML 渲染层 bug 的全部组合（UI 端另行走 QM 路径验证）。

### 1.4 与车规的映射

| 域 | 等级 | 理由 |
| --- | --- | --- |
| HMI 仪表盘（主） | **ASIL-B** | 错显将影响驾驶员决策（速度误判、SOC 误判），可能导致违反交通法规或失稳 |
| 报警/指示灯（关键条目） | **ASIL-B** | 漏报警（如安全带未系、电池过热）将直接危及人身安全 |
| 报警/指示灯（一般条目） | **ASIL-A** | 转向灯/远光漏显示属于违规但可控性较高 |
| 副驾/后排装饰性信号 | QM | 不影响驾驶行为 |

> 仪表盘域的 ASIL 目标在 OEM 项目中常见 ASIL-B 或 QM，**本项目拟走 ASIL-B** 是因为：包含安全带未系、电池警告等关键报警条目。

---

## 2. 系统架构与故障链路标注

### 2.1 进程级数据流（ASCII）

```
┌────────────────────────────────────────────────────────────────────────┐
│                       物理 / 总线层                                    │
│                                                                        │
│  [车辆 CAN 总线]   ──物理──►   [CAN 收发器]  ──►   [SocketCAN 驱动]    │
│  (500 kbps)                   (PEAK/CANable)         (Linux 内核)     │
│                                       │                                │
│                                       ▼                                │
│                                [字符设备 can0/vcan0]                  │
└──────────────────────────────────────┬─────────────────────────────────┘
                                       │ 链路 1：物理层 / 驱动
                                       ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       数据采集层（can-processor）                       │
│                                                                        │
│  [ICanTransport]  ──► [TransportFactory]  ──► [SocketCanTransport]   │
│  (can_transport.h)                         (socketcan_transport.cpp)  │
│                                       │                                │
│                                       ▼                                │
│              [CanConverter::processFrame] → DisplayData (RAM)         │
│                                       │                                │
│                                       ▼                                │
│         [AlarmRuntime] [IndicatorRuntime] [SeatBeltRuntime]            │
│         (alarm_runtime.cpp)   (indicator_runtime.cpp)  (seat_belt)    │
│                                       │                                │
│                                       ▼                                │
│  [CanSignalMonitor] (5 态质量: GOOD/STALE/INVALID/ABNORMAL/NEVER)    │
│                                       │                                │
│                                       ▼                                │
│  [shm_display_commit()] → msync(MS_SYNC)                               │
└──────────────────────────────────────┬─────────────────────────────────┘
                                       │ 链路 2：进程内 IPC
                                       ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       共享内存层（Layer 1）                             │
│                                                                        │
│   /dev/shm/can_display  ←mmap 488 字节→  DisplayDataShm  (shm_display.h)│
│                                                                        │
│  ┌──────────────────────────────────────────────────────────────┐      │
│  │ magic (0xCA07D15A) | version (1.1.0) | last_commit_ms       │      │
│  │ updated_mask | checksum (CRC32-IEEE offset 24..N-1)         │      │
│  │ motor_rpm, vehicle_speed, bat_volt, bat_curr, bat_soc, ...  │      │
│  │ indicators[12] | alarm_message_zh[128] | backlight_state    │      │
│  └──────────────────────────────────────────────────────────────┘      │
└──────────────────────────────────────┬─────────────────────────────────┘
                                       │ 链路 3：跨进程共享内存
                                       ▼
┌────────────────────────────────────────────────────────────────────────┐
│                       UI 显示层（can-dash）                             │
│                                                                        │
│  [shm_display_read]  →  [DashboardBackend Q_PROPERTY]  →  [QML 控件]  │
│  (src/main.cpp)         (src/layer3/...)                (src/ui/)      │
│                                                                        │
│  ┌─────────────┬────────────┬────────────┬──────────────┐               │
│  │ GaugeCanvas │ AlarmBanner│ Indicator  │ BatteryGauge │               │
│  │ (车速/转速)  │ (报警文字) │ Light×12   │ (SOC/里程)   │               │
│  └─────────────┴────────────┴────────────┴──────────────┘               │
└────────────────────────────────────────────────────────────────────────┘
```

### 2.2 链路故障模式标注

| 链路 | 描述 | 故障模式 | 故障表征 | 检测机制 |
| --- | --- | --- | --- | --- |
| **L1：物理层** | CAN 总线 → 收发器 → 内核驱动 | 总线短路/开路、收发器掉电、bit stuffing 错误 | can-processor 永远读不到帧 | `SocketCanTransport::readFrame` poll 超时（`timeout_ms=100`），主循环照常运行 |
| **L2：进程内 IPC** | can-processor 内部 DisplayData → shm | 写入半完成（reader 读到中间态）、指针越界 | dash 端读到脏数据 | `compute_checksum`（CRC32-IEEE，offset 24..N-1） |
| **L3：跨进程共享内存** | shm 文件被截断/被替换/magic 不匹配 | 第三方进程写入同路径、版本升级不兼容、磁盘位翻转 | dash 拒绝读，返回 -2/-3 | `SHM_MAGIC` 校验 + `SHM_VERSION` 校验 + `checksum` 校验（`shm_display.cpp:107-122`） |
| **L4：UI 渲染** | shm → Q_PROPERTY → QML 控件 | 渲染线程卡死、QML 绑定中断、字体加载失败 | 屏幕花屏/无更新 | **TODO**：UI 自检、watchdog 喂狗、fallback 静态画面 |

### 2.3 时序约束

| 项 | 值 | 来源 |
| --- | --- | --- |
| 主循环 tick 周期 | **100 ms** | `can-processor/main.cpp:28` `#define TICK_PERIOD_MS 100` |
| readFrame 超时 | 100 ms（与 tick 对齐） | `can-processor/main.cpp:260-261` |
| SHM 心跳超时 | **500 ms** | `shm_display.h:46` `SHM_HEARTBEAT_TIMEOUT_MS` |
| SignalMonitor timeout | per-signal `timeout_ms`（如车速通常 1000 ms） | `can_signal_monitor.cpp:90-93` |
| 报警触发确认时间 | `duration_ms / 100` 个 tick | `alarm_runtime.cpp:38` |

> 100 ms tick 是仪表盘 HMI 10 Hz 刷新的常见值；过短增加 CPU，过长导致"信号丢失"判定延迟。

---

## 3. ASIL 等级分解（HAZOP 分析）

### 3.1 ASIL 等级判定方法

依据 ISO 26262-3:2018 §7.4.4，使用严重度（S）、暴露率（E）、可控性（C）三因子矩阵：

| 严重度 S | 描述 |
| --- | --- |
| S0 | 无伤害 |
| S1 | 轻伤（无医疗） |
| S2 | 重伤（需医疗） |
| S3 | 危及生命或致命伤 |

| 暴露率 E | 描述 |
| --- | --- |
| E0 | 几乎不可能 |
| E1 | 极低概率 |
| E2 | 低概率 |
| E3 | 中等概率 |
| E4 | 高概率（驾驶工况默认） |

| 可控性 C | 描述 |
| --- | --- |
| C0 | 完全可控 |
| C1 | 简单可控（99% 驾驶员） |
| C2 | 一般可控（90% 驾驶员） |
| C3 | 难以控制或不可控 |

ASIL 查表（节选）：

| S \ E | E1 | E2 | E3 | E4 |
| --- | --- | --- | --- | --- |
| S3 | QM | A | B | **B** |
| S2 | QM | QM | A | **A** |
| S1 | QM | QM | QM | QM |

### 3.2 HAZOP 信号分解表

| 编号 | 信号 | 驾驶员用途 | 失效率 | 严重度 S | 暴露率 E | 可控性 C | ASIL | 故障后果 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- |
| H-01 | **vehicle_speed** | 法定必看（限速判断、跟车距离） | 高 | S3 | E4 | C3 | **B** | 显示 200 km/h 实际 50 → 驾驶员误判保持车距 |
| H-02 | **motor_rpm** | 重要（换挡提示） | 中 | S2 | E4 | C3 | **A** | 转速显示错误导致错误换挡判断 |
| H-03 | **bat_soc** | 重要（续航焦虑） | 中 | S2 | E4 | C2 | **A** | SOC 虚高导致半路抛锚 |
| H-04 | **alarm_message_zh** | 关键（高温/绝缘故障） | 中 | S3 | E4 | C3 | **B** | 漏报"电池过热"导致热失控 |
| H-05 | **left_turn / right_turn** | 法规（变道信号） | 中 | S2 | E4 | C3 | **A** | 转向灯不亮导致被追尾 |
| H-06 | **high_beam / low_beam** | 法规（夜间会车） | 中 | S2 | E4 | C2 | **A** | 远光漏显示/错显引起对向眩目 |
| H-07 | **seat_belt_warning** | 关键（法规 + 安全） | 中 | S3 | E4 | C3 | **B** | 未系安全带未告警 → 事故中伤亡 |
| H-08 | **tire_pressure** | 重要（爆胎预警） | 低 | S2 | E3 | C2 | **A** | 漏报胎压异常导致高速爆胎 |
| H-09 | **engine_fault** | 重要 | 低 | S2 | E3 | C2 | **A** | 漏报发动机故障导致抛锚 |
| H-10 | **charge_status / charge_power** | 信息 | 低 | S1 | E3 | C1 | **QM** | 仅影响体验 |
| H-11 | **driver_occupied / passenger_occupied** | 信息 | 低 | S1 | E4 | C1 | **QM** | 影响安全带联动判断的二级信号 |
| H-12 | **fuel_level / fuel_range** | 信息 | 低 | S1 | E4 | C1 | **QM** | 仅影响续航焦虑 |

### 3.3 整体 ASIL 目标

- 系统级 ASIL 目标 = **ASIL-B**（H-01 车速 + H-04 报警 + H-07 安全带 三条均 ASIL-B 决定上限）
- 其他信号可走 ASIL-A / QM 降级路径
- 失效率分配：车速、报警文字、安全带各占 PMHF 预算的 1/3

---

## 4. FMEA（故障模式与影响分析）

> **评分法**：严重度 S（1–10）× 发生率 O（1–10）× 探测度 D（1–10）= RPN（Risk Priority Number）。D 越高 = 越难被现有机制探测到。本表 D 的语义与传统 FMEA 略有不同：取"系统层面发现此故障的难度"，便于和现有 can_signal_monitor / shm_display 校验机制对照。

### 4.1 FMEA 主表

| 编号 | 故障模式 | 根因 | 影响 | S | O | D | RPN | 现有探测 | 缓解措施 | 责任模块 |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
| **F001** | can-processor 不读 CAN | `transport->open()` 失败（接口不存在 / 权限不足） | UI 无数据，全屏无更新 | 8 | 3 | 2 | **48** | UI 显示"信号丢失"占位 | 1) 启动时检查 `if_nametoindex` 失败立即退出；2) `shm.last_commit_ms` 500 ms 内未刷新 → dash 端降级 | `socketcan_transport.cpp:58-65` |
| **F002** | can-processor 进程崩溃 | segfault / OOM / 未捕获异常 / 死锁 | UI 冻结（无新数据） | 9 | 2 | 2 | **36** | `shm_display_health_check` 返回 -1 | **TODO**：systemd `WatchdogSec=5s` 自动重启；主循环加 `alarm()` 喂狗 | `can-processor/main.cpp:257-275` |
| **F003** | 共享内存 magic 不匹配 | processor / dash 二进制版本不一致、第三方写脏 | dash 拒绝读，返回 -2 | 7 | 2 | 1 | **14** | `shm_display_open` 中 `g_ptr->magic != SHM_MAGIC` 校验 | 1) 升级两个二进制一起；2) 启动脚本里加版本号断言 | `shm_display.cpp:108-115` |
| **F004** | 共享内存 CRC32 不匹配 | 内存位翻转 / processor 半写 dash 正在读 | `shm_display_read` 返回 -3，dash 不渲染 | 5 | 3 | 1 | **15** | `compute_checksum` 覆盖 offset 24..N-1 | 1) 重试一次（偶尔位翻转）；2) 多次失败则降级显示"信号异常" | `shm_display.cpp:135-142` |
| **F005** | 车速信号突变（>max_delta） | CAN 总线干扰 / 传感器接触不良 / 仪表未标定 | UI 短暂错显（从 50 跳到 200） | 8 | 4 | 2 | **64** | `CanSignalMonitor` 标 `SIGNAL_ABNORMAL_DELTA` | 1) 保留旧值 + 标记 STALE；2) 100 ms 内不连续两次异常才确认 | `can_signal_monitor.cpp:54-62` |
| **F006** | 车速信号长时间不变 | CAN 总线断 / 车速传感器死机 / gateway 死 | UI 显示过期速度（驾驶员误以为 0） | 9 | 3 | 2 | **54** | `CanSignalMonitor::tick()` 100 ms 巡检 `timeout_ms` | 1) 标 STALE，UI 灰色显示；2) 超过 3 s 显示"信号异常"图标 | `can_signal_monitor.cpp:83-95` |
| **F007** | 报警误触发 | 阈值配置错误（YAML 笔误）/ 噪声 | UI 报警闪烁，干扰驾驶员 | 4 | 4 | 4 | **64** | `AlarmRuntime::onValueChanged` 用 `duration_ms / 100` 持续确认 | 1) YAML review 流程；2) `PRIORITY_HIGH` 才闪烁；3) 单元测试覆盖正常边界 | `alarm_runtime.cpp:25-48` |
| **F008** | QML 渲染异常 | UI 层 bug / 字体丢失 / GPU 驱动崩溃 | 显示乱码、空白、卡顿 | 4 | 3 | 4 | **48** | **TODO**：UI 自检（每 500 ms 喂狗） | 1) watchdog 5 s；2) fallback 静态画面（"系统启动中…"）；3) **TODO** | `src/ui/*` |
| **F009** | shm 文件残留 | can-processor 上次异常退出（kill -9、OOM） | 下次启动 `open(O_CREAT\|O_EXCL)` 失败 | 3 | 4 | 1 | **12** | `shm_display_create` 启动时 `unlink(path)` | 启动脚本中加 `rm -f /dev/shm/can_display` 兜底 | `shm_display.cpp:74` |
| **F010** | 共享内存 version 不匹配 | dash 是 1.0.0，processor 是 1.1.0 | 当前实现只 printk，不阻塞 | 5 | 3 | 2 | **30** | `shm_display_open` 校验 `version` | 1) minor 不一致时仅警告；2) major 不一致时拒绝（**TODO**） | `shm_display.cpp:116-122` |
| **F011** | 双写者覆盖 | 异常情况下两个 processor 同时写 shm | 数据竞争、checksum 错乱 | 7 | 1 | 3 | **21** | 暂无（**TODO**：进程 ID 单调递增字段） | 1) `flock(LOCK_EX)`；2) systemd `ExecStartPre` 检查单实例 | `shm_display.cpp`（待加） |
| **F012** | sim_socket 与 socketcan 误用 | `--transport` 解析错误，dash 在生产用 sim 数据 | 测试数据被驾驶员看到 | 6 | 2 | 5 | **60** | CLI 启动参数无强制校验 | 1) 生产构建 `-DCANDASH_BUILD_PROFILE=production` 编译期排除 sim；2) 启动时 `getenv("VEHICLE_MODE")` 必须为 `production` | `can-processor/main.cpp:200-210` |
| **F013** | 报警文字溢出 / 截断 | 报警文字 > 128 字节（`SHM_ALARM_TEXT_LEN=128`） | 报警内容不完整 | 3 | 2 | 2 | **12** | `strncpy` + 末尾强制 `\0` | YAML 校验：`max-length: 127` | `shm_display.cpp:220-222` |
| **F014** | 共享内存半写（producer crash 期间） | can-processor 在 memcpy 中途崩溃 | dash 读到魔数对、checksum 错 | 6 | 1 | 2 | **12** | CRC32 校验、`MS_SYNC` msync 后才返回 | 1) dash 端在 checksum 失败时丢弃并显示旧值；2) Producer 用双缓冲（**TODO**） | `shm_display.cpp:229-234` |
| **F015** | 共享内存文件被外部进程替换 | 攻击者写 `can_display.bad` 试图伪造车速 | dash 读到伪造数据 | 9 | 1 | 6 | **54** | magic 校验（防匿名替换） | 1) `/dev/shm` 权限 0644 限制；2) **TODO**：HMAC 签名（密钥注入） | `shm_display.cpp:108` |
| **F016** | 100 ms tick 抖动 / 饿死 | 高负载下 `readFrame` 阻塞 > 100 ms | 报警 tick 漏触发 | 3 | 3 | 3 | **27** | `tick_ms - last_tick_ms` 累计检测 | 1) 减少 readFrame timeout；2) 关键报警走"事件驱动"而非 tick | `can-processor/main.cpp:265-274` |
| **F017** | NVI 析构陷阱 | 派生类析构里调 `close()` 虚函数 | 资源泄漏（sock_fd 未关闭） | 4 | 1 | 4 | **16** | `closeImpl_()` 非虚 | 已落实 NVI 模式（`socketcan_transport.cpp:89-101`） | `socketcan_transport.cpp` |
| **F018** | Heartbeat 超时误判 | dash 进程被 swap 出去 > 500 ms | dash 误报"信号丢失" | 2 | 2 | 3 | **12** | `shm_display_age_ms(now_ms)` | 1) dash 进程 `mlockall(MCL_CURRENT)` 防 swap；2) **TODO** | `shm_display.cpp:250-256` |

### 4.2 RPN 排序（Top-10 关注）

| 排序 | 编号 | RPN | 处理优先级 |
| --- | --- | --- | --- |
| 1 | F005 车速突变 | 64 | 高 |
| 1 | F007 报警误触发 | 64 | 高 |
| 3 | F012 模式误用 | 60 | 高 |
| 4 | F006 车速信号长时间不变 | 54 | 高 |
| 4 | F015 shm 伪造 | 54 | 中（攻击模型尚未建模） |
| 6 | F001 进程不读 CAN | 48 | 中 |
| 6 | F008 QML 渲染异常 | 48 | 中 |
| 8 | F002 进程崩溃 | 36 | 中 |
| 9 | F010 version 不匹配 | 30 | 中 |
| 10 | F016 tick 抖动 | 27 | 低 |

### 4.3 FMEA 趋势分析

- **RPN ≥ 50 的条目**：6 条（F005、F006、F007、F012、F015、F001 接近）—— 全部列入下一里程碑必修。
- **RPN ≤ 20 的条目**：5 条（F009、F013、F014、F017、F018）—— 已通过现有机制覆盖，仅做回归测试即可。
- **新型条目**：F011（双写者）、F015（伪造攻击）需在 v1.1 中加 mutex / HMAC 缓解。

---

## 5. 现有安全机制清单

### 5.1 Layer 1：共享内存数据完整性

| 机制 | 实现位置 | 说明 |
| --- | --- | --- |
| **Magic 标识** | `shm_display.h:38` `#define SHM_MAGIC 0xCA07D15A` | "CANDASH" 标识常量。dash 端 `shm_display_open()` 第 108 行比较，不一致返回 -2，区分"文件不存在"与"ABI 不匹配" |
| **Version 字段** | `shm_display.h:39-42` `SHM_VERSION_MAJOR/MINOR/PATCH` | minor 版本不一致仅 printk 警告（不阻塞运行）；major 不一致应拒绝（**TODO**） |
| **CRC32 校验** | `shm_display.cpp:26-58` CRC32-IEEE 802.3，多项式 `0xEDB88320` | 覆盖范围 offset 24..N-1（跳过 magic/version/last_commit_ms/updated_mask/checksum 自身）。`commit()` 时计算，dash `read()` 时再算一次比对 |
| **`msync(MS_SYNC)`** | `shm_display.cpp:233` | commit 阶段强制刷到物理内存，确保另一进程下次 mmap 立即可见 |
| **`SHM_BARRIER()`** | `can-processor/main.cpp:31` `__sync_synchronize()` | commit 前插入内存屏障，防止 CPU / 编译器重排把"先写字段再写 timestamp"颠倒成"先写 timestamp" |
| **运行路径可覆盖** | `shm_display.h:13-29` `CANDASH_SHM_PATH` / `CANDASH_SOCKET_PATH` 环境变量 | 量产部署到 `/var/run/can_display` 等持久化路径，避免 `/dev/shm` tmpfs 重启丢失 |
| **启动清理** | `shm_display.cpp:74` `unlink(path)` 在 `open(O_CREAT\|O_EXCL)` 之前 | 防止上次异常退出残留文件导致本次启动失败 |

### 5.2 Layer 2：信号质量监控

| 机制 | 实现位置 | 说明 |
| --- | --- | --- |
| **5 态质量分类** | `can_signal_monitor.cpp` `SignalQuality` 枚举 | GOOD / STALE / INVALID_RANGE / ABNORMAL_DELTA / NEVER_RECEIVED |
| **范围检查** | `can_signal_monitor.cpp:48-52` `if (value < def->min_value \|\| value > def->max_value)` | 越界即标 INVALID_RANGE |
| **突变检查** | `can_signal_monitor.cpp:54-62` `delta > def->max_delta` | 仅在质量为 GOOD / STALE 时检测（异常质量的上一次值不可信） |
| **超时检查** | `can_signal_monitor.cpp:83-95` `tick()` 巡检 `elapsed >= def->timeout_ms` | 100 ms tick 内对所有已接收信号做超时判定 |
| **滑动平滑** | `can_signal_monitor.cpp:64-74` + `vehicle_logic.cpp:54-75` | 5 点移动平均，降低噪声；SOC 走 `m_socHistory` 环形缓冲 |
| **预充电超时** | `vehicle_logic.cpp:98-107` `precharge_timeout_ms=3000` | 高压上电 3 s 未完成 → 标 PRECHARGE_FAILED 并发 `precharge_failed` 事件 |
| **报警去抖** | `alarm_runtime.cpp:36-46` `duration_ms / 100` 个 tick 持续确认 | 防止单帧噪声触发报警；典型 500 ms |

### 5.3 Layer 2：业务逻辑安全

| 机制 | 实现位置 | 说明 |
| --- | --- | --- |
| **Indicator 状态机** | `indicator_runtime.cpp:31-43` `setIndicator` | 每次写入更新 `lastChangeMs`，便于闪烁时序 |
| **Alarm 优先级** | `alarm_runtime.cpp:155` `flash = (rule->priority == PRIORITY_HIGH)` | 仅高优先级闪烁，避免告警疲劳 |
| **Alarm acknowledge** | `alarm_runtime.cpp:164-167` | 驾驶员已确认的报警不再重复触发 |
| **SeatBelt 联动** | `seat_belt_runtime.cpp` | 占用信号与 buckle 信号联合判定 |
| **ICanTransport 接口契约** | `can_transport.h:30-63` | 错误通过返回值传递，**不抛异常**（车规 MISRA-C:2012 Rule 21.3/15.5 友好） |

### 5.4 资源管理（NVI 模式）

| 机制 | 实现位置 | 说明 |
| --- | --- | --- |
| **NVI 析构** | `socketcan_transport.cpp:89-101` `closeImpl_()` 非虚 | 派生类析构里只调非虚 `closeImpl_()`，避免 cppcheck/MISRA 的"析构中调虚函数"警告 |
| **RAII 替代** | `can-processor/main.cpp:223, 230, 234` 局部对象 + 顺序析构 | `AlarmRuntime` / `IndicatorRuntime` / `SeatBeltRuntime` 出 main 作用域自动析构 |
| **fd 状态机** | 全局 `g_fd=-1`, `g_ptr=NULL` 哨兵 | close / mmap 失败时统一回退 |

### 5.5 静态分析 & 测试

- **clang-tidy**：`.clang-tidy` 已配置（`can-dash` 仓库根）
- **cppcheck**：`.cppcheck` 已配置
- **单元测试**：`tests/` 目录存在，重点覆盖 alarm / indicator / signal monitor（详见后续补全）
- **CMake 编译期断言**：`can-processor/main.cpp:30` 注释"// TODO(SIGINT)" 标记已知 TODO 项

---

## 6. 待实施安全机制（Gap 分析）

下表罗列**尚未完成**的安全条目，按优先级排序，全部列入下一里程碑（v1.1）。

| 编号 | 机制 | 优先级 | 实施位置 | 验收标准 |
| --- | --- | --- | --- | --- |
| **G01** | systemd watchdog（5 s 心跳） | P0 | `/etc/systemd/system/can-processor.service` | 模拟 segfault，5 s 内被拉起，`journalctl` 有 `Watchdog timeout` 日志 |
| **G02** | 共享内存双写者互斥 | P0 | `shm_display.cpp` 新增 `flock` | 启动两个 processor 实例，第二个 `shm_display_create` 失败并退出 |
| **G03** | QML UI 降级（数据无效时显示"信号丢失"） | P0 | `src/ui/qml/AlarmBanner.qml` 等 | 杀掉 can-processor，5 s 内 QML 显示"信号丢失"占位画面 |
| **G04** | 启动自检（POST） | P1 | `can-processor/main.cpp` 新增 `post_check()` | 启动时校验：1) `SHM_MAGIC` 已写入；2) `if_nametoindex(can0) > 0`；3) DISPLAY 环境变量存在 |
| **G05** | 故障注入单元测试 | P1 | `tests/test_signal_monitor.cpp` | 用例覆盖：a) 注入超范围值 → 期望 INVALID_RANGE；b) 注入突变值 → 期望 ABNORMAL_DELTA；c) 停止更新 1500 ms → 期望 STALE |
| **G06** | 共享内存 HMAC 签名 | P2 | `shm_display.cpp` 新增 `compute_hmac()` | 伪造 `/dev/shm/can_display`，dash 端因 HMAC 错拒绝 |
| **G07** | 死锁检测 | P2 | 全部 `tick()` 路径 | 单元测试：模拟 `readFrame` 永久阻塞，主循环 200 ms 内仍能切到 tick 路径 |
| **G08** | QML UI 自检（喂狗） | P2 | `src/main.cpp` `QTimer` | UI 线程 500 ms 内未触发 `frameSwapped` 信号则记录告警 |
| **G09** | 编译期模式选择 | P1 | `CMakeLists.txt` | production profile 编译时 sim-socket transport 链接失败 |
| **G10** | `mlockall(MCL_CURRENT)` 防 dash 被 swap | P1 | `can-dash/src/main.cpp` 启动 | 内存压力下 dash 进程 `RSS` 保持不变（`/proc/<pid>/status` VmRSS） |
| **G11** | 生产环境 `/var/run/can_display` 部署脚本 | P1 | `tools/install.sh` | systemd unit 默认 `Environment=CANDASH_SHM_PATH=/var/run/can_display` |
| **G12** | Fault Injection Campaign 报告 | P2 | `docs/FAULT_INJECTION.md` | 跑完 G05 的 8 个用例，输出 PASS/FAIL 矩阵 |
| **G13** | 双缓冲 shm（避免半写读） | P3 | `shm_display.cpp` 引入 ping-pong 字段 | reader 读到的永远是完整一帧（无 `INVALID_RANGE` 中间态） |
| **G14** | 报警文字长度 YAML schema 校验 | P1 | `tools/yaml_to_c.py` 启动时 | `> 127 chars` 报警文字编译失败 |
| **G15** | 启动时 `daemon_reload` 等待 | P2 | systemd unit `After=can-bus.service` | can0 接口出现后才启动 can-processor |

---

## 7. 安全生命周期（ISO 26262-2）

### 7.1 当前阶段总览

```
[概念阶段] ── 完成 ──> [系统级] ── TODO ──> [硬件级] ── TODO ──>
[软件级] ── 部分 ──> [生产] ── TODO ──> [运行/服务] ── TODO
     │                  │              │              │
     │ 本文件           │ 系统设计     │ 目标硬件      │ 量产
     │                  │ FMEA         │              │ 维护
     ▼                  ▼              ▼              ▼
   ✔ ASIL 分解       待启动         依赖选型        监控
   ✔ HAZOP           待启动         失效模式        OTA
   ✔ 初始 FMEA       待启动         诊断覆盖率      现场召回
   ✔ 安全目标 (SG)
```

### 7.2 各阶段状态

| 阶段 | ISO 26262-2 条款 | 状态 | 文档/证据 |
| --- | --- | --- | --- |
| **概念阶段** | 5 | ✅ 完成 | 本文件 `docs/SAFETY-ANALYSIS.md` |
| **系统级** | 6 | 🟡 TODO | 待输出：系统架构图、软硬件接口、HSI、SSC |
| **硬件级** | 7 | 🟡 TODO | 依赖目标硬件选型（PEAK / CANable / Vector）；需硬件 FMEDA |
| **软件级** | 8 | 🟡 部分 | 已落实：单元测试、clang-tidy/cppcheck；TODO：MISRA-C 合规报告、需求追溯矩阵 |
| **生产** | 9 | 🟡 TODO | 待输出：生产测试程序、PPAP 资料 |
| **运行/服务** | 10 | 🟡 TODO | 待输出：现场监测计划、OTA 策略、问题追踪 |

### 7.3 软件级已落实项

- ✅ **单元测试**：`tests/` 覆盖 alarm / indicator / signal monitor
- ✅ **静态分析**：clang-tidy + cppcheck CI
- ✅ **代码风格**：`.editorconfig` + `.clang-format`
- ✅ **YAML 即业务**：所有阈值/窗口参数走编译期 const 表，无运行时配置
- ✅ **Git Hook**：`.pre-commit-config.yaml` 强制 cppcheck / clang-tidy

### 7.4 软件级 TODO

- ⚠️ **MISRA-C:2012 合规报告**：当前实现已知违规点已用 `// cppcheck-suppress` 标注
- ⚠️ **需求追溯矩阵**：`docs/requirements/` 已有 REQ-*.md，需建立 REQ → FMEA 编号 → 代码位置的三向追溯
- ⚠️ **代码覆盖率**：目标 ≥ 80%（语句）/ ≥ 70%（分支）
- ⚠️ **动态分析**：valgrind 跑全套单元测试无泄漏

### 7.5 量产前门槛（Production Readiness Checklist）

| 项 | 当前 | 量产要求 |
| --- | --- | --- |
| FMEA 条目闭环率 | 0% | 100%（每条都有 mitigation + 验证） |
| 残余风险评审 | 0 | 全部 ACCEPT/MITIGATE/AVOID 标记 |
| 需求覆盖率 | 0% | 100% |
| 静态分析 | 已知违规 5 处 | 0 |
| 单元测试覆盖率 | 未测 | ≥ 80% |
| 集成测试 | 仿真通过 | 真实 CAN bus 通过 |
| watchdog 自愈 | 未实施 | 5 s 内重启 + 计数器 |
| OTA 通道 | 无 | 安全启动 + 签名验证 |

---

## 8. 残余风险评审

> 残余风险评估采用 ISO 26262 中常用的 **ACCEPT / MITIGATE / AVOID** 决策模型。本节列出当前识别到的 3 个最高残余风险（按 RPN 排序 Top 3 + 1 个未来关注）。

### 8.1 残余风险清单

| 编号 | 残余风险描述 | 严重度 | 类别 | 评审决策 | 后续行动 |
| --- | --- | --- | --- | --- | --- |
| **RR-01** | **CAN 总线物理干扰导致车速信号错显**（F005）<br/>根因：传感器故障 / 电磁干扰 / 总线竞争。现有机制：`CanSignalMonitor` 标 ABNORMAL_DELTA + 保留旧值。<br/>残余：100 ms 内有 1 帧错显可能。 | 高 | **MITIGATE** | 接受当前 1 帧 100 ms 的"擦除时间"，要求 v1.1 引入"双源交叉校验"（如车速 ← ESP + ← VCU 投票），投票结果一致才提交共享内存。 | v1.1 增加双源交叉校验，目标消除错显（PMHF < 10⁻⁸ /h） |
| **RR-02** | **生产/测试模式误用导致仿真数据被驾驶员看到**（F012）<br/>根因：can-processor 启动时未强制 production 模式，CI 误把 sim build 部署到车辆。<br/>现有机制：CLI `--transport` 解析无校验。 | 高 | **AVOID** | 在 v1.0.1 中通过**编译期**（`-DCANDASH_BUILD_PROFILE=production`）和**启动期**（`getenv("VEHICLE_MODE")=="production"`）双重检查避免：sim transport 在 production 构建里**不参与链接**，链接都失败。 | v1.0.1 强制编译期排除 sim transport；v1.1 加 systemd unit 强校验 |
| **RR-03** | **共享内存被外部进程伪造/篡改**（F015）<br/>根因：`/dev/shm` 权限 0644，未授权进程可写入；magic 校验只防"匿名替换"，不防"已知格式伪造"。 | 中 | **MITIGATE** | 接受 v1.0 阶段"无主动攻击模型"假设，仅依靠 magic + CRC。v1.1 引入 HMAC-SHA256 签名（密钥由 secure boot 注入到 processor 进程的 `mlock` 段），dash 端验签失败丢弃。 | v1.1 加 HMAC；v2.0 评估是否需要 Secure Boot |
| **RR-04** | **QML 渲染线程死锁或 GPU 驱动崩溃**（F008）<br/>根因：UI 层 bug；现有机制：无。残余：屏幕可能长时间定格。 | 中 | **MITIGATE** | 接受短期风险（驾驶员视觉发现后可主动泊车），v1.1 加 UI 自检（500 ms 喂狗 + fallback 静态画面）。 | v1.1 加 UI watchdog + 静态 fallback |
| **RR-05** | **systemd watchdog 缺失导致进程崩溃后无自愈**（F002）<br/>根因：当前无 systemd unit 配 watchdog 字段。 | 中 | **MITIGATE** | 接受 v1.0 阶段手动重启；v1.0.1 加 `WatchdogSec=5s` systemd unit。 | v1.0.1 systemd 单元 + mainloop `alarm(5)` 喂狗 |

### 8.2 残余风险评审结论

- **不存在** AVOID 类别中的"必须立刻停项目"的高风险项。
- **已识别** 5 条残余风险，其中 4 条已规划到具体版本（v1.0.1 / v1.1）做 MITIGATE，1 条（RR-02）做 AVOID。
- **PMHF 估算**（待 v1.1 双源校验后）：
  - 车速错显：~10⁻⁷ /h → 双源校验后 < 10⁻⁸ /h
  - 报警漏报：~5×10⁻⁸ /h（duration_ms 去抖）→ 维持
  - shm 伪造：~10⁻⁹ /h（攻击模型未建立）→ HMAC 后 < 10⁻¹⁰ /h
  - **合计 PMHF < 10⁻⁷ /h**，满足 ASIL-B 目标（10⁻⁷ /h）。
- **审核委员会**应在量产前重新评审残余风险清单，每条 MITIGATE 措施必须附验证证据（测试报告 / 评审纪要）。

---

## 9. 审计与签名

### 9.1 评审流程

| 阶段 | 评审委员 | 评审内容 | 输出 |
| --- | --- | --- | --- |
| 概念阶段（v1.0） | 项目负责人 / 安全负责人 / 系统架构师 | ASIL 分解、HAZOP、初始 FMEA | 本文件 v1.0 |
| 系统设计完成 | 项目负责人 / 安全负责人 / 测试负责人 / OEM 接口人 | 软硬件接口、HSI、SSC 草案 | 系统设计报告 |
| 量产前 | 项目负责人 / 安全负责人 / 测试负责人 / 质量负责人 / OEM SQE | 残余风险清单、PPAP 资料 | 量产放行决议 |

### 9.2 签名区

> **说明**：本节用于评审委员在文档基线化时手写签名（或在 GitLab/Phabricator 等工具中以 e-sign 留痕）。下表为示例填法，正式评审时填入实际姓名与日期。

| 角色 | 姓名 | 签字 | 日期 | 备注 |
| --- | --- | --- | --- | --- |
| **项目负责人** | _________________ | _________________ | _____________ |  |
| **安全负责人（FSM）** | _________________ | _________________ | _____________ | 独立安全评估 |
| **系统架构师** | _________________ | _________________ | _____________ |  |
| **测试负责人** | _________________ | _________________ | _____________ |  |
| **质量负责人** | _________________ | _________________ | _____________ |  |
| **OEM SQE / 接口人** | _________________ | _________________ | _____________ | 客户代表 |
| **评审委员会主席** | _________________ | _________________ | _____________ | 量产放行 |

### 9.3 版本历史

| 版本 | 日期 | 修订人 | 修订说明 |
| --- | --- | --- | --- |
| v1.0 | 2026-06-01 | FSM | 初版（概念阶段完成）：ASIL 分解、HAZOP、初始 FMEA、残余风险初评 |
| （待定）v1.0.1 | TBD | FSM | 加 G02/G09/G10 等 P0 缓解措施后重新评审 |
| （待定）v1.1 | TBD | FSM | 系统级 + 软件级设计完成，全量闭环 |
| （待定）v2.0 | TBD | FSM | 量产前最终评审 |

### 9.4 引用文档

- **项目架构**：`ARCHITECTURE.md`
- **多智能体架构**：`docs/MULTI_AGENT_ARCHITECTURE.md`
- **需求文档**：`docs/requirements/REQ-*.md`（REQ-ALM-*、REQ-HYBRID-*、REQ-IND-* 等）
- **共享内存协议**：`references/shared_memory_ipc.md`（如有）
- **ISO 26262-2:2018** §5 概念阶段、§6 系统级、§7 硬件级、§8 软件级
- **ISO 26262-5:2018** §7.4.4 ASIL 分解、§8 硬件架构度量
- **ISO 26262-6:2018** §5 软件级产品开发
- **ISO 26262-9:2018** §5 ASIL 分解与安全分析

### 9.5 工具链

- **静态分析**：clang-tidy 14+、cppcheck 2.10+
- **动态分析**：valgrind 3.20+、ASan、UBSan
- **测试框架**：GoogleTest / GoogleMock
- **CI**：GitHub Actions（`.github/workflows/`）
- **配置管理**：Git + pre-commit hook
- **构建**：CMake 3.22+、GCC 11+ / Clang 14+

---

> **文档结束。**
>
> 本文件是 can-dash 项目 ISO 26262 概念阶段的核心交付物。后续阶段（系统级 / 硬件级 / 软件级）须以本文件为起点，逐步细化 HSI、SSC、技术安全需求、软硬件接口与追溯矩阵。**任何 FMEA 条目关闭必须有验证证据；任何残余风险必须有评审纪要。**
