import QtQuick
import QtQuick.Controls
import "../theme"

Rectangle {
    color: Theme.bgPanel
    border.color: Qt.rgba(1, 1, 1, 0.1)

    Column {
        anchors.fill: parent
        anchors.margins: Theme.padding
        spacing: 10

        Text {
            text: "REPORT FORGE"
            color: Theme.textPrimary
            font.pixelSize: 11
            font.bold: true
            font.family: Theme.monoFont
        }

        Text {
            text: "REPORT: " + (bridge.snapshot.report_name || "REDACTED")
            color: Theme.muted
            font.pixelSize: 9
            font.family: Theme.monoFont
        }

        ProgressBar {
            width: parent.width
            value: bridge.snapshot.report_progress || 0.67
            background: Rectangle { height: 4; color: "#1A1A1C"; radius: 2 }
            contentItem: Item {
                Rectangle {
                    width: parent.visualPosition * parent.width
                    height: 4
                    radius: 2
                    color: Theme.primary
                }
            }
        }

        Column {
            spacing: 5
            Text { text: "✓ Executive Summary"; color: Theme.success; font.pixelSize: 9; font.family: Theme.monoFont }
            Text { text: "✓ Operation Overview"; color: Theme.success; font.pixelSize: 9; font.family: Theme.monoFont }
            Text { text: "✓ Timeline"; color: Theme.success; font.pixelSize: 9; font.family: Theme.monoFont }
            Text { text: "○ Assess Path Analysis"; color: Theme.muted; font.pixelSize: 9; font.family: Theme.monoFont }
        }

        Button {
            width: parent.width
            text: "PREVIEW REPORT"
            contentItem: Text { text: parent.text; color: Theme.primary; font.bold: true; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 10 }
            background: Rectangle { color: "transparent"; border.color: Theme.primary; radius: 4 }
        }
    }
}
