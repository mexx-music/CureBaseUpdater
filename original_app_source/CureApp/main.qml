import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    width: 640
    height: 480
    visible: true
    title: qsTr("CureApp")
    onBeforeRendering: {

        if (Uart.initBLE()) {
            enableRadioMessage.visible = false
            tabBar.visible = true
            swipeView.visible = true
        } else {

            enableRadioMessage.visible = true
            tabBar.visible = false
            swipeView.visible = false
        }

    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex


        MyProgramsForm {
            enabled: !Statemachine.updateInProgress
        }



        AvailableProgramsForm {
            enabled: !Statemachine.updateInProgress

            breadCrumbs {
                id:breadCrumbs
                delegate: Button {
                    id: breadcrumbItem
                    text: breadcrumbText
                    enabled: breadcrumbButtonIsEnabled
                    flat: breadcrumbItemIsFlat
                    icon.name: breadcrumbIcon
                    font.capitalization: Font.MixedCase
                    onClicked: {
                        //root-entry has a icon only...
                        var startValue=breadCrumbs.model.count-1;
                        if (startValue==index)
                            startValue++;

                        //traverse back
                        for (var i=startValue; i>index;i--) {
                            availableProgramsDelegateModel.rootIndex=availableProgramsDelegateModel.rootIndex.parent;
                        }

                        //do not add any text to the root entry
                        if (index>0) {
                            var BreadCrumb={
                                breadcrumbText:availableProgramsDelegateModel.model.data(availableProgramsDelegateModel.rootIndex,257),
                                breadcrumbIcon:"",
                                breadcrumbButtonIsEnabled:false,
                                breadcrumbItemIsFlat:true
                            };

                            breadCrumbs.model.append(BreadCrumb);

                            //remove the old items
                            breadCrumbs.model.remove(index,breadCrumbs.model.count-index-1);
                        } else {
                            //for the root-item, remove all...
                            breadCrumbs.model.remove(index,breadCrumbs.model.count);
                        }



                    }
                }

            }

            property var editFilterDialog: Dialog {
                id: editFilterDlg
                title: qsTr("Edit Filter")
                standardButtons: Dialog.Ok | Dialog.Cancel
                focus:true

                onAccepted: {
                    AvailablePrograms.filterString=filterText.text
                    close()
                }
                onRejected: {
                    close()
                }
                onOpened: {
                    filterText.text=AvailablePrograms.filterString
                    filterText.focus=true
                }

                anchors.centerIn: parent
                parent: Overlay.overlay

                contentItem: Rectangle {
                    id:textRectangle
                    width: parent.width
                    height:filterText.height+4
                    border.width: 1
                    TextInput {
                        anchors.centerIn: textRectangle
                        width: parent.width-4
                        id:filterText
                        focus: true
                    }

                }

            }

            filterButton {
                onClicked: {
                    editFilterDialog.open();
                }
            }

            availableProgramsView{
                id: availableProgramsView
            }

            availableProgramsDelegateModel {
                id:availableProgramsDelegateModel
                items.onChanged: {
                    for (var i = 0; i < availableProgramsDelegateModel.items.count; ++i) {
                        const item = availableProgramsDelegateModel.items.get(i);
                        if (!item.model.ProgramIsVisible) {
                            availableProgramsDelegateModel.items.remove(i);
                            i--;
                        }
                    }

                }

                delegate:
                    RowLayout {
                    id: row1
                    width:availableProgramsView.width

                    Button {
                        id:expandButton
                        Layout.leftMargin: 10
                        icon.name: "expand"
                        Layout.alignment: Qt.AlignVCenter

                        enabled : hasChildren
                        opacity: 1.0*hasChildren

                        onClicked: {
                            if (model.hasModelChildren) {
                                if (breadCrumbs.model.count==0) {
                                    console.log("Adding Home Icon");
                                    breadCrumbs.model.append({
                                                                 breadcrumbText:"",
                                                                 breadcrumbIcon:"home",
                                                                 breadcrumbButtonIsEnabled:true,
                                                                 breadcrumbItemIsFlat:false
                                                             });
                                } else {
                                    const BreadCrumb={
                                        breadcrumbText:availableProgramsView.model.model.data(availableProgramsView.model.rootIndex,257),
                                        breadcrumbIcon:"",
                                        breadcrumbButtonIsEnabled:true,
                                        breadcrumbItemIsFlat:false
                                    };
                                    console.log("Adding BreadCrump for "+BreadCrumb.breadcrumbText);

                                    if (breadCrumbs.model.count>1)
                                        breadCrumbs.model.remove(breadCrumbs.model.count-1,1);

                                    breadCrumbs.model.append(BreadCrumb);
                                }

                                const BreadCrumb={
                                    breadcrumbText:availableProgramsView.model.model.data(availableProgramsView.model.modelIndex(
                                                                                              index),257),
                                    breadcrumbIcon:"",
                                    breadcrumbButtonIsEnabled:false,
                                    breadcrumbItemIsFlat:true
                                };
                                breadCrumbs.model.append(BreadCrumb);


                                availableProgramsView.model.rootIndex
                                        = availableProgramsView.model.modelIndex(
                                            index);

                                availableProgramsView.positionViewAtBeginning();

                            }
                        }
                    }

                    Rectangle {
                        width: progNameText.height
                        height: progNameText.height
                        color: ProgramLevelIndicatorColor
                        radius: 2
                        visible: ProgramLevelIndicatorVisible
                    }

                    Text {
                        id: progNameText
                        text: ProgramName
                        Layout.leftMargin: 10
                        Layout.alignment: Qt.AlignVCenter
                        Layout.fillWidth: true
                        font.bold: true
                    }

                    Button {
                        icon.name: "list-add"
                        Layout.alignment: Qt.AlignVCenter

                        visible:ProgramHasFrequencies
                        onClicked: {
                            availableProgramsDelegateModel.model.addProgram(availableProgramsDelegateModel.modelIndex(index), 258);
                            if (Settings.switchView) {
                                myProgramsPageTab.clicked();
                                swipeView.setCurrentIndex(0);
                            }
                        }
                    }
                    spacing: 10

                }
            }

        }

        MyOwnProgramsForm {
            visible: (Settings.programFilter===2)
            enabled: !Statemachine.updateInProgress

            addNewOwnProgram.onClicked: {
                MyOwnPrograms.insertNewOwnProgram();
            }
        }

        CureDevicesForm {
        }



        SettingsForm {
            enabled: !Statemachine.updateInProgress

            property var clientToDelete: Dialog {
                property string itemToDelete: ""

                id: confirmDeletionDlg
                title: qsTr("Delete Client")
                standardButtons: Dialog.Ok | Dialog.Cancel

                onAccepted: {
                    Settings.removeClient(clientToDeleteName.text)
                    close()
                }
                onRejected: {
                    close()
                }

                anchors.centerIn: parent
                parent: Overlay.overlay


                contentItem: Rectangle {
                    width: parent.width
                    height:filterText.height+4
                    border.width: 0
                    Label {
                        width: parent.width-4
                        id:clientToDeleteName
                        focus: true
                        text:qsTr("New Client")
                    }
                }
            }

            property var addClientDialog: Dialog {
                id: addDlg
                title: qsTr("Add Client")
                standardButtons: Dialog.Ok | Dialog.Cancel

                onAccepted: {
                    Settings.addClient(clientName.text)
                    close()
                }
                onRejected: {
                    close()
                }
                onOpened: {
                    clientName.text=qsTr("New Client")
                }

                anchors.centerIn: parent
                parent: Overlay.overlay

                contentItem: Rectangle {
                    width: parent.width
                    height:filterText.height+4
                    border.width: 1
                    TextInput {
                        width: parent.width-4
                        id:clientName
                        focus: true
                        text:qsTr("New Client")
                    }
                }


            }

            addClientButton.onClicked: {
                addDlg.open();
            }
        }

    }




    Text {
        id: enableRadioMessage
        text: qsTr("Please enable radio to use your CureDevice.")
    }


    footer: TabBar {
        id: tabBar

        currentIndex: swipeView.currentIndex

        TabButton {
            id: myProgramsPageTab
            text: qsTr("My\nPrograms")
        }
        TabButton {
            id: availableProgramsPageTab
            text: qsTr("Available\nPrograms")
        }
        TabButton {
            visible: (Settings.programFilter===2)
            width: (Settings.programFilter===2)?undefined:0
            id: myOwnProgramsPageTab
            text: qsTr("My Own\nPrograms")
        }
        TabButton {
            id: cureDevicesPageTab
            text: qsTr("Devices")
        }
        TabButton {
            id: settingsPageTab
            text: qsTr("Settings")
        }
    }

}
