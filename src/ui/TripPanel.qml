// TripPanel.qml - 行程数据面板 (PR 3 + PR 4)
//
// 显示由 C++ TripComputer 算出的 7 个派生指标 (PR 3 加 4 个, PR 4 加 3 个)。
// 数据流: shm.{vehicle_speed, bat_volt, bat_curr, bat_soc, ev_range}
//        → ShmDataSource::onTick
//        → m_trip.tick() + m_trip.tickEnergy()
//        → DisplaySnapshot{trip_distance_km, trip_avg_speed_kmh,
//                          trip_duration_s, trip_is_moving,
//                          trip_energy_kwh, trip_efficiency_kwh100km,
//                          trip_range_confidence_pct}
//        → QtDataBinder 7 Q_PROPERTY → DashboardBackend 透传 → QML
//
// 7 个字段 (2×3 网格):
//   第 1 行: DIST (km) / TIME (mm:ss 或 h:mm:ss) / AVG (km/h, 停车 = 0)
//   第 2 行: ENERGY (kWh 累计) / EFF (kWh/100km, <0.5km 时为 0) / RANGE (0-100%)
//
// 顶部状态点: 停车灰 / 行驶绿
// RESET 按钮: 调 dashboard.resetTrip() 反向清零全部 7 字段
//
// QML 端 DerivedMetrics.qml 已被本面板取代 —
// 详见 PR 4 commit (git log 070fafe 之前的 commit)
import QtQuick 2.15
import QtQuick.Controls 2.15

Item {
    id: root
    width: 320
    height: 100

    // ─── 背景 ───
    Rectangle {
        anchors.fill: parent
        color: "#1a1a1a"
        radius: 6
        border.color: "#333333"
        border.width: 1
    }

    // ─── 标题栏 + reset 按钮 ───
    Row {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 6
        height: 14
        spacing: 4

        Text {
            text: "TRIP"
            color: "#888888"
            font.pixelSize: 9
            font.letterSpacing: 1
            font.family: "Roboto Mono, monospace"
            anchors.verticalCenter: parent.verticalCenter
        }

        Item { width: 1; height: 1 }  // 弹性间距

        // 行驶状态点 (停车 = 灰, 行驶 = 绿)
        Rectangle {
            width: 6; height: 6; radius: 3
            color: dashboard.tripIsMoving ? "#00FF88" : "#666666"
            anchors.verticalCenter: parent.verticalCenter
        }

        // 重置按钮
        Rectangle {
            id: resetBtn
            width: 38; height: 14
            color: resetMa.containsMouse ? "#FF4400" : "#552200"
            radius: 2
            border.color: "#FF4400"
            border.width: 1
            anchors.verticalCenter: parent.verticalCenter

            Text {
                anchors.centerIn: parent
                text: "RESET"
                color: resetMa.containsMouse ? "#FFFFFF" : "#FFAA88"
                font.pixelSize: 8
                font.family: "Roboto Mono, monospace"
            }
            MouseArea {
                id: resetMa
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: dashboard.resetTrip()
            }
        }
    }

    // ─── 2 行 3 列指标 ───
    Column {
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 4
        spacing: 2

        // ── 第 1 行: 里程 / 时长 / 均速 ──
        Row {
            spacing: 4
            width: parent.width
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "DIST"
                value: dashboard.tripDistanceKm.toFixed(2)
                unit: "km"
                valueColor: "#88CCFF"
            }
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "TIME"
                value: {
                    var s = dashboard.tripDurationS
                    if (s >= 3600) {
                        var h = Math.floor(s / 3600)
                        var m = Math.floor((s % 3600) / 60)
                        var sec = s % 60
                        return h + ":" + (m < 10 ? "0" : "") + m + ":" + (sec < 10 ? "0" : "") + sec
                    }
                    var m2 = Math.floor(s / 60)
                    var sec2 = s % 60
                    return m2 + ":" + (sec2 < 10 ? "0" : "") + sec2
                }
                unit: ""
                valueColor: "#FFAA00"
            }
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "AVG"
                value: dashboard.tripAvgSpeedKmh.toFixed(1)
                unit: "km/h"
                // 停车时灰色, 行驶时绿色
                valueColor: dashboard.tripIsMoving ? "#00FF88" : "#666666"
            }
        }

        // ── 第 2 行: 能耗 (kWh) / 百公里电耗 / 续航可信度 ──
        Row {
            spacing: 4
            width: parent.width
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "ENERGY"
                value: dashboard.tripEnergyKWh.toFixed(3)
                unit: "kWh"
                valueColor: "#CC88FF"
            }
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "EFF"
                value: dashboard.tripEfficiencyKWh100Km.toFixed(1)
                unit: "kWh/100km"
                // < 0.5km 时 TripComputer 返回 0, 灰色表示未起步
                valueColor: dashboard.tripEfficiencyKWh100Km > 0.01 ? "#FFAA00" : "#666666"
            }
            MetricCell {
                width: (parent.width - 8) / 3; height: 36
                label: "RANGE"
                value: dashboard.tripRangeConfidencePct.toFixed(0)
                unit: "%"
                // 颜色按可信度分档: ≥80% 绿, 50-80% 橙, <50% 红
                valueColor: dashboard.tripRangeConfidencePct >= 80 ? "#00FF88" :
                            dashboard.tripRangeConfidencePct >= 50 ? "#FFAA00" : "#FF4400"
            }
        }
    }
}
