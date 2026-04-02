import QtQuick 2.15
//import QtQuick.Controls 1.4
//treeview!
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQml.Models 2.2

Page {
    property alias availableProgramsDelegateModel: availableProgramsDelegateModel
    property alias availableProgramsView: availableProgramsView
    property alias breadCrumbs: breadCrumbs
    property alias filterButton: filterButton
    property alias filterText: filterText

    id: availableProgramsPage
    width: 600
    height: 400

    header: RowLayout {
        Label {
            text: qsTr("Available Programs")
            font.pixelSize: Qt.application.font.pixelSize * 2
            padding: 10
            Layout.fillWidth: true
        }
        Label {
            id: filterText
            text: AvailablePrograms.filterString
            font.pixelSize: Qt.application.font.pixelSize
            color: "gray"
            padding: 10
            Layout.fillWidth: true
        }
        Button {
            id: filterButton
            icon.name: "edit-find"

            enabled: true
            visible: true

            Layout.rightMargin: 10
            padding: 2
        }
    }

    ColumnLayout {
        anchors {
            fill: parent
            margins: 2
        }

        Flow {
            spacing: 3
            Layout.fillWidth: true
            Layout.rightMargin: 10
            Layout.leftMargin: 10
            Repeater {
                model: ListModel {}
                id: breadCrumbs
                Layout.rightMargin: 10
            }
        }

        ListView {
            objectName: "AvailableProgramsView"
            id: availableProgramsView

            Layout.fillHeight: true
            Layout.fillWidth: true
            Layout.rightMargin: 10

            clip: true

            model: DelegateModel {
                model: AvailablePrograms
                id: availableProgramsDelegateModel
            }
        }
    }
}
