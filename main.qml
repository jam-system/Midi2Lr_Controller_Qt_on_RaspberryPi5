import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 960
    height: 600
    title: "Midi2Lr Controller"

    function textColorFor(bg) {
        if (!bg) return "white";
        var l = 0.299 * bg.r + 0.587 * bg.g + 0.114 * bg.b;
        return l < 0.5 ? "white" : "black";
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        RowLayout {
            spacing: 8
            Label {
                text: controller.connected ? "Connected" : "Disconnected"
                color: controller.connected ? "lightgreen" : "red"
            }
            Label {
                text: controller.currentSetName +
                      " (" + (controller.currentSet + 1) + "/" + controller.setCount + ")"
            }
            Button {
                text: "<"
                enabled: controller.currentSet > 0
                onClicked: controller.currentSet = controller.currentSet - 1
            }
            Button {
                text: ">"
                enabled: controller.currentSet + 1 < controller.setCount
                onClicked: controller.currentSet = controller.currentSet + 1
            }
        }

        GridLayout {
            columns: 4
            columnSpacing: 8
            rowSpacing: 8

            Repeater {
                model: 16
                Button {
                    id: btn
                    Layout.fillWidth: true

                    property color baseColor: {
                        var cols = controller.buttonColors
                        if (cols.length > index) return cols[index]
                        return "#444444"
                    }

                    text: {
                        var texts = controller.buttonTexts
                        if (texts.length > index) return texts[index]
                        return "B" + index
                    }

                    background: Rectangle {
                        radius: 4
                        border.width: 1
                        border.color: "#222"
                        color: btn.down ? Qt.darker(btn.baseColor, 1.4) : btn.baseColor
                    }

                    contentItem: Label {
                        text: btn.text
                        color: root.textColorFor(btn.baseColor)
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onPressed: controller.sendButton(index, true)
                    onReleased: controller.sendButton(index, false)
                }
            }
        }

        ColumnLayout {
            spacing: 4
            Repeater {
                model: 8
                RowLayout {
                    spacing: 8
                    Label {
                        text: {
                            var labels = controller.labelTexts
                            if (labels.length > index) return labels[index]
                            return "Enc " + index
                        }
                        Layout.preferredWidth: 90
                    }
                    Label {
                        text: {
                            var vals = controller.rotaryValues
                            if (vals.length > index) return vals[index]
                            return 0
                        }
                        Layout.preferredWidth: 40
                    }
                    Rectangle {
                        Layout.fillWidth: true
                        height: 8
                        radius: 4
                        color: "#333"
                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            height: parent.height
                            width: {
                                var vals = controller.rotaryValues
                                var v = (vals.length > index) ? vals[index] : 0
                                return parent.width * (v / 127.0)
                            }
                        }
                    }
                }
            }
        }
    }
}
