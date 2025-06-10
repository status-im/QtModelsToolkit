import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import QtModelsToolkit 1.0

Item {
    id: root

    ListModel {
        id: firstModel

        ListElement {
            name: "entry 1 (1)"
        }
        ListElement {
            name: "entry 2 (1)"
        }
        ListElement {
            name: "entry 3 (1)"
        }
        ListElement {
            name: "entry 4 (1)"
        }
        ListElement {
            name: "entry 5 (1)"
        }
        ListElement {
            name: "entry 6 (1)"
        }
        ListElement {
            name: "entry 7 (1)"
        }
        ListElement {
            name: "entry 8 (1)"
        }
    }

    ListModel {
        id: secondModel

        ListElement {
            name: "entry 1 (2)"
            key: 1
        }
        ListElement {
            key: 2
            name: "entry 2 (2)"
        }
        ListElement {
            key: 3
            name: "entry 3 (2)"
        }
    }

    ConcatModel {
        id: concatModel

        sources: [
            SourceModel {
                model: firstModel
                markerRoleValue: "first_model"
            },
            SourceModel {
                model: secondModel
                markerRoleValue: "second_model"
            }
        ]

        markerRoleName: "which_model"
        expectedRoles: ["key", "name"]
    }

    RowLayout {
        anchors.fill: parent

        ColumnLayout {
            Layout.preferredWidth: parent.width / 3

            ListView {
                id: firstModelListView

                spacing: 15
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true

                model: firstModel

                header: Label {
                    text: "First model"
                    font.pixelSize: 20
                    font.bold: true
                    bottomPadding: 20

                    verticalAlignment: Text.AlignVCenter
                }

                delegate: RowLayout {
                    width: ListView.view.width

                    Label {
                        Layout.fillWidth: true

                        font.bold: true
                        text: model.name

                        MouseArea {
                            anchors.fill: parent

                            onClicked: {
                                firstModel.setProperty(
                                            model.index, "name",
                                            firstModel.get(model.index).name + "_")
                            }
                        }
                    }
                }
            }

            Button {
                text: "append"

                onClicked: {
                    firstModel.append({name: "appended entry (1)"})
                }
            }

            Button {
                text: "insert at 1"

                onClicked: {
                    firstModel.insert(1, {name: "inserted entry (1)"})
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width / 3
            Layout.fillHeight: true

            ListView {
                id: secondModelListView

                spacing: 15
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true

                model: secondModel

                header: Label {
                    text: "Second model"
                    font.pixelSize: 20
                    font.bold: true
                    bottomPadding: 20

                    verticalAlignment: Text.AlignVCenter
                }

                delegate: RowLayout {
                    width: ListView.view.width

                    Label {
                        Layout.fillWidth: true

                        font.bold: true
                        text: model.name

                        MouseArea {
                            anchors.fill: parent

                            onClicked: secondModel.setProperty(
                                           model.index, "name",
                                           secondModel.get(model.index).name + "_")
                        }
                    }
                }
            }

            Button {
                text: "append"

                onClicked: {
                    secondModel.append({name: "appended entry (1)", key: 34})
                }
            }

            Button {
                text: "insert at 1"

                onClicked: {
                    secondModel.insert(1, {name: "inserted entry (1)", key: 999})
                }
            }
        }

        ColumnLayout {
            Layout.preferredWidth: parent.width / 3
            Layout.fillHeight: true

            ListView {
                id: concatListView

                spacing: 15
                clip: true

                Layout.fillWidth: true
                Layout.fillHeight: true

                model: concatModel

                header: Label {
                    text: "Concatenated model"
                    font.pixelSize: 20
                    font.bold: true
                    bottomPadding: 20

                    verticalAlignment: Text.AlignVCenter
                }

                ScrollBar.vertical: ScrollBar {}

                section.property: "which_model"
                section.delegate: ColumnLayout {
                    width: concatListView.width

                    Label {
                        height: implicitHeight * 2
                        text: section + " inset"
                        font.pixelSize: 20
                        font.bold: true

                        verticalAlignment: Text.AlignVCenter

                        Layout.fillWidth: true
                    }
                    CheckBox {
                        text: "some switch inside section delegate"
                        Layout.fillWidth: true
                    }
                }

                delegate: RowLayout {
                    width: ListView.view.width

                    Label {
                        Layout.fillWidth: true

                        readonly property bool second: model.which_model === "second_model"

                        readonly property string baseText:
                            model.name + ", " + model.which_model
                        readonly property string fullText:
                            baseText + (second ? " (" + model.key + ")" : "")

                        font.bold: true
                        text: fullText
                        color: second ? "darkblue" : "darkgreen"
                    }
                }
            }

            Label {
                text: "count:" + concatListView.count
            }
        }
    }
}
