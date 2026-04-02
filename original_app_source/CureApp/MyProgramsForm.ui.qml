import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.1

import "mcharts"

Page {
    id: myProgramsPage

    header: RowLayout {

        id: topRowLayout

        Label {
            Layout.leftMargin: 10
            text: qsTr("My Programs")
            font.pixelSize: Qt.application.font.pixelSize * 2
            padding: 2
            Layout.fillWidth: false
        }

        ComboBox {
            id: selectClientComboBox

            model: Settings.clients
            currentIndex: Settings.currentClient

            Connections {
                target: selectClientComboBox
                function onCurrentTextChanged() {
                    Settings.setLastClient(selectClientComboBox.currentText)
                }
            }

            enabled: true
            visible: Settings.clients.length > 0
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            visible: Settings.clients.length === 0
        }

        Button {
            id: playAllPrograms
            Layout.rightMargin: 10
            padding: 2
            icon.name: "media-playlist-play"
            enabled: Statemachine.ready & (!Statemachine.cureProgramRunning)


            display: AbstractButton.IconOnly
            Connections {
                target: playAllPrograms
                function onClicked() {
                    Statemachine.runAllPrograms();
                }
            }
        }
    }

    ColumnLayout {
        id: cureProgramTransfer
        visible: Statemachine.transferCureProgramInProgress
        enabled: Statemachine.transferCureProgramInProgress

        anchors.fill: parent
        anchors {
            margins: 2
        }

        GroupBox {
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillHeight: false
            title: qsTr("Transfer Cure Program")
            Layout.fillWidth: true

            ColumnLayout {
                anchors.fill: parent
                anchors {
                    margins: 2
                }

                Label {
                    Layout.fillWidth: true

                    text: qsTr("transfering CureProgram...")
                    padding: 2

                    horizontalAlignment: Text.AlignHCenter
                }

                ProgressBar {
                    Layout.fillWidth: true

                    from: 0
                    to: 1
                    value: Statemachine.transferCureProgramProgress
                    padding: 2
                }
            }

            Dialog {
                id: cureProgTooLargeDlg
                anchors.centerIn: parent
                parent: Overlay.overlay
                title: qsTr("CureProgram is too large!")
                contentItem: Label {
                    Layout.fillWidth: true

                    text: qsTr("Please reduce the number of programs.")
                    padding: 2

                    horizontalAlignment: Text.AlignHCenter
                }
                standardButtons: Dialog.Ok
            }

            Connections{
                target: Statemachine
                onShowProgrammTooLarge: cureProgTooLargeDlg.visible = true;
            }
        }
    }

    ColumnLayout {
        id: cureProgramPlayback
        visible: Statemachine.cureProgramRunning
        enabled: Statemachine.cureProgramRunning

        anchors.fill: parent
        anchors {
            margins: 2
        }

        GroupBox {
            id: groupBox
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            Layout.fillHeight: false
            title: qsTr("Cure Program Playback")
            Layout.fillWidth: true

            visible: Statemachine.cureProgramRunning
            enabled: Statemachine.cureProgramRunning

            ColumnLayout {

                anchors.fill: parent
                anchors {
                    margins: 2
                }

                Label {
                    id: currentPlaybackCureProgram
                    Layout.fillWidth: true

                    text: Statemachine.currentCureProgramPlayback
                    padding: 2

                    horizontalAlignment: Text.AlignHCenter
                }

                MChart {
                    id: chart1
                    height: 200
                    width: 322
                    Layout.alignment: Qt.AlignHCenter


                    type: MChart.Type.Line

                    labels: dataProvider.labels
                    options.legendDisplay: false
                    //options.padding: 2
                    options.padding: -20

                    options.hasScale: true
                    options.scaleFontColor: "white"
                    options.scaleLineWidth: 0
                    options.tooltipsDisplay: false

                    options.scaleFont {
                        pixelSize: 0
                        family: "sans-serif"
                    }
                    silent: false

                    data: [
                        MDataset {
                            values: dataProvider.elapsedData
                            fillColor: "white"
                            lineColor: "black"
                            borderWidth: 3
                            pointColor: "white"
                            pointRadius: 0
                            pointHitRadius: 0
                        },

                        MDataset {
                            values: dataProvider.remainingData
                            fillColor: "white"
                            lineColor: "lightgray"
                            borderWidth: 3
                            pointColor: "white"
                            pointRadius: 0
                            pointHitRadius: 0
                        }
                    ]
                }

                Label {
                    Layout.fillWidth: true

                    text: 'Fortschritt'
                    padding: 2

                    horizontalAlignment: Text.AlignLeft
                }

                ProgressBar {
                    Layout.fillWidth: true

                    from: 0
                    to: 1
                    value: Statemachine.currentCureProgramPlaybackProgressPercent
                    padding: 2
                }
                Label {
                    Layout.fillWidth: true

                    text: Statemachine.currentCureProgramPlaybackProgress

                    horizontalAlignment: Text.AlignRight
                }

                RowLayout {
                    id: playbackControlLayout

                    Rectangle {
                        Layout.fillWidth: true
                    }

                    Button {
                        id: currentCureProgramResumeButton
                        enabled: Statemachine.currentCureProgramPlaybackCanStart
                        icon.name: "media-playback-start"
                        Connections {
                            target: currentCureProgramResumeButton
                            function onClicked() {
                                Statemachine.resumeProgram();
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                    }

                    Button {
                        id: currentCureProgramPauseButton
                        enabled: Statemachine.currentCureProgramPlaybackCanPause
                        icon.name: "media-playback-pause"
                        Connections {
                            target: currentCureProgramPauseButton
                            function onClicked() {
                                Statemachine.pauseProgram();
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                    }

                    Button {
                        id: currentCureProgramStopButton
                        enabled: Statemachine.currentCureProgramPlaybackCanStop
                        icon.name: "media-playback-stop"
                        Connections {
                            target: currentCureProgramStopButton
                            function onClicked() {
                                Statemachine.terminateProgram();
                            }
                        }
                    }
                    Rectangle {
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    ColumnLayout {
        id: myCurePrograms
        visible: Statemachine.cureProgramIdle

        anchors {
            fill: parent
            margins: 2
        }

        ListView {
            id: listView
            objectName: "MyProgramsView"
            clip: true //needed; otherwise the upper most item would overflow the header

            Layout.fillWidth: true
            Layout.fillHeight: true

            moveDisplaced: Transition {
                NumberAnimation {
                    properties: "x,y"
                    duration: 200
                }
            }

            model: myDelegatesModel

            DelegateModel {
                objectName: "MyProgramsDelegateModel"
                id: myDelegatesModel
                delegate: dragItemDelegate
            }

            Component {
                id:dragItemDelegate
                MouseArea {
                    width: ListView.view.width

                    id: dragArea
                    height: content.height

                    property bool held: false

                    drag.target: held ? content : undefined
                    drag.axis: Drag.YAxis

                    Connections {
                        target: dragArea

                        function onPressAndHold() {
                            dragArea.held = true
                        }
                        function onReleased() {
                            dragArea.held = false
                            for (var i = 0; i < myDelegatesModel.items.count; i++) {
                                //console.log("i: " + i + " => " + myDelegatesModel.items.get(i).model.index)
                                myDelegatesModel.model.setData(
                                            myDelegatesModel.modelIndex(
                                                myDelegatesModel.items.get(
                                                    i).model.index), i, 263)
                            }
                        }
                    }

                    Rectangle {
                        id: content
                        height: column.implicitHeight + 4
                        width: dragArea.width
                        clip: true

                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            verticalCenter: parent.verticalCenter
                        }

                        color: dragArea.held ? ("lightsteelblue") : (OwnProgram ? "lightgray" : "white")


                        RowLayout {
                            id: column
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6

                            Button {
                                id: playProgram
                                icon.name: "media-playback-start"
                                enabled: Statemachine.ready

                                Connections {
                                    target: playProgram

                                    function onClicked() {
                                        Statemachine.runProgram(index);
//                                        listView.runProgramsPopup.run(index)
                                    }
                                }
                            }

                            ColumnLayout {
                                id: programNameColumn
                                Layout.fillWidth: true
                                Text {
                                    id: programNameLabel
                                    text: '<b>' + ProgramName + '<b>'
                                    wrapMode: Text.Wrap
                                    Layout.fillWidth: true
                                }

                                RowLayout {
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
                                        myDelegatesModel.model.removeProgram(index)
                                        confirmDeletionDlg.close()
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
                                id: editPopup

                                parent: Overlay.overlay
                                anchors.centerIn: parent
                                modal: true
                                focus: true
                                closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutsideParent

                                Column {
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

                                        label: CheckBox {
                                            id: editProgramUseEFields
                                            text: qsTr("Use Electric Fields")
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


                                    GroupBox {
                                        label: CheckBox {
                                            id: editProgramUseHFields
                                            text: qsTr("Use Magnetic Fields")
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
                                                    ProgramDuration=editPopupDurationSlider.value;
                                                    ProgramIntensity=editPopupIntensitySlider.value;

                                                    ProgramUseEFields=editProgramUseEFields.checked;
                                                    ProgramUseHFields=editProgramUseHFields.checked;

                                                    EWaveForm=editProgramEwaveForm.currentValue;
                                                    HWaveForm=editProgramHwaveForm.currentValue;

                                                    editPopup.close()
                                                }
                                            }
                                        }

                                        Button {
                                            id: editPopupOkCancel
                                            icon.name: "dialog-cancel"
                                            Connections {
                                                target: editPopupOkCancel

                                                function onClicked() {
                                                    editPopup.close()
                                                }
                                            }

                                            Layout.alignment: Qt.AlignRight
                                        }
                                    }
                                }
                            }

                            Button {
                                id: editProgram
                                icon.name: "document-properties"

                                Connections {
                                    target: editProgram

                                    function onClicked() {
                                        editPopupDurationSlider.value = ProgramDuration;
                                        editPopupIntensitySlider.value = ProgramIntensity;

                                        editProgramUseEFields.checked = ProgramUseEFields;
                                        editProgramUseHFields.checked = ProgramUseHFields;

                                        editProgramEwaveForm.currentIndex = editProgramEwaveForm.indexOfValue(EWaveForm);
                                        editProgramHwaveForm.currentIndex = editProgramHwaveForm.indexOfValue(HWaveForm);

                                        editPopupDurationText.text = qmlHelpers.formatDuration(ProgramDuration);
                                        editPopupIntensityText.text = Math.floor(ProgramIntensity * 100.0) + "%";

                                        editPopup.open();
                                    }
                                }
                            }
                        }

                        Drag.active: dragArea.held
                        Drag.source: dragArea
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        states: State {
                            when: dragArea.held
                            ParentChange {
                                target: content
                                parent: listView
                            }
                            AnchorChanges {
                                target: content
                                anchors {
                                    horizontalCenter: undefined
                                    verticalCenter: undefined
                                }
                            }
                        }
                    }

                    DropArea {
                        id: dropArea
                        anchors.fill: parent

                        Connections {
                            target: dropArea
                            function onEntered(drag) {
                                console.log("move item from " +drag.source.DelegateModel.itemsIndex+" to "+dragArea.DelegateModel.itemsIndex);
                                //myDelegatesModel.moveItems(dragArea.DelegateModel.itemsIndex);

                                myDelegatesModel.items.move(
                                            drag.source.DelegateModel.itemsIndex,
                                            dragArea.DelegateModel.itemsIndex)
                            }
                        }
                    }
                }
            }
        }
    }
}

