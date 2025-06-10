import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtModelsToolkit 1.0

Item {
    id: root
    ListModel {
        id: listModel
        Component.onCompleted: {
            for (var i = 1; i < 1000; ++i) {
                listModel.append({
                    name: "Item " + i,
                    key: i
                })
            }
        }
    }

    ListModel {
        id: listModel2
        Component.onCompleted: {
            for (var i = 10; i < 50; ++i) {
                listModel2.append({
                    name: "Item " + i,
                    key: i
                })
            }
        }
    }
    RowLayout {
        anchors.fill: parent
        spacing: 20
        ColumnLayout {
            Layout.maximumWidth: root.width / 2
            Layout.fillHeight: true
            ListView {
                id: listView
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: listModel
                clip: true

                delegate: MouseArea {
                    id: dragArea

                    property bool held: false
                    readonly property int idx: model.index

                    anchors {
                        left: parent ? parent.left : undefined
                        right: parent ? parent.right : undefined
                    }
                    height: content.implicitHeight

                    drag.target: held ? content : undefined
                    drag.axis: Drag.YAxis

                    onPressAndHold: held = true
                    onReleased: held = false

                    onClicked: {
                        listModel.setProperty(index, "name", listModel.get(index).name + "!")
                    }

                    RowLayout {
                        id: content

                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            verticalCenter: parent.verticalCenter
                        }

                        width: dragArea.width

                        Drag.active: dragArea.held
                        Drag.source: dragArea
                        Drag.hotSpot.x: width / 2
                        Drag.hotSpot.y: height / 2

                        states: State {
                            when: dragArea.held

                            ParentChange { target: content; parent: root }
                            AnchorChanges {
                                target: content
                                anchors {
                                    horizontalCenter: undefined
                                    verticalCenter: undefined
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true

                            font.bold: true
                            text: model.name + ", key: " + model.key
                        }

                        Button {
                            text: "⬆️"
                            enabled: index > 0

                            onClicked: listModel.move(index, index - 1, 1)
                        }
                        Button {
                            text: "⬇️"
                            enabled: index < listView.count - 1

                            onClicked: listModel.move(index, index + 1, 1)
                        }
                    }

                    DropArea {
                        anchors { fill: parent; margins: 10 }

                        onEntered: {
                            const from = drag.source.idx
                            const to = dragArea.idx

                            if (from === to)
                                return

                            listModel.move(from, to, 1)
                        }
                    }
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.preferredHeight: 100

                RowLayout{
                    Button {
                        text: "Insert at"
                        onClicked: {
                            listModel.insert(parseInt(insertIndex.text),{
                                name: "Item " + (listModel.count + 1),
                                key: listModel.count + 1
                            })
                        }
                    }
                    TextField {
                        id: insertIndex
                        text: "0"
                        cursorVisible: false
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                }
                RowLayout{
                    Button {
                        text: "Remove at"
                        onClicked: {
                            listModel.remove(parseInt(removeIndex.text), 1)
                        }
                    }
                    TextField {
                        id: removeIndex
                        text: "0"
                        cursorVisible: false
                        inputMethodHints: Qt.ImhDigitsOnly
                    }
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Label {
                Layout.alignment: Qt.AlignLeft | Qt.AlignTop
                Layout.fillWidth: true
                text: writableProxyModel.dirty ? "Dirty" : "Clean"
                Rectangle {
                    anchors.fill: parent
                    color: "transparent"
                    border.color: writableProxyModel.dirty ? "red" : "green"
                    border.width: 2
                }
            }
            ListView {
                id: listViewWritable
                Layout.preferredWidth: implicitWidth
                Layout.fillHeight: true
                clip: true
                implicitWidth: 300
                model: WritableProxyModel {
                    id: writableProxyModel
                    sourceModel: listModel
                }

                delegate: MouseArea {
                    id: draggableDelegate

                    property bool held: false
                    readonly property int idx: model.index

                    anchors {
                        left: parent ? parent.left : undefined
                        right: parent ? parent.right : undefined
                    }
                    height: content.implicitHeight

                    onClicked: {
                        if (mouse.button === Qt.LeftButton)
                            model.name = model.name + "!"
                        else
                            model.name = model.name.substr(0, model.name.length - 1)
                    }

                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    RowLayout {
                        id: content

                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            verticalCenter: parent.verticalCenter
                        }

                        width: draggableDelegate.width

                        Label {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 32

                            font.bold: true
                            text: model.name + ", key: " + model.key
                        }
                    }
                }
            }

            RowLayout{
                Button {
                    text: "Insert at"
                    onClicked: {
                        writableProxyModel.insert(parseInt(insertWritableIndex.text), {
                            name: "Item " + (writableProxyModel.rowCount() + 1),
                            key: writableProxyModel.rowCount() + 1
                        })
                    }
                }
                TextField {
                    id: insertWritableIndex
                    text: "0"
                    cursorVisible: false
                    inputMethodHints: Qt.ImhDigitsOnly
                }
            }
            RowLayout{
                Button {
                    text: "Append"
                    onClicked: {
                        writableProxyModel.append({
                            name: "Item " + (writableProxyModel.rowCount() + 1),
                            key: writableProxyModel.rowCount() + 1
                        })
                    }
                }
            }
            RowLayout{
                Button {
                    text: "Remove from"
                    onClicked: {
                        writableProxyModel.remove(parseInt(removeWritableIndex.text))
                    }
                }
                TextField {
                    id: removeWritableIndex
                    text: "0"
                    cursorVisible: false
                    inputMethodHints: Qt.ImhDigitsOnly
                }
            }
            RowLayout {
                Button {
                    text: "Swap models"
                    onClicked: {
                        if (writableProxyModel.sourceModel === listModel) {
                            writableProxyModel.sourceModel = listModel2
                        } else {
                            writableProxyModel.sourceModel = listModel
                        }
                    }
                }
                Button {
                    text: "Revert"
                    onClicked: {
                        writableProxyModel.revert()
                    }
                }
            }
        }
    }
}
