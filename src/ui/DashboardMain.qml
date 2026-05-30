// DashboardMain.qml - 主界面
import QtQuick 2.15
import QtQuick.Controls 2.5
import QtQuick.Layouts 1.3
import CanDash 1.0

ApplicationWindow {
    id: root
    width: 800
    height: 480
    visible: true
    title: "CAN-Dash"

    // 背景
    Rectangle {
        anchors.fill: parent
        color: "#0A0A0A"
    }

    // 车速表盘
    GaugeControl {
        id: speedGauge
        x: 240
        y: 40
        width: 320
        height: 320
        minValue: 0
        maxValue: 260
        value: dashboard.get("vehicle_speed") || 0
        unit: "km/h"
    }

    // 电池电压文字
    TextDisplay {
        id: batVoltText
        x: 600
        y: 60
        text: (dashboard.get("bat_volt") || 0).toFixed(1) + "V"
        fontSize: 24
        color: "#FFFFFF"
    }

    // SOC 进度条
    BarIndicator {
        id: socBar
        x: 600
        y: 100
        width: 40
        height: 180
        minValue: 0
        maxValue: 100
        value: dashboard.get("bat_soc") || 0
        direction: "bottom_up"
        barColor: "#00FF88"
    }

    // 电池报警灯
    WarningLight {
        id: batWarnLight
        x: 600
        y: 260
        active: dashboard.alarmActive
        imageOn: "images/warning_bat_red.png"
        imageOff: "images/warning_bat_dim.png"
    }

    // 安全带区域
    SeatBeltZone {
        id: seatBeltZone
        x: 200
        y: 380
        width: 400
        height: 80
        isMoving: dashboard.isMoving
        seatIconStates: dashboard.seatIconStates
    }

    // 报警文字（最高优先级）
    Rectangle {
        id: alarmBanner
        anchors.horizontalCenter: parent.horizontalCenter
        y: 80
        visible: dashboard.alarmActive

        color: "#CC000000"
        radius: 6

        Text {
            anchors.fill: parent
            anchors.margins: 10
            text: dashboard.alarmMessageZh
            color: "#FF4400"
            font.pixelSize: 32
            font.weight: Font.Bold
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // 连接 backend
    Component.onCompleted: {
        console.log("DashboardMain.qml loaded")
    }
}
