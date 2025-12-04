import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
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

        // Top row: status + profile navigation
        RowLayout {
            spacing: 8
            Layout.fillWidth: true

            Label {
                text: controller.connected ? "Connected" : "Disconnected"
                color: controller.connected ? "lightgreen" : "red"
            }
            Label {
                text: controller.currentSetName +
                      " (" + (controller.currentSet + 1) + "/" + controller.setCount + ")"
            }
            Item { Layout.fillWidth: true } // spacer

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

        // Middle: 16 buttons
        GridLayout {
            columns: 4
            columnSpacing: 8
            rowSpacing: 18
            Layout.fillWidth: true

            Layout.preferredHeight: 250

            Repeater {
                model: 16
                Button {
                    id: btn
                    Layout.fillWidth: true
                    Layout.fillHeight: true
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

                    onPressed:  controller.sendButton(index, true)
                    onReleased: controller.sendButton(index, false)
                }
            }
        }

        // Spacer to push encoder area to the bottom
        Item {
            Layout.fillHeight: true
        }

        // Bottom: encoder labels + values + bars
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Repeater {
                model: 8

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignBottom
                    spacing: 2

                    // Encoder text label
                    Label {
                        text: {
                            var labels = controller.labelTexts
                            if (labels.length > index) return labels[index]
                            return "Enc " + index
                        }
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: 14
                    }

                    // Encoder value label
                    Label {
                        text: {
                            var vals = controller.rotaryValues
                            if (vals.length > index) return vals[index]
                            return 0
                        }
                        horizontalAlignment: Text.AlignHCenter
                        Layout.alignment: Qt.AlignHCenter
                        font.pixelSize: 12
                        color: "#cccccc"
                    }

                    // Value bar
                    Rectangle {
                        Layout.fillWidth: true
                        height: 10
                        radius: 5
                        color: "#333"

                        Rectangle {
                            anchors.left: parent.left
                            anchors.verticalCenter: parent.verticalCenter
                            height: parent.height
                            radius: 5
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
