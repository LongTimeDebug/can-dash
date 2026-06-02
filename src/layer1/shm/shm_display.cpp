// shm_display.cpp
// 使用 /dev/shm/can_display 路径 + 常规文件 open() + mmap
// 路径可通过 CANDASH_SHM_PATH / CANDASH_SOCKET_PATH 环境变量覆盖
#include "shm_display.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <time.h>
#include <inttypes.h>  // PRIu32
#include <cmath>       // std::lround

static int g_fd = -1;
static DisplayDataShm* g_ptr = NULL;

// ─── 真实时间戳（CLOCK_MONOTONIC，防止系统时间跳变）────────
static uint64_t monotonic_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

// ─── CRC32-IEEE 802.3（查表 256 项）────────────────────────
static uint32_t crc32_table[256];
static int crc32_table_init = 0;
static void crc32_init(void) {
    if (crc32_table_init) return;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++)
            c = (c & 1) ? (SHM_CRC32_POLY ^ (c >> 1)) : (c >> 1);
        crc32_table[i] = c;
    }
    crc32_table_init = 1;
}

static uint32_t crc32_compute(const void* buf, size_t len) {
    crc32_init();
    const uint8_t* p = (const uint8_t*)buf;
    uint32_t c = 0xFFFFFFFFU;
    for (size_t i = 0; i < len; i++)
        c = crc32_table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFU;
}

// 计算结构体 checksum（跳过 magic/version/last_commit_ms/updated_mask/checksum/frame_seq/pad）
// 实际：magic(0..3) version(4..7) last_commit_ms(8..15) updated_mask(16..19) checksum(20..23) frame_seq(24..27) _pad(28..31) motor_rpm(32..)
// 要排除的：0..31（含 magic/version/last_commit_ms/updated_mask/checksum/frame_seq/pad）
// 包含的：32..N-1
static uint32_t compute_checksum(const DisplayDataShm* d) {
    const uint8_t* base = (const uint8_t*)d;
    return crc32_compute(base + 32, sizeof(DisplayDataShm) - 32);
}

// ─── 运行时路径解析 ─────────────────────────────────────
const char* shm_display_get_path(void) {
    const char* env = getenv("CANDASH_SHM_PATH");
    return (env && env[0]) ? env : SHM_DISPLAY_PATH;
}

const char* socket_get_path(void) {
    const char* env = getenv("CANDASH_SOCKET_PATH");
    return (env && env[0]) ? env : SOCKET_PATH;
}

// ─── 创建（processor用）────────────────────────────────
int shm_display_create(void) {
    const char* path = shm_display_get_path();
    unlink(path);
    g_fd = open(path, O_RDWR | O_CREAT | O_EXCL, 0664);
    if (g_fd < 0) { fprintf(stderr, "[shm] create %s: %s\n", path, strerror(errno)); return -1; }
    if (ftruncate(g_fd, sizeof(DisplayDataShm)) < 0) {
        fprintf(stderr, "[shm] ftruncate: %s\n", strerror(errno));
        close(g_fd); unlink(path); g_fd=-1; return -1;
    }
    g_ptr = (DisplayDataShm*)mmap(NULL, sizeof(DisplayDataShm),
                                   PROT_READ|PROT_WRITE, MAP_SHARED, g_fd, 0);
    if (g_ptr == MAP_FAILED) {
        fprintf(stderr, "[shm] mmap: %s\n", strerror(errno));
        close(g_fd); unlink(path); g_fd=-1; return -1;
    }
    // 显式初始化 magic/version（避开 memset on float 警告：
    //   IEEE 754 0.0 = 全 0 位，memset 0 在我们平台上完全等价于清零，
    //   但严格 MISRA 建议用赋值。下面的 magic/version 立刻被覆盖，
    //   其余 464 字节全部 0 等价于 0.0/0/false，与运行时语义一致）
    // cppcheck-suppress memsetClassFloat
    memset(g_ptr, 0, sizeof(DisplayDataShm));
    // 写 magic + version（保护 ABI 兼容）
    g_ptr->magic = SHM_MAGIC;
    g_ptr->version = SHM_VERSION;
    return 0;
}

// ─── 打开（dash用）─────────────────────────────────────
int shm_display_open(void) {
    const char* path = shm_display_get_path();
    g_fd = open(path, O_RDONLY);
    if (g_fd < 0) return -1;
    g_ptr = (DisplayDataShm*)mmap(NULL, sizeof(DisplayDataShm),
                                   PROT_READ, MAP_SHARED, g_fd, 0);
    if (g_ptr == MAP_FAILED) { close(g_fd); g_fd=-1; return -1; }
    // 校验 magic（保护 ABI 兼容）
    if (g_ptr->magic != SHM_MAGIC) {
        fprintf(stderr, "[shm] magic mismatch: 0x%08X (expected 0x%08X) — incompatible processor binary\n",
                g_ptr->magic, SHM_MAGIC);
        munmap(g_ptr, sizeof(DisplayDataShm));
        close(g_fd);
        g_ptr = NULL; g_fd = -1;
        return -2;  // 区分"文件不存在"和"ABI 不匹配"
    }
    if (g_ptr->version != SHM_VERSION) {
        // cppcheck: SHM_VERSION 是无符号常量宏，g_ptr->version 是 uint32_t；
        //   用 PRIu32 避免任何类型假设
        fprintf(stderr, "[shm] version mismatch: %" PRIu32 " (expected %u) — update both binaries\n",
                static_cast<uint32_t>(g_ptr->version), SHM_VERSION);
        // 不阻塞运行（不同次版本可兼容），但记录告警
    }
    return 0;
}

// ─── 轮询 ──────────────────────────────────────────────
uint64_t shm_display_poll(uint64_t last_ts) {
    if (!g_ptr) return last_ts;
    return g_ptr->last_commit_ms;
}

// ─── 读（dash用）───────────────────────────────────────
// 返回值：0=OK；-1=未连接；-2=ABI 不匹配；-3=checksum 校验失败
// out_timestamp_ms: 写入上次 commit 的 monotonic 毫秒
int shm_display_read(DisplayDataShm* out, uint64_t* out_timestamp_ms) {
    if (!g_ptr || !out) return -1;
    if (g_ptr->magic != SHM_MAGIC) return -2;
    memcpy(out, g_ptr, sizeof(DisplayDataShm));
    if (out_timestamp_ms) *out_timestamp_ms = g_ptr->last_commit_ms;
    if (out->checksum != compute_checksum(out)) return -3;
    return 0;
}

// ─── 写整个结构（processor用）──────────────────────────
void shm_display_write(const DisplayDataShm* data) {
    if (!g_ptr || !data) return;
    memcpy(g_ptr, data, sizeof(DisplayDataShm));
}

// ─── 标记字段已更新 ────────────────────────────────────
void shm_display_mark_updated(ShmFieldIndex idx) {
    if (!g_ptr || idx >= SHM_FIELD_COUNT) return;
    g_ptr->updated_mask |= (1U << idx);
}

// ─── 字段写入 ─────────────────────────────────────────
void shm_display_set_float(ShmFieldIndex idx, float value) {
    if (!g_ptr || idx >= SHM_FIELD_COUNT) return;
    switch (idx) {
        case SHM_FIELD_BAT_VOLT:      g_ptr->bat_volt = value; break;
        case SHM_FIELD_BAT_CURR:      g_ptr->bat_curr = value; break;
        case SHM_FIELD_VEHICLE_SPEED:  g_ptr->vehicle_speed = value; break;
        case SHM_FIELD_MOTOR_RPM:     g_ptr->motor_rpm = value; break;
        case SHM_FIELD_CHARGE_POWER:  g_ptr->charge_power = value; break;
        default: return;
    }
    shm_display_mark_updated(idx);
}

void shm_display_set_uint8(ShmFieldIndex idx, uint8_t value) {
    if (!g_ptr || idx >= SHM_FIELD_COUNT) return;
    switch (idx) {
        case SHM_FIELD_BAT_SOC:             g_ptr->bat_soc = value; break;
        case SHM_FIELD_MOTOR_TEMP:          g_ptr->motor_temp = value; break;
        case SHM_FIELD_BRAKE:               g_ptr->brake = value; break;
        case SHM_FIELD_DRIVER_OCCUPIED:     g_ptr->driver_occupied = value; break;
        case SHM_FIELD_PASSENGER_OCCUPIED:  g_ptr->passenger_occupied = value; break;
        case SHM_FIELD_DRIVER_BUCKLED:      g_ptr->driver_buckled = value; break;
        case SHM_FIELD_PASSENGER_BUCKLED:   g_ptr->passenger_buckled = value; break;
        case SHM_FIELD_REAR_BUCKLE:         g_ptr->rear_buckle = value; break;
        // HYBRID uint8 fields
        case SHM_FIELD_BATTERY_TEMP:        g_ptr->battery_temp = value; break;
        case SHM_FIELD_ENERGY_MODE:          g_ptr->energy_mode = value; break;
        case SHM_FIELD_FUEL_LEVEL:           g_ptr->fuel_level = value; break;
        case SHM_FIELD_CHARGE_STATUS:        g_ptr->charge_status = value; break;
        case SHM_FIELD_ENGINE_FAULT:         g_ptr->engine_fault = value; break;
        case SHM_FIELD_GEAR_STATUS:          g_ptr->gear_status = value; break;
        default: return;
    }
    shm_display_mark_updated(idx);
}

void shm_display_set_uint16(ShmFieldIndex idx, uint16_t value) {
    if (!g_ptr || idx >= SHM_FIELD_COUNT) return;
    switch (idx) {
        case SHM_FIELD_FUEL_RANGE:   g_ptr->fuel_range = value; break;
        case SHM_FIELD_EV_RANGE:     g_ptr->ev_range = value; break;
        case SHM_FIELD_ENGINE_RPM:   g_ptr->engine_rpm = value; break;
        default: return;
    }
    shm_display_mark_updated(idx);
}

// NOLINT(bugprone-easily-swappable-parameters) — IPC 接口固定: (id, on, flash, hz)
void shm_display_set_indicator(ShmIndicatorId id, int on, int flash, float hz) {  // NOLINT(bugprone-easily-swappable-parameters)
    if (!g_ptr || id < 0 || id >= SHM_INDICATOR_COUNT) return;
    g_ptr->indicators[id].on = (uint8_t)on;
    g_ptr->indicators[id].flash = (uint8_t)flash;
    g_ptr->indicators[id].hz_x10 = static_cast<uint8_t>(std::lround(static_cast<double>(hz) * 10.0));
}

void shm_display_set_backlight_state(uint8_t state) {
    if (!g_ptr) return;
    g_ptr->backlight_state = state;
}

void shm_display_set_alarm(const char* text_zh) {
    if (!g_ptr) return;
    if (text_zh && text_zh[0]) {
        g_ptr->alarm_active = 1;
        strncpy(g_ptr->alarm_message_zh, text_zh, SHM_ALARM_TEXT_LEN - 1);
        g_ptr->alarm_message_zh[SHM_ALARM_TEXT_LEN - 1] = '\0';
    } else {
        g_ptr->alarm_active = 0;
        g_ptr->alarm_message_zh[0] = '\0';
    }
}

// ─── 提交（写 monotonic 毫秒 + 帧序号自增 + 重算 checksum + msync）────────
void shm_display_commit(void) {
    if (!g_ptr) return;
    g_ptr->frame_seq++;
    g_ptr->last_commit_ms = monotonic_ms();
    g_ptr->checksum = compute_checksum(g_ptr);
    msync(g_ptr, sizeof(DisplayDataShm), MS_SYNC);
}

// ─── 帧序号读取（dash 用，可选）────────────────────────
uint32_t shm_display_frame_seq(void) {
    if (!g_ptr) return 0;
    return g_ptr->frame_seq;
}

// ─── 关闭 ──────────────────────────────────────────────
void shm_display_close(void) {
    if (g_ptr && g_ptr != (void*)MAP_FAILED) munmap(g_ptr, sizeof(DisplayDataShm));
    if (g_fd >= 0) close(g_fd);
    g_ptr = NULL; g_fd = -1;
}

// ─── 健康监测 ──────────────────────────────────────────
int shm_display_health_check(void) {
    if (!g_ptr) return -1;
    if (g_ptr->magic != SHM_MAGIC) return -2;  // ABI 不匹配
    return 0;
}

uint64_t shm_display_age_ms(uint64_t now_ms) {
    if (!g_ptr) return UINT64_MAX;
    if (g_ptr->magic != SHM_MAGIC) return UINT64_MAX;  // ABI 不匹配
    if (g_ptr->last_commit_ms == 0) return UINT64_MAX;  // 还没收到第一帧
    if (now_ms < g_ptr->last_commit_ms) return 0;       // 时钟回退保护
    return now_ms - g_ptr->last_commit_ms;
}
