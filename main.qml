import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: root
    visible: true
    width: 1280
    height: 800
    title: "Midi2Lr Controller UI"

        function textColorFor(bg) {
            if (!bg)
                return "white";

            // bg.r/g/b are 0..1
            var l = 0.299 * bg.r + 0.587 * bg.g + 0.114 * bg.b;
            // dark background -> white text, light background -> black text
            return l < 0.5 ? "white" : "black";
        }

        // ... rest of your UI ...


        Row {
            spacing: 10

            Button {
                text: "<"
                enabled: controller.currentSet > 0
                onClicked: controller.currentSet = controller.currentSet - 1
            }

            Label {
                text: controller.currentSetName +
                      "  (" + (controller.currentSet + 1) + "/" + controller.setCount + ")"
                font.pixelSize: 20
            }

            Button {
                text: ">"
                enabled: controller.currentSet + 1 < controller.setCount
                onClicked: controller.currentSet = controller.currentSet + 1
            }
        }




    Repeater {
        model: 16
        Button {
            id: btn
            property color baseColor: {
                var cols = controller.buttonColors
                if (cols.length > index)
                    return cols[index]
                return "#444444"
            }

            text: {
                var texts = controller.buttonTexts
                if (texts.length > index)
                    return texts[index]
                return "B" + index
            }

            checkable: true

            background: Rectangle {
                radius: 4
                border.width: 1
                border.color: "#222"

                // darker when pressed/checked
                color: (btn.down || btn.checked)
                       ? Qt.darker(btn.baseColor, 1.4)   // factor > 1 = darker
                       : btn.baseColor
            }

            contentItem: Label {
                text: btn.text
                // pick text color based on baseColor brightness
                color: root.textColorFor(btn.baseColor)
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            onPressed:  controller.sendButton(index, true)
            onReleased: controller.sendButton(index, false)
        }
    }



    ColumnLayout {

        Row {
            spacing: 10

            Button {
                text: "<"
                enabled: controller.currentSet > 0
                onClicked: controller.currentSet = controller.currentSet - 1
            }

            Label {
                text: controller.currentSetName +
                      "  (" + (controller.currentSet + 1) + "/" + controller.setCount + ")"
                font.pixelSize: 20
            }

            Button {
                text: ">"
                enabled: controller.currentSet + 1 < controller.setCount
                onClicked: controller.currentSet = controller.currentSet + 1
            }
        }




        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        // --- Connection row ---
        RowLayout {
            spacing: 8
            Label { text: "Port:" }
            TextField {
                id: portField
                Layout.fillWidth: true
                text: controller.portName
                onEditingFinished: controller.portName = text
            }
            Button {
                text: controller.connected ? "Disconnect" : "Connect"
                onClicked: controller.connectSerial()
            }
        }

        // --- 16 buttons (4x4 grid) ---
        GroupBox {
            title: "Control Buttons (send to Pico)"
            Layout.fillWidth: true

            GridLayout {
                columns: 4
                rowSpacing: 8
                columnSpacing: 8
                anchors.margins: 8
                Repeater {
                    model: 16
                    Button {
                        id: btn
                        property color baseColor: {
                            var cols = controller.buttonColors
                            if (cols.length > index)
                                return cols[index]
                            return "#444444"
                        }

                        text: {
                            var texts = controller.buttonTexts
                            if (texts.length > index)
                                return texts[index]
                            return "B" + index
                        }

                        checkable: true

                        background: Rectangle {
                            radius: 4
                            border.width: 1
                            border.color: "#222"

                            color: btn.down
                            ? Qt.darker(btn.baseColor, 1.4)
                            : btn.baseColor
                        }

                        contentItem: Label {
                            text: btn.text
                            // pick text color based on baseColor brightness
                            color: root.textColorFor(btn.baseColor)
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                        }

                        onPressed:  controller.sendButton(index, true)
                        onReleased: controller.sendButton(index, false)
                    }
                }

            }
        }

        // --- 8 rotary labels ---
        GroupBox {
            title: "Rotary Positions (0–127 from Pico)"
            Layout.fillWidth: true
            Layout.fillHeight: true

            GridLayout {
                columns: 4
                rowSpacing: 4
                columnSpacing: 12
                anchors.margins: 8

                Repeater {
                    model: 8
                    RowLayout {
                        Label {
    					text: {
        				var labels = controller.labelTexts
        				if (labels.length > index)
            				return labels[index]
        					return "Enc " + index
    					}
    					Layout.preferredWidth: 80
						}

                        Label {
                            Layout.preferredWidth: 40
                            text: {
                                var vals = controller.rotaryValues
                                if (vals.length > index)
                                    return vals[index]
                                return 0
                            }
                        }
                        // simple bar-visualisation 0..127
                        Rectangle {
                            Layout.fillWidth: true
                            height: 6
                            radius: 3
                            color: "#333"
                            border.width: 1
                            border.color: "#666"

                            Rectangle {
                                anchors.left: parent.left
                                anchors.top: parent.top
                                anchors.bottom: parent.bottom
                                width: {
                                    var vals = controller.rotaryValues
                                    var v = (vals.length > index) ? vals[index] : 0
                                    return parent.width * (v / 127.0)
                                }
                                radius: 3
                                color: "#66aaff"
                            }
                        }
                    }
                }
            }
        }

        // --- Log area (optional) ---
        GroupBox {
            title: "Log"
            Layout.fillWidth: true
            Layout.preferredHeight: 100

            ScrollView {
                anchors.fill: parent
                TextArea {
                    id: logArea
                    readOnly: true
                    wrapMode: TextArea.NoWrap
                }
            }
        }
    }

    Connections {
        target: controller
        function onLogMessage(msg) {
            logArea.append(msg)
        }
    }
}

