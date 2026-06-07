// SeatBeltZone.qml - 安全带监控（REQ-SYS-004）
// 使用 QtQuick.Shapes 替代 Canvas 2D：GPU 合成 opacity，只动 NumberAnimation，无需 requestPaint
import QtQuick 2.15
import QtQuick.Shapes

Item {
    id: root
    width: 420
    height: 90

    // ─── 公开属性（兼容旧版）───
    property var dashboard: null
    readonly property var seatDefs: [
        { id: "driver",      labelZh: "主驾", labelEn: "DRIVER" },
        { id: "passenger",   labelZh: "副驾", labelEn: "PASSENGER" },
        { id: "rear_left",   labelZh: "后左", labelEn: "REAR L" },
        { id: "rear_center", labelZh: "后中", labelEn: "REAR C" },
        { id: "rear_right",  labelZh: "后右", labelEn: "REAR R" }
    ]

    // ─── 内部状态 ───
    property bool _hasWarning: false
    property bool isFlashing: false    // 驱动动画 running 状态

    // 布局常量（与旧 Canvas 算法保持一致视觉）
    readonly property real _seatW: Math.min(root.width / 5 - 8, 68)
    readonly property real _seatH: Math.min(root.height - 20, 62)
    readonly property real _gap: 8
    readonly property real _totalW: 5 * _seatW + 4 * _gap
    readonly property real _startX: (root.width - _totalW) / 2
    readonly property int _iconFontSize: Math.round(_seatH * 0.38)
    readonly property real _labelYOffset: _seatH + 14

    // ─── Shape: 5 个座位背景圆角矩形 ───
    // 使用 Shape + ShapePath 而非 Canvas：GPU 合成，路径不变时不重绘
    Shape {
        id: seatShape
        anchors.fill: parent
        antialiasing: true

        // Qt 6.6+ 曲线渲染器，比软件顶点着色快 5-10x
        // 不可用时自动 fallback 到默认渲染器
        rendererType: Shape.CurveRenderer

        // 5 个 ShapePath，共享一个 Shape（文档建议用法）
        // 座位 0: driver
        ShapePath {
            id: sp0
            fillColor: _bgColor(0)
            strokeColor: _strokeColor(0)
            strokeWidth: _strokeWidth(0)
            strokeStyle: ShapePath.SolidLine
            // 圆角矩形 SVG path：M=moveto, a=arc, h=horz, v=vert
            PathSvg {
                property real sx: _seatX(0)
                property real sy: 2
                property real sw: _seatW
                property real sh: _seatH
                property real r: 6
                path: "M" + (sx + r) + "," + sy
                    + " h" + (sw - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + r
                    + " v" + (sh - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + r
                    + " h" + (-(sw - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + (-r)
                    + " v" + (-(sh - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + (-r)
                    + " Z"
            }
        }
        // 座位 1: passenger
        ShapePath {
            id: sp1
            fillColor: _bgColor(1)
            strokeColor: _strokeColor(1)
            strokeWidth: _strokeWidth(1)
            strokeStyle: ShapePath.SolidLine
            PathSvg {
                property real sx: _seatX(1)
                property real sy: 2
                property real sw: _seatW
                property real sh: _seatH
                property real r: 6
                path: "M" + (sx + r) + "," + sy
                    + " h" + (sw - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + r
                    + " v" + (sh - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + r
                    + " h" + (-(sw - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + (-r)
                    + " v" + (-(sh - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + (-r)
                    + " Z"
            }
        }
        // 座位 2: rear_left
        ShapePath {
            id: sp2
            fillColor: _bgColor(2)
            strokeColor: _strokeColor(2)
            strokeWidth: _strokeWidth(2)
            strokeStyle: ShapePath.SolidLine
            PathSvg {
                property real sx: _seatX(2)
                property real sy: 2
                property real sw: _seatW
                property real sh: _seatH
                property real r: 6
                path: "M" + (sx + r) + "," + sy
                    + " h" + (sw - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + r
                    + " v" + (sh - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + r
                    + " h" + (-(sw - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + (-r)
                    + " v" + (-(sh - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + (-r)
                    + " Z"
            }
        }
        // 座位 3: rear_center
        ShapePath {
            id: sp3
            fillColor: _bgColor(3)
            strokeColor: _strokeColor(3)
            strokeWidth: _strokeWidth(3)
            strokeStyle: ShapePath.SolidLine
            PathSvg {
                property real sx: _seatX(3)
                property real sy: 2
                property real sw: _seatW
                property real sh: _seatH
                property real r: 6
                path: "M" + (sx + r) + "," + sy
                    + " h" + (sw - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + r
                    + " v" + (sh - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + r
                    + " h" + (-(sw - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + (-r)
                    + " v" + (-(sh - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + (-r)
                    + " Z"
            }
        }
        // 座位 4: rear_right
        ShapePath {
            id: sp4
            fillColor: _bgColor(4)
            strokeColor: _strokeColor(4)
            strokeWidth: _strokeWidth(4)
            strokeStyle: ShapePath.SolidLine
            PathSvg {
                property real sx: _seatX(4)
                property real sy: 2
                property real sw: _seatW
                property real sh: _seatH
                property real r: 6
                path: "M" + (sx + r) + "," + sy
                    + " h" + (sw - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + r
                    + " v" + (sh - 2 * r)
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + r
                    + " h" + (-(sw - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + (-r) + "," + (-r)
                    + " v" + (-(sh - 2 * r))
                    + " a" + r + "," + r + " 0 0 1 " + r + "," + (-r)
                    + " Z"
            }
        }
    }

    // ─── 5 个座位的图标文字和标签文字（层叠在 Shape 上方）───
    // 每个座位一个 Item，opacity 绑定动画，座位之间独立
    Item {
        id: seatItems
        anchors.fill: parent

        // 座位 0
        Item {
            id: seatItem0
            x: _seatX(0); y: 0; width: _seatW; height: root.height
            opacity: isFlashing && _seatWarning(0) ? _flashOpacity : 1.0
            NumberAnimation on _flashOpacity {
                id: anim0
                from: 1.0; to: 0.3
                duration: 250
                running: isFlashing && _seatWarning(0)
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
            property real _flashOpacity: 1.0
            Text {
                anchors.centerIn: parent
                y: -_seatH * 0.02
                text: _seatIconText(0)
                color: _iconColor(0)
                font.bold: true
                font.pixelSize: _iconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: _labelYOffset
                text: _seatLabel(0)
                color: "rgba(136,136,136,1)"
                font.bold: true
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }
        }

        // 座位 1
        Item {
            id: seatItem1
            x: _seatX(1); y: 0; width: _seatW; height: root.height
            opacity: isFlashing && _seatWarning(1) ? _flashOpacity : 1.0
            NumberAnimation on _flashOpacity {
                id: anim1
                from: 1.0; to: 0.3
                duration: 250
                running: isFlashing && _seatWarning(1)
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
            property real _flashOpacity: 1.0
            Text {
                anchors.centerIn: parent
                y: -_seatH * 0.02
                text: _seatIconText(1)
                color: _iconColor(1)
                font.bold: true
                font.pixelSize: _iconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: _labelYOffset
                text: _seatLabel(1)
                color: "rgba(136,136,136,1)"
                font.bold: true
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }
        }

        // 座位 2
        Item {
            id: seatItem2
            x: _seatX(2); y: 0; width: _seatW; height: root.height
            opacity: isFlashing && _seatWarning(2) ? _flashOpacity : 1.0
            NumberAnimation on _flashOpacity {
                id: anim2
                from: 1.0; to: 0.3
                duration: 250
                running: isFlashing && _seatWarning(2)
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
            property real _flashOpacity: 1.0
            Text {
                anchors.centerIn: parent
                y: -_seatH * 0.02
                text: _seatIconText(2)
                color: _iconColor(2)
                font.bold: true
                font.pixelSize: _iconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: _labelYOffset
                text: _seatLabel(2)
                color: "rgba(136,136,136,1)"
                font.bold: true
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }
        }

        // 座位 3
        Item {
            id: seatItem3
            x: _seatX(3); y: 0; width: _seatW; height: root.height
            opacity: isFlashing && _seatWarning(3) ? _flashOpacity : 1.0
            NumberAnimation on _flashOpacity {
                id: anim3
                from: 1.0; to: 0.3
                duration: 250
                running: isFlashing && _seatWarning(3)
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
            property real _flashOpacity: 1.0
            Text {
                anchors.centerIn: parent
                y: -_seatH * 0.02
                text: _seatIconText(3)
                color: _iconColor(3)
                font.bold: true
                font.pixelSize: _iconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: _labelYOffset
                text: _seatLabel(3)
                color: "rgba(136,136,136,1)"
                font.bold: true
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }
        }

        // 座位 4
        Item {
            id: seatItem4
            x: _seatX(4); y: 0; width: _seatW; height: root.height
            opacity: isFlashing && _seatWarning(4) ? _flashOpacity : 1.0
            NumberAnimation on _flashOpacity {
                id: anim4
                from: 1.0; to: 0.3
                duration: 250
                running: isFlashing && _seatWarning(4)
                loops: Animation.Infinite
                easing.type: Easing.InOutQuad
            }
            property real _flashOpacity: 1.0
            Text {
                anchors.centerIn: parent
                y: -_seatH * 0.02
                text: _seatIconText(4)
                color: _iconColor(4)
                font.bold: true
                font.pixelSize: _iconFontSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }
            Text {
                anchors.horizontalCenter: parent.horizontalCenter
                y: _labelYOffset
                text: _seatLabel(4)
                color: "rgba(136,136,136,1)"
                font.bold: true
                font.pixelSize: 9
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignBottom
            }
        }
    }

    // ─── 辅助函数（每帧调用，不触发整图重绘）───
    function _seatX(i) { return _startX + i * (_seatW + _gap) }
    function _seatWarning(i) {
        var seat = dashboard ? (dashboard.seatIconStates[i] || {}) : {}
        return !!seat.warning
    }
    function _seatOccupied(i) {
        var seat = dashboard ? (dashboard.seatIconStates[i] || {}) : {}
        return !!seat.occupied
    }
    function _seatBuckled(i) {
        var seat = dashboard ? (dashboard.seatIconStates[i] || {}) : {}
        return !!seat.buckled
    }
    function _bgColor(i) {
        var occupied = _seatOccupied(i)
        var buckled = _seatBuckled(i)
        var warning = _seatWarning(i)
        if (!occupied)     return "#33" + "333333"
        if (warning)       return "#AA" + "1100"
        if (buckled)       return "#00" + "6622"
        return "#33" + "333333"
    }
    function _strokeColor(i) {
        var warning = _seatWarning(i)
        return warning ? "#FF4400" : "#3C3C3C"
    }
    function _strokeWidth(i) {
        return _seatWarning(i) ? 2 : 1
    }
    function _iconText(i) {
        var occupied = _seatOccupied(i)
        if (!occupied) return "\u2014"  // —
        var buckled = _seatBuckled(i)
        return buckled ? "\u2713" : "!"  // ✓ or !
    }
    function _iconColor(i) {
        var occupied = _seatOccupied(i)
        if (!occupied) return "#55" + "555555"
        var buckled = _seatBuckled(i)
        return buckled ? "#00FF88" : "#FF4400"
    }
    function _seatIconText(i) { return _iconText(i) }
    function _seatLabel(i) {
        var def = seatDefs[i]
        var lang = dashboard ? dashboard.currentLanguage : "zh_CN"
        return (lang === "zh_CN") ? def.labelZh : def.labelEn
    }

    // ─── 监听 dashboard 信号（只更新状态，ShapePath 绑定自动刷新）───
    Connections {
        target: dashboard
        function onSeatBeltWarningChanged() {
            _updateWarningState()
            _updateIndicator()
        }
    }

    Connections {
        target: dashboard
        function onMovingChanged() {
            if (!dashboard.isMoving) {
                isFlashing = false
            }
        }
    }

    // ─── 状态更新 ───
    function _updateWarningState() {
        var states = dashboard ? dashboard.seatIconStates : []
        _hasWarning = Array.isArray(states) && states.some(function(s) { return !!s.warning })
        isFlashing = _hasWarning && dashboard.isMoving
    }

    function _updateIndicator() {
        if (dashboard) dashboard.setIndicator("seatbelt_warning", _hasWarning)
    }

    // ─── 初始化 ───
    Component.onCompleted: {
        _updateWarningState()
        _updateIndicator()
    }
}
