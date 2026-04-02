import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: cureDevicesPage
    width: 300
    height: 500

    SystemPalette {
        id: activePalette
        colorGroup: SystemPalette.Active
    }

    header: Label {
        text: qsTr("Devices")
        font.pixelSize: Qt.application.font.pixelSize * 2

        padding: 10
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 2
        }

        GroupBox {
            id: updateCureDeviceProgressGroupBox
            title: qsTr("Firmware Update Progress")
            visible: Statemachine.updateInProgress
            enabled: Statemachine.updateInProgress
            Layout.fillWidth: true
            RowLayout {
                anchors {
                    fill: parent
                    margins: 2
                }

                ProgressBar {
                    Layout.fillWidth: true
                    value:Statemachine.UpdateProgress
                    from: 0.0
                    to: 1.0
                    padding: 2

                }
            }
        }

        GroupBox {
            id: updateCureDeviceGroupBox
            title: qsTr("Firmware Update")
            visible: Statemachine.suggestUpdate
            enabled: Statemachine.suggestUpdate
            Layout.fillWidth: true
            RowLayout {
                anchors {
                    fill: parent
                    margins: 2
                }

                Rectangle {
                    Layout.fillWidth: true
                }

                Button {
                    id: acceptCureDeviceUpdate
                    icon.name: "dialog-ok"
                    text: qsTr("Yes, Update!")
                    Connections {
                        target: acceptCureDeviceUpdate
                        function onClicked() {
                           Statemachine.acceptUpdate();
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                }

                Button {
                    id: acceptCureDeviceIgnore
                    icon.name: "dialog-cancel"
                    text: qsTr("Maybe later.")
                    Connections {
                        target: acceptCureDeviceIgnore
                        function onClicked() {
                           Statemachine.ignoreUpdate();
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                }
            }
        }

        GroupBox {
            title: qsTr("Available Cure Devices")

            Layout.fillWidth: true
            Layout.fillHeight: true

            enabled: !Statemachine.updateInProgress

            ColumnLayout {
                anchors {
                    fill: parent
                    margins: 2
                }

                ListView {
                    id: listView
                    objectName: "AvailableCureDevices"

                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ScrollIndicator.vertical: ScrollIndicator {}

                    delegate: ItemDelegate {
                        id: cureDevicesDelegate
                        width: listView.width

                        anchors {
                            //left: parent.left//listView.left
                           // right: parent.right//listView.right
                        }

                        RowLayout {
                            id: deviceRow
                            anchors {
                                fill: parent
                                margins: 0
                            }
                            Text {

                                text: '<b>' + DeviceName + '<b>'
                                Layout.fillWidth: true
                                padding: 2
                            }

                            Button {
                                visible: isBatInfoAvailable
                                icon.source: BatIconName
                                flat: true
                            }

                            Button {
                                id: connectButton
                                visible: Uart.disconnected
                                icon.name: "network-connect"
                                text: qsTr("Connect")
                                Connections {
                                    target: connectButton
                                    function onClicked() {
                                        Uart.startConnect(DeviceId)
                                    }
                                }
                            }

                            Button {
                                id: disconnectButton
                                visible: isConnectedToCurrentItem
                                icon.name: "network-disconnect"
                                text: qsTr("Disconnect")

                                Connections {
                                    target: disconnectButton
                                    //disconnect resets the view => the item cannot access uart anymore => we need to call both functions (disconnect&startScan) from c++
                                    function onClicked () {
                                        Uart.disconnectAndRescan()
                                    }
                                }
                            }
                        }
                    }
                }

                RowLayout {
                    Layout.fillWidth: true

                    Rectangle {
                        Layout.fillWidth: true
                    }

                    Button {
                        id: startBleScanButton
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        enabled: Uart.disconnected
                        icon.name: "view-refresh"

                        Connections {
                            target: startBleScanButton
                            function onClicked() {
                                Uart.startScan()
                            }
                        }
                    }

                    Rectangle {
                        Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }
}
