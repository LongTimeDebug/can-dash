// ⚠️ 此文件由 tools/yaml_to_c.py 自动生成
// ⚠️ 请勿手动修改，修改请改 config/can_ids.yaml

#pragma once
#include <stdint.h>
#include <stdbool.h>

// 显示数据结构（按领域拆分）
typedef struct {
    float         bat_volt;   // unit: V
    float         bat_curr;   // unit: A
    uint8_t       bat_soc;   // unit: %
    float         vehicle_speed;   // unit: km/h
    uint8_t       brake;   // unit: %
    int16_t       motor_rpm;   // unit: rpm
    uint8_t       motor_temp;   // unit: °C
    uint8_t       driver_occupied;   // unit: 
    uint8_t       passenger_occupied;   // unit: 
    uint8_t       driver_buckled;   // unit: 
    uint8_t       passenger_buckled;   // unit: 
    uint8_t       rear_buckle;   // unit: 
} DisplayData;

