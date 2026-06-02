// shm_display.h
// Layer 1: 共享内存显示数据结构（纯C，无Qt，无动态内存）

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// 共享内存路径可通过环境变量覆盖（量产环境部署到 /var/run 等）
// 优先级：CANDASH_SHM_PATH 环境变量 > CMAKE 编译时宏 > 默认
#ifndef SHM_DISPLAY_PATH
#  ifdef CANDASH_DEFAULT_SHM_PATH
#    define SHM_DISPLAY_PATH CANDASH_DEFAULT_SHM_PATH
#  else
#    define SHM_DISPLAY_PATH "/dev/shm/can_display"
#  endif
#endif

#ifndef SOCKET_PATH
#  ifdef CANDASH_DEFAULT_SOCKET_PATH
#    define SOCKET_PATH CANDASH_DEFAULT_SOCKET_PATH
#  else
#    define SOCKET_PATH "/tmp/can_processor_socket"
#  endif
#endif

// 解析运行时路径（环境变量 > 编译时默认）
const char* shm_display_get_path(void);
const char* socket_get_path(void);

// ─── 共享内存 ABI 标识 ──────────────────────────────────
// Magic: 固定标识，processor/dash 必须一致，否则拒绝打开（防 ABI 错乱）
// Version: 兼容版本号，major.minor.patch × 10000 + minor × 100 + patch
#define SHM_MAGIC       0xCA07D15A  // "CANDASH" 标识
#define SHM_VERSION_MAJOR  1
#define SHM_VERSION_MINOR  2
#define SHM_VERSION_PATCH  0
#define SHM_VERSION     ((SHM_VERSION_MAJOR * 10000) + (SHM_VERSION_MINOR * 100) + SHM_VERSION_PATCH)

// 健康监测：处理器心跳超时阈值（dash 端使用）
#ifndef SHM_HEARTBEAT_TIMEOUT_MS
#define SHM_HEARTBEAT_TIMEOUT_MS 500
#endif

// 共享内存 CRC32 多项式（IEEE 802.3）— 用于结构体完整性校验
#define SHM_CRC32_POLY 0xEDB88320U

typedef enum {
    SHM_FIELD_TIMESTAMP = 0,
    SHM_FIELD_MOTOR_RPM,
    SHM_FIELD_VEHICLE_SPEED,
    SHM_FIELD_BAT_VOLT,
    SHM_FIELD_BAT_CURR,
    SHM_FIELD_BAT_SOC,
    SHM_FIELD_MOTOR_TEMP,
    SHM_FIELD_BRAKE,
    SHM_FIELD_DRIVER_OCCUPIED,
    SHM_FIELD_PASSENGER_OCCUPIED,
    SHM_FIELD_DRIVER_BUCKLED,
    SHM_FIELD_PASSENGER_BUCKLED,
    SHM_FIELD_REAR_BUCKLE,
    // HYBRID fields (indices 12-18)
    SHM_FIELD_BATTERY_TEMP,
    SHM_FIELD_ENERGY_MODE,
    SHM_FIELD_FUEL_LEVEL,
    SHM_FIELD_FUEL_RANGE,
    SHM_FIELD_CHARGE_POWER,
    SHM_FIELD_CHARGE_STATUS,
    SHM_FIELD_EV_RANGE,
    SHM_FIELD_ENGINE_RPM,
    SHM_FIELD_ENGINE_FAULT,
    SHM_FIELD_GEAR_STATUS,
    SHM_FIELD_COUNT
} ShmFieldIndex;

#define SHM_INDICATOR_COUNT 12
typedef struct {
    uint8_t on;
    uint8_t flash;
    uint8_t hz_x10;
    uint8_t _pad;
} ShmIndicatorSlot;

#define SHM_ALARM_TEXT_LEN 128

typedef struct {
    uint32_t  magic;              // ABI magic = SHM_MAGIC
    uint32_t  version;            // ABI version = SHM_VERSION
    uint64_t  last_commit_ms;     // processor 上次 commit 的 CLOCK_MONOTONIC 毫秒
    uint32_t  updated_mask;       // bit[i]=1 表示字段 i 已更新（消费后可清除）
    uint32_t  checksum;           // CRC32 of struct bytes [12..N-1]（不含 magic/version/checksum）
    uint32_t  frame_seq;          // 帧序号（processor 每次 commit 自增，用于检测丢帧/重放）
    uint32_t  _pad_after_seq;     // 保持 8 字节对齐（PR2 新增）
    // Basic fields (same as before)
    float     motor_rpm;
    float     vehicle_speed;
    float     bat_volt;
    float     bat_curr;
    uint8_t   bat_soc;
    uint8_t   motor_temp;
    uint8_t   brake;
    uint8_t   driver_occupied;
    uint8_t   passenger_occupied;
    uint8_t   driver_buckled;
    uint8_t   passenger_buckled;
    uint8_t   rear_buckle;
    // HYBRID fields (indices 12+)
    uint8_t   battery_temp;      // SHM_FIELD_BATTERY_TEMP (index 13)
    uint8_t   energy_mode;       // SHM_FIELD_ENERGY_MODE (index 14)
    uint8_t   fuel_level;        // SHM_FIELD_FUEL_LEVEL  (index 15)
    uint16_t  fuel_range;        // SHM_FIELD_FUEL_RANGE  (index 16)
    float     charge_power;       // SHM_FIELD_CHARGE_POWER (index 17)
    uint8_t   charge_status;     // SHM_FIELD_CHARGE_STATUS (index 18)
    uint16_t  ev_range;          // SHM_FIELD_EV_RANGE    (index 19)
    uint16_t  engine_rpm;        // SHM_FIELD_ENGINE_RPM  (index 20)
    uint8_t   engine_fault;      // SHM_FIELD_ENGINE_FAULT (index 21)
    uint8_t   gear_status;       // SHM_FIELD_GEAR_STATUS (index 22)
    uint8_t   _reserved2[3];     // padding to 8-byte alignment
    // Backlight control (REQ-SYS-003)
    uint8_t   backlight_timeout_seconds; // LCD背光超时时间(秒)
    uint8_t   backlight_state;            // 0=正常, 1=暗, 2=关闭
    uint8_t   alarm_active;
    char      alarm_message_zh[SHM_ALARM_TEXT_LEN];
    ShmIndicatorSlot indicators[SHM_INDICATOR_COUNT];
    uint8_t   _padding[227];     // 保持总大小不变（新增 frame_seq 4 字节，padding 减 4）
} DisplayDataShm;


typedef enum {
    IND_LEFT_TURN = 0, IND_RIGHT_TURN = 1, IND_PARK_BRAKE = 2,
    IND_READY_GO = 3, IND_BAT_WARN = 4, IND_ENGINE = 5,
    IND_HIGH_VOLT = 6, IND_FOG_LIGHT = 7, IND_SEATBELT = 8,
    IND_TIRE_PRESSURE = 9, IND_COUNT
} ShmIndicatorId;

// processor端
int shm_display_create(void);
void shm_display_write(const DisplayDataShm* data);
void shm_display_set_float(ShmFieldIndex idx, float value);
void shm_display_set_uint8(ShmFieldIndex idx, uint8_t value);
void shm_display_set_uint16(ShmFieldIndex idx, uint16_t value);
void shm_display_set_alarm(const char* msg_zh);
void shm_display_set_indicator(ShmIndicatorId id, int on, int flash, float hz);
void shm_display_set_backlight_state(uint8_t state);
void shm_display_mark_updated(ShmFieldIndex idx);
void shm_display_commit(void);

// dash端
int      shm_display_open(void);
// 返回值：0=OK；-1=未连接；-2=ABI 不匹配；-3=checksum 校验失败
// out_timestamp_ms: 写入上次 commit 的 monotonic 毫秒（NULL 忽略）
int      shm_display_read(DisplayDataShm* out_data, uint64_t* out_timestamp_ms);
uint64_t shm_display_poll(uint64_t last_timestamp);
void     shm_display_close(void);

// 健康监测
int      shm_display_health_check(void);              // 0=OK, -1=未连接, -2=ABI 不匹配
uint64_t shm_display_age_ms(uint64_t now_ms);         // 距上次 commit 的毫秒数，UINT64_MAX=未连接
uint32_t shm_display_frame_seq(void);                // 当前帧序号（dash 用，可用于检测丢帧）

#ifdef __cplusplus
}
#endif
