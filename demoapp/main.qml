import QtQuick 2.15
import QtQuick.Controls 2.15

ApplicationWindow {
    width: 1024
    height: 768

    visible: true
    title: qsTr("Hello World")

    ListView {
        id: listView

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        width: 200

        model: [
            "ConcatModel",
            "GroupingModel",
            "LeftJoinModel",
            "ModelEntry",
            "MovableModel",
            "MovableModelWithSfpm",
            "ObjectProxyModel",
            "RolesRenamingModel",
            "WritableProxyModel"
        ]

        delegate: Button {
            text: modelData
            width: ListView.view.width

            onClicked: {
                loader.source = modelData + "Page.qml"
            }
        }
    }

    Pane {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: listView.right
        anchors.right: parent.right

        Loader {
            id: loader

            anchors.fill: parent
            source: listView.model[0] + "Page.qml"
        }
    }
}
