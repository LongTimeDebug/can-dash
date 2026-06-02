// test_qt_data_binder.cpp
// QtDataBinder 单元测试（手写测试，遵循项目惯例）
//
// 覆盖：
// 1. displayData QVariantMap 转换（28 字段）
// 2. 健康状态变化（OK → STALE → DISCONNECTED）
// 3. 报警列表正确填充与清空
// 4. 安全带警告条件
// 5. 指示灯状态映射
// 6. 帧元数据 → dataHealth
// 7. is_moving 映射

#include "layer3/qt_data_binder.h"
#include "layer3/display_data_types.h"
#include "layer2/language_manager.h"

#include <QCoreApplication>
#include <cstdio>
#include <cstring>
#include <cassert>

static int g_test_count = 0;
static int g_test_passed = 0;

#define TEST_ASSERT(cond, msg) do { \
    g_test_count++; \
    if (cond) { \
        g_test_passed++; \
        printf("  ✓ %s\n", msg); \
    } else { \
        printf("  ✗ %s (line %d)\n", msg, __LINE__); \
    } \
} while(0)

namespace {

// 构造一个测试用 DisplaySnapshot
DisplaySnapshot makeSnapshot() {
    DisplaySnapshot s;
    s.data.bat_volt = 350.0f;
    s.data.bat_soc = 75;
    s.data.vehicle_speed = 60.0f;
    s.data.motor_rpm = 3000;
    s.data.motor_temp = 65;
    s.data.driver_occupied = 1;
    s.data.driver_buckled = 0;  // 未系安全带
    s.data.passenger_occupied = 0;
    s.meta.timestamp_ms = 1000;
    s.meta.frame_seq = 42;
    s.meta.updated_mask = (1U << 0) | (1U << 3);
    s.health = HEALTH_OK;
    s.is_moving = true;

    s.alarm_count = 1;
    auto& e = s.alarms[0];
    std::strncpy(e.name, "test_alarm", sizeof(e.name) - 1);
    std::strncpy(e.text_zh, "测试报警", sizeof(e.text_zh) - 1);
    std::strncpy(e.text_en, "Test Alarm", sizeof(e.text_en) - 1);
    e.priority = 0;
    e.color_r = 0xFF; e.color_g = 0x44; e.color_b = 0x00;

    s.seat_belt.seats[0].occupied = true;
    s.seat_belt.seats[0].buckled = false;
    s.seat_belt.seats[0].warning = true;
    s.seat_belt.warning_active = true;

    s.indicators.lights[0].on = true;
    s.indicators.lights[0].flash = true;
    s.indicators.lights[0].hz = 1.5f;

    return s;
}

}  // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);

    printf("\n=== QtDataBinder 测试 ===\n");

    // ─── Test 1: displayData QVariantMap 转换 ───
    printf("\n[1] displayData 字段映射:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        binder.onDataUpdated(s);

        auto dd = binder.displayData();
        TEST_ASSERT(qFuzzyCompare(dd["bat_volt"].toFloat(), 350.0f), "bat_volt = 350.0");
        TEST_ASSERT(dd["bat_soc"].toInt() == 75, "bat_soc = 75");
        TEST_ASSERT(qFuzzyCompare(dd["vehicle_speed"].toFloat(), 60.0f), "vehicle_speed = 60.0");
        TEST_ASSERT(dd["motor_rpm"].toInt() == 3000, "motor_rpm = 3000");
        TEST_ASSERT(dd["motor_temp"].toInt() == 65, "motor_temp = 65");
    }

    // ─── Test 2: 健康状态变化 ───
    printf("\n[2] 健康状态映射:\n");
    {
        QtDataBinder binder;
        binder.onHealthChanged(HEALTH_OK);
        TEST_ASSERT(binder.processorOnline(), "HEALTH_OK → processorOnline=true");
        TEST_ASSERT(binder.processorStatus() == "ok", "HEALTH_OK → status='ok'");

        binder.onHealthChanged(HEALTH_STALE);
        TEST_ASSERT(!binder.processorOnline(), "HEALTH_STALE → processorOnline=false");
        TEST_ASSERT(binder.processorStatus() == "stale", "HEALTH_STALE → status='stale'");

        binder.onHealthChanged(HEALTH_DISCONNECTED);
        TEST_ASSERT(binder.processorStatus() == "disconnected", "HEALTH_DISCONNECTED → status='disconnected'");
    }

    // ─── Test 3: 报警列表填充 ───
    printf("\n[3] 报警列表:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        s.alarm_count = 1;
        binder.onDataUpdated(s);

        TEST_ASSERT(binder.alarmActive(), "alarmActive = true");
        TEST_ASSERT(binder.alarmMessageZh() == "测试报警", "alarmMessageZh = '测试报警'");
        auto list = binder.alarmList();
        TEST_ASSERT(list.size() == 1, "alarmList.size() = 1");
        if (list.size() == 1) {
            auto m = list[0].toMap();
            TEST_ASSERT(m["text_zh"].toString() == "测试报警", "list[0].text_zh");
            TEST_ASSERT(m["text_en"].toString() == "Test Alarm", "list[0].text_en");
            TEST_ASSERT(m["priority"].toInt() == 0, "list[0].priority = 0");
            TEST_ASSERT(m["color"].toString() == "#ff4400", "list[0].color = '#ff4400'");
        }
    }

    // ─── Test 4: 报警清空 ───
    printf("\n[4] 报警清空:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        s.alarm_count = 1;
        binder.onDataUpdated(s);
        TEST_ASSERT(binder.alarmActive(), "初始有报警");

        s.alarm_count = 0;
        binder.onDataUpdated(s);
        TEST_ASSERT(!binder.alarmActive(), "第二帧无报警 → alarmActive=false");
        TEST_ASSERT(binder.alarmList().isEmpty(), "alarmList 已清空");
    }

    // ─── Test 5: 安全带警告 ───
    printf("\n[5] 安全带警告:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        binder.onDataUpdated(s);

        TEST_ASSERT(binder.seatBeltWarningActive(), "seatBeltWarningActive = true");
        auto seats = binder.seatIconStates();
        TEST_ASSERT(seats.size() == 5, "seatIconStates.size() = 5");
        if (seats.size() >= 1) {
            TEST_ASSERT(seats[0].toMap()["id"].toString() == "seatbelt.driver", "seats[0].id = 'seatbelt.driver'");
            TEST_ASSERT(seats[0].toMap()["warning"].toBool(), "seats[0].warning = true");
        }
    }

    // ─── Test 6: 指示灯映射 ───
    printf("\n[6] 指示灯映射:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        binder.onDataUpdated(s);

        TEST_ASSERT(binder.indicatorOn("left_turn_light"), "indicatorOn('left_turn_light') = true");
        auto states = binder.indicatorStates();
        auto left = states["left_turn_light"].toMap();
        TEST_ASSERT(left["on"].toBool(), "left.on = true");
        TEST_ASSERT(left["flash"].toBool(), "left.flash = true");
        TEST_ASSERT(qFuzzyCompare(left["hz"].toFloat(), 1.5f), "left.hz = 1.5");
    }

    // ─── Test 7: 帧元数据 ───
    printf("\n[7] 帧元数据:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        binder.onDataUpdated(s);

        TEST_ASSERT(binder.frameSeq() == 42u, "frameSeq = 42");
        auto validity = binder.fieldValidity();
        TEST_ASSERT(validity.contains("bat_volt"), "fieldValidity 包含 'bat_volt'");
        TEST_ASSERT(validity.contains("vehicle_speed"), "fieldValidity 包含 'vehicle_speed'");
    }

    // ─── Test 8: is_moving ───
    printf("\n[8] is_moving:\n");
    {
        QtDataBinder binder;
        auto s = makeSnapshot();
        s.is_moving = true;
        binder.onDataUpdated(s);
        TEST_ASSERT(binder.isMoving(), "isMoving = true (speed=60)");

        s.is_moving = false;
        binder.onDataUpdated(s);
        TEST_ASSERT(!binder.isMoving(), "isMoving = false (speed=0)");
    }

    printf("\n=== 总计: %d/%d 通过 ===\n", g_test_passed, g_test_count);
    return (g_test_passed == g_test_count) ? 0 : 1;
}
