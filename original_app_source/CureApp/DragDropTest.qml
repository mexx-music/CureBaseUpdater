import QtQuick 2.15
import QtQuick.Controls 1.4
//treeview!
import QtQuick.Controls 2.15

import QtQuick.Layouts 1.15
//import QtCharts 2.15
import QtQml.Models 2.1

Page {
    ColumnLayout {
        anchors {
            fill: parent
            margins: 2
        }
ListView{
            id: timeline
            anchors.fill: parent
            orientation: ListView.Vertical
            model: visualModel
            delegate: timelineDelegate

            moveDisplaced: Transition {
                NumberAnimation{
                    properties: "x,y"
                    duration: 200
                }
            }

            DelegateModel {
  //              objectName: "MyProgramsDelegateModel"
                id: visualModel
                model: timelineModel
                delegate: timelineDelegate
            }

            Component {
                id: timelineDelegate


                MouseArea {
                    id: dragArea

                   // width: 100; height: 100
                    anchors { left: parent.left; right: parent.right; margins: 2 }
                    height: content.height

                    property bool held: false

                    drag.target: held ? content : undefined
                    drag.axis: Drag.YAxis

                    onPressAndHold: held = true
                    onReleased: {
                        held = false
                        /*
                        var listOnModel = "{";
                        for(var i = 0; i < timelineModel.count; i++){
                            listOnModel += timelineModel.get(i).colore + ", "
                        }
                        console.log("List: " + listOnModel + "}");*/
                        console.log("released");
                    }

                    Rectangle {
                        id: content
                        height: column.implicitHeight + 4
                        width: dragArea.width

                        anchors { horizontalCenter: parent.horizontalCenter; verticalCenter: parent.verticalCenter }
                        /*width: 100
                        height: 100

                        color: colore
                        opacity: dragArea.held ? 0.8 : 1.0

                        Text{
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: '<b>' + ProgramName + '<b>'
                            font.pixelSize: 20
                        }*/
                        color: dragArea.held ? "lightsteelblue" : "white"

                        RowLayout{
                            id: column
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 6

                            Button {
                                id: playProgram
                                icon.name: "media-playback-start"
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                Text {
                                    id: programNameLabel
                                    text: '<b>' + ProgramName + '<b>'
                                    //padding: 2
                                }

                                RowLayout {

                                    Text {
                                        text:   Math.round(
                                                    ProgramIntensity * 100) + "%"
                                    }

                                    Text {
                                        Layout.fillWidth: true
                                        text:   ""
                                    }

                                    Text {
                                        text: Math.floor(
                                                  ProgramDuration / 60) + ":" + (ProgramDuration % 60).toString(
                                                  ).padStart(2, "0")
                                    }
                                }
                            }
                            Button {
                                id: removeProgramButton
                                icon.name: "edit-delete"
                                onClicked: confirmDeletionDlg.open()
                            }

                            Button {
                                id: editProgram
                                icon.name: "document-properties"
                                onClicked: editPopup.open()
                            }
                        }

                        Drag.active: dragArea.held
                        Drag.source: dragArea
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        states: State{
                            when: dragArea.held
                            ParentChange { target: content; parent: timeline }
                            AnchorChanges {
                                target: content
                                anchors { horizontalCenter: undefined; verticalCenter: undefined }
                            }
                        }
                    }

                    DropArea {
                        anchors.fill: parent
                        onEntered: {
                            visualModel.items.move( drag.source.DelegateModel.itemsIndex, dragArea.DelegateModel.itemsIndex);
                        }
                    }
                }
            }

        }
    }}
