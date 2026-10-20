import QtQuick
import QtQuick.Controls
import "../../theme"

Rectangle {
    id: root
    color: Theme.bgCard
    border.color: Theme.border
    border.width: 1
    radius: 4

    property alias text: textArea.text
    property string title: "INTERLOCK SCRIPT"

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 30
            color: Theme.bgLighter
            radius: 4

            Text {
                text: root.title
                anchors.centerIn: parent
                color: Theme.fgMain
                font.pixelSize: 11
                font.bold: true
                font.family: "JetBrains Mono"
            }
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            Flickable {
                anchors.fill: parent
                contentWidth: textArea.width
                contentHeight: textArea.height

                ChrxHighlighter {
                    id: highlighter
                    text: textArea.text
                }

                TextEdit {
                    id: textArea
                    padding: 10
                    color: "transparent"
                    selectionColor: Theme.accent
                    selectedTextColor: "white"
                    font.family: "JetBrains Mono"
                    font.pixelSize: 13
                    wrapMode: TextEdit.NoWrap
                    selectByMouse: true

                    cursorDelegate: Rectangle {
                        width: 2
                        color: Theme.accent
                        visible: textArea.activeFocus
                        Timer {
                            interval: 500
                            repeat: true
                            running: true
                            onTriggered: parent.visible = !parent.visible
                        }
                    }

                    // The actual colored text layer
                    Text {
                        anchors.fill: parent
                        padding: 10
                        text: highlighter.highlightedText
                        color: Theme.fgMain
                        font: textArea.font
                        textFormat: Text.RichText
                        visible: true
                    }
                }
            }
        }
    }
}
