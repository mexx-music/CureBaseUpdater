import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.1


Page {
    property alias myOwnProgramsDelegateModel: myOwnProgramsDelegateModel
    property alias addNewOwnProgram: addNewOwnProgram

    id: myOwnProgramsPage

    header: RowLayout {
        Label {
            Layout.leftMargin: 10
            text: qsTr("My Own Programs")
            font.pixelSize: Qt.application.font.pixelSize * 2
            padding: 2
            Layout.fillWidth: false
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 2
        }

        DelegateModel {
            objectName: "MyProgramsDelegateModel"
            id: myDelegatesModel
            delegate: dragItemDelegate
        }

        ListView {
            objectName: "MyOwnProgramsView"
            id: myOwnProgramsView

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.rightMargin: 10

            clip: true

            model: DelegateModel {
                model: MyOwnPrograms
                id: myOwnProgramsDelegateModel
                delegate: dragItemDelegate

            }

            Component {
                id:dragItemDelegate


                RowLayout {
                    width: myOwnProgramsView.width
                    Layout.fillWidth: true

                    Button {
                        icon.name: "list-add"
                        Layout.alignment: Qt.AlignVCenter
                        onClicked: {
                            MyOwnPrograms.addProgram(ProgramId);
                            if (Settings.switchView) {
                                myProgramsPageTab.clicked();
                                swipeView.setCurrentIndex(0);
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true

                        Text {
                            id: programNameLabel
                            text: '<b>' + ProgramName + '<b>'
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }

                        RowLayout {
                            Layout.fillWidth: true

                            Text {
                                text: Math.round(
                                          ProgramIntensity * 100) + "%"
                            }

                            Text {
                                Layout.fillWidth: true
                                text: ""
                            }

                            Text {
                                text: ProgramDurationText
                            }
                        }
                    }


                    Dialog {
                        id: confirmDeletionDlg
                        anchors.centerIn: parent
                        parent: Overlay.overlay
                        title: qsTr("Delete Program")
                        standardButtons: Dialog.Ok | Dialog.Cancel

                        Connections {
                            target: confirmDeletionDlg

                            function onAccepted() {
                                console.log("Delete item " + index)
                                confirmDeletionDlg.close()
                                MyOwnPrograms.removeProgram(index)
                            }
                            function onRejected() {
                                confirmDeletionDlg.close()
                            }
                        }
                    }

                    Button {
                        id: removeProgramButton
                        icon.name: "edit-delete"
                        Connections {
                            target: removeProgramButton

                            function onClicked() {
                                confirmDeletionDlg.open()
                            }
                        }
                    }

                    Popup {
                        id: editOwnProgramPopup

                        parent: Overlay.overlay
                        anchors.centerIn: parent
                        modal: true
                        focus: true
                        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                        Column {
                            GroupBox {
                                width:parent.width
                                title: qsTr("Name")
                                Rectangle {
                                    anchors.centerIn: parent
                                    width: parent.width
                                    height:edidPopupProgramName.height+4
                                    border.width: 1
                                    TextInput {
                                        id: edidPopupProgramName
                                        width:parent.width-4
                                        anchors.centerIn: parent
                                    }
                                }
                            }

                            GroupBox {
                                width:parent.width
                                title: qsTr("Frequency")
                                Rectangle {
                                    id:edidPopupProgramFrequencyRectangle
                                    width: parent.width
                                    height:edidPopupProgramFrequency.height+4
                                    anchors.centerIn: parent
                                    border.width: 1
                                    TextInput {
                                        id: edidPopupProgramFrequency
                                        width:parent.width-4
                                        anchors.centerIn: edidPopupProgramFrequencyRectangle
                                        //text: "0.123"
                                        validator: DoubleValidator {
                                            top:10000.0
                                            bottom:0.01
                                            decimals:2
                                            notation: DoubleValidator.StandardNotation
                                        }
                                    }
                                }


                            }

                            Label {
                                text: qsTr("Duration")
                            }

                            Row {
                                Slider {
                                    id: editPopupDurationSlider
                                    value: 5
                                    from: 5
                                    to: 3 * 60
                                    stepSize: 5
                                    padding: 2

                                    Connections {
                                        target: editPopupDurationSlider

                                        function onMoved() {
                                            editPopupDurationText.text = qmlHelpers.formatDuration(
                                                        editPopupDurationSlider.value)
                                        }
                                    }
                                }
                                Text {
                                    id: editPopupDurationText
                                    text: "1"
                                    padding: 2
                                }
                            }

                            Label {
                                text: qsTr("Intensity")
                            }

                            Row {
                                id: editPopupIntensityRow
                                Slider {
                                    id: editPopupIntensitySlider
                                    value: 0
                                    from: 0.1
                                    to: 1.0
                                    stepSize: 0.1
                                    onMoved: editPopupIntensityText.text = Math.floor(
                                                 editPopupIntensitySlider.value * 100.0) + "%"
                                    padding: 2
                                }
                                Text {
                                    id: editPopupIntensityText

                                    padding: 2
                                }
                            }

                            GroupBox {
                                title: qsTr("Electric Fields")
                                width:parent.width

                                Row {
                                    CheckBox {
                                        id: editProgramUseEFields
                                        text: qsTr("")
                                        checked: false
                                    }

                                    ComboBox {
                                        id: editProgramEwaveForm
                                        enabled: editProgramUseEFields.checked

                                        model: ListModel {
                                                ListElement {
                                                    text: qsTr("Sine")
                                                    value:0
                                                }
                                                ListElement {
                                                    text: qsTr("Triangle")
                                                    value:1
                                                }
                                                ListElement {
                                                    text: qsTr("Rectangle")
                                                    value:2
                                                }
                                                ListElement {
                                                    text: qsTr("Saw-Tooth")
                                                    value:3
                                                }
                                            }
                                        textRole: "text"
                                        valueRole: "value"
                                    }
                                }
                            }


                            GroupBox {
                                title: qsTr("Magnetic Fields")
                                width:parent.width

                                Row {
                                    CheckBox {
                                        id: editProgramUseHFields
                                        text: qsTr("")
                                        checked: false
                                    }
                                    ComboBox {
                                        id: editProgramHwaveForm
                                        enabled: editProgramUseHFields.checked
                                        model: ListModel {
                                                ListElement {
                                                    text: qsTr("Sine")
                                                    value:0
                                                }
                                                ListElement {
                                                    text: qsTr("Triangle")
                                                    value:1
                                                }
                                                ListElement {
                                                    text: qsTr("Rectangle")
                                                    value:2
                                                }
                                                ListElement {
                                                    text: qsTr("Saw-Tooth")
                                                    value:3
                                                }
                                            }
                                        textRole: "text"
                                        valueRole: "value"
                                    }
                                }
                            }

                            RowLayout {
                                anchors.left: parent.left
                                anchors.right: parent.right

                                Button {
                                    id: editPopupOkButton
                                    icon.name: "dialog-ok"
                                    Layout.alignment: Qt.AlignLeft
                                    Connections {
                                        target: editPopupOkButton

                                        function onClicked() {
                                            ProgramFrequency=edidPopupProgramFrequency.text;
                                            ProgramDuration=editPopupDurationSlider.value;
                                            ProgramIntensity=editPopupIntensitySlider.value;

                                            ProgramUseEFields=editProgramUseEFields.checked;
                                            ProgramUseHFields=editProgramUseHFields.checked;

                                            EWaveForm=editProgramEwaveForm.currentValue;
                                            HWaveForm=editProgramHwaveForm.currentValue;

                                            editOwnProgramPopup.close()

                                            ProgramName=edidPopupProgramName.text; //this resets the model to resort everything
                                            //in case the name got changed. This has the be the last assignment as it triggers a model-reset!
                                        }
                                    }
                                }

                                Button {
                                    id: editPopupOkCancel
                                    icon.name: "dialog-cancel"
                                    Connections {
                                        target: editPopupOkCancel

                                        function onClicked() {
                                            editOwnProgramPopup.close()
                                        }
                                    }

                                    Layout.alignment: Qt.AlignRight
                                }
                            }
                        }
                    }

                    Button {
                        id: editOwnProgram
                        icon.name: "document-properties"

                        Connections {
                            target: editOwnProgram

                            function onClicked() {
                                edidPopupProgramName.text = ProgramName;
                                edidPopupProgramFrequency.text = ProgramFrequency;
                                editPopupDurationSlider.value = ProgramDuration;
                                editPopupIntensitySlider.value = ProgramIntensity;

                                editProgramUseEFields.checked = ProgramUseEFields;
                                editProgramUseHFields.checked = ProgramUseHFields;

                                editProgramEwaveForm.currentIndex = editProgramEwaveForm.indexOfValue(EWaveForm);
                                editProgramHwaveForm.currentIndex = editProgramHwaveForm.indexOfValue(HWaveForm);

                                editPopupDurationText.text = qmlHelpers.formatDuration(ProgramDuration);
                                editPopupIntensityText.text = Math.floor(ProgramIntensity * 100.0) + "%";

                                editOwnProgramPopup.open();
                            }
                        }
                    }
                }
            }
        }
        Button {
            id: addNewOwnProgram
            visible: true

            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            icon.name: "list-add"
        }
    }
}

