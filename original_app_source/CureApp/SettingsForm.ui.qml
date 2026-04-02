import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "ComboBoxHacks.js" as CBH

Page {
    id: settingsPage
    width: 600
    height: 400
    property alias clientList: clientList
    property alias addClientButton: addClientButton

    header: Label {
        text: qsTr("Settings")
        font.pixelSize: Qt.application.font.pixelSize * 2
        padding: 10
    }

    ColumnLayout {
        anchors.fill: parent

        RowLayout {
            Label {
                text: qsTr("Program Filter")
            }

            ComboBox {
                id: filterPrograms
                editable: false
                textRole: "text"
                valueRole: "level"
                Component.onCompleted: {
                    width = CBH.calcComboBoxImplicitWidth(filterPrograms);
                    currentIndex=indexOfValue(Settings.programFilter);
                }

                model: ListModel {
                    id: model
                    ListElement {
                        level: 0
                        text: qsTr("Novice")
                    }
                    ListElement {
                        level: 1
                        text: qsTr("Advanced")
                    }
                    ListElement {
                        level: 2
                        text: qsTr("Expert")
                    }
                }

                onActivated: Settings.programFilter=currentValue

            }
        }

        CheckBox {
            id: reconnectCheckbox

            text: qsTr("Reconnect to last Cure Device")

            onCheckStateChanged: Settings.reconnect = checked
            Component.onCompleted: reconnectCheckbox.checked = Settings.reconnect
        }

        CheckBox {
            id: switchViewCheckbox
            text: qsTr("Switch view after adding a programm")

            Component.onCompleted: switchViewCheckbox.checked = Settings.switchView
            onCheckStateChanged: Settings.switchView = checked
        }

        CheckBox {
            id: flashExperimentalFirmware
            visible: false
            text: qsTr("Enable experimental firmware (setting not saved on close)")

            Component.onCompleted: flashExperimentalFirmware.checked
                                   = Settings.flashExperimentalFirmware
            onCheckStateChanged: Settings.flashExperimentalFirmware = checked
        }

        Label {
            text: qsTr("Clients")
            font.pixelSize: Qt.application.font.pixelSize * 2
            padding: 10

            enabled: true
            visible: true
        }

        ListView {
            id: clientList
            Layout.fillWidth: true
            Layout.fillHeight: true
            model: Settings.clients

            //needed; otherwise the upper most item would overflow the header
            clip: true

            Layout.leftMargin: 20
            Layout.rightMargin: 10

            delegate: Rectangle {
                id: content
                required property string modelData
                height: row.implicitHeight
                width: parent.width
                RowLayout {
                    id: row
                    width: parent.width
                    Text {
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        text: content.modelData
                    }
                    Button {
                        id: removeClientButton
                        Layout.alignment: Qt.AlignVCenter
                        icon.name: "edit-delete"
                        Connections {
                            target: removeClientButton
                            function onClicked() {
                                clientToDeleteName.text = content.modelData
                                clientToDelete.open()
                            }
                        }
                    }
                }
            }
        }

        Button {
            id: addClientButton
            visible: true

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            icon.name: "list-add"
        }
    }
}
