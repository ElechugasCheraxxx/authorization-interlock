import QtQuick
import QtQuick.Layouts
import "../theme"
import "../components"
import "../components/chrx"

Rectangle {
    color: Theme.bgPanel
    border.color: Qt.rgba(1, 1, 1, 0.1)

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.padding
        spacing: 10

        Text {
            text: "SOVEREIGN SWARM COORDINATION - .CHRXXX NATIVE"
            color: Theme.primary
            font.pixelSize: 11
            font.bold: true
            font.family: Theme.monoFont
        }

        ChrxEditor {
            Layout.fillWidth: true
            Layout.fillHeight: true
            title: "AGENT REASONING (GHOST-7)"
            text: `mission "SILENT_INFILTRATION" {
    @target: "M4_PRO";

    choreography INFILTRATE {
        phase SCAN:
            bind scanner(port_80);
            await scan_complete;
            assert network_up == true;

        phase EXPLOIT:
            lock(target_buffer);
            call ghost_injector;
            commit(artifact);
    }
}`
        }

        ChoreographyGraph {
            Layout.fillWidth: true
            Layout.preferredHeight: 180
            phases: [
                { name: "SCAN", commands: ["bind scanner", "await scan", "assert net"] },
                { name: "EXPLOIT", commands: ["lock buffer", "call ghost", "commit"] },
                { name: "EXFIL", commands: ["purge logs", "halt", "resume"] }
            ]
        }
    }
}
