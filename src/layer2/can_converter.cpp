// can_converter.cpp
#include "can_converter.h"
#include "../generated/can_field_def.h"
#include <cstdio>

CanConverter::CanConverter() = default;

void CanConverter::init(const CanFieldDef* table, int table_count) {
    m_table = table;
    m_tableCount = table_count;
}

uint32_t CanConverter::processFrame(uint32_t can_id, const uint8_t* data,
                                    size_t len, DisplayData& out) {
    uint32_t updated_mask = 0;

    for (int i = 0; i < m_tableCount; i++) {
        const CanFieldDef* def = &m_table[i];
        if (def->can_id != can_id) continue;
        if (def->byte_end >= (int)len) continue;

        uint64_t raw = extractRaw(def, data);
        float value = applyScaleOffset(def, raw);

        // 根据字段索引写入 DisplayData
        // 索引顺序与 can_field_table.cpp 完全一致（PR 60 之后 28 项）
        switch (i) {
            case  0: out.bat_volt = value; break;                       // bat_volt
            case  1: out.bat_curr = value; break;                       // bat_curr
            case  2: out.bat_soc = (uint8_t)value; break;              // bat_soc
            case  3: out.battery_temp = (uint8_t)value; break;          // battery_temp (was: vehicle_speed 错位)
            case  4: out.vehicle_speed = value; break;                  // vehicle_speed (was: brake 错位)
            case  5: out.brake = (uint8_t)value; break;                 // brake (was: motor_rpm 错位)
            case  6: out.motor_rpm = (int16_t)value; break;             // motor_rpm (was: motor_temp 错位)
            case  7: out.motor_temp = (uint8_t)value; break;            // motor_temp (was: missing)
            case  8: out.driver_occupied = (uint8_t)value; break;       // driver_occupied
            case  9: out.passenger_occupied = (uint8_t)value; break;     // passenger_occupied
            case 10: out.driver_buckled = (uint8_t)value; break;         // driver_buckled
            case 11: out.passenger_buckled = (uint8_t)value; break;      // passenger_buckled
            case 12: out.rear_buckle = (uint8_t)value; break;            // rear_buckle
            // 13-27 暂未在 DisplayData 暴露（HYBRID/CHARGE/LIMP_HOME/TIRE 字段）
            // 写到 out 的扩展字段需要 DisplayDataShm 添加；目前安全忽略
            default: break;
        }

        updated_mask |= (1U << i);
    }

    return updated_mask;
}

uint64_t CanConverter::extractRaw(const CanFieldDef* def, const uint8_t* data) {
    int byte_len = def->byte_end - def->byte_start + 1;
    if (byte_len <= 0 || byte_len > 8) return 0;

    uint64_t value = 0;
    if (def->endian == ENDIAN_LITTLE) {
        for (int i = 0; i < byte_len; i++) {
            value |= (uint64_t(data[def->byte_start + i]) << (i * 8));
        }
    } else {
        for (int i = 0; i < byte_len; i++) {
            value |= (uint64_t(data[def->byte_start + i]) << ((byte_len - 1 - i) * 8));
        }
    }

    if (def->bits < byte_len * 8) {
        uint64_t mask = (1ULL << def->bits) - 1;
        value = (value >> def->shift) & mask;
    }

    return value;
}

float CanConverter::applyScaleOffset(const CanFieldDef* def, uint64_t raw) {
    return float(raw) * def->scale + def->offset;
}

int CanConverter::findFieldIndex(const char* name) const {
    for (int i = 0; i < m_tableCount; i++) {
        if (strcmp(m_table[i].display_key, name) == 0) return i;
    }
    return -1;
}
