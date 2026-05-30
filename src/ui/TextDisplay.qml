// TextDisplay.qml - 文字显示组件
import QtQuick 2.15

Item {
    id: root
    property string text: ""
    property int fontSize: 24
    property color color: "#FFFFFF"

    width: 200
    height: 40

    Text {
        id: label
        anchors.centerIn: parent
        text: root.text
        color: root.color
        font.pixelSize: root.fontSize
        font.family: "Roboto Mono, monospace"
    }
}
