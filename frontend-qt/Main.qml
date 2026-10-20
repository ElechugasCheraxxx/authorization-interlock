import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "theme"
import "panels"

ApplicationWindow {
    id: root
    visible: true
    width: 1440
    height: 900
    title: "Authorization Interlock"
    color: Theme.bgDark

    Component.onCompleted: {
        bridge.boot()
    }

    // Global Theme singleton access
    readonly property var theme: Theme

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // 1. Sidebar (Navigation)
        Sidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: Theme.sidebarWidth
        }

        // 2. Main Content Area
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0

            // Top Bar (Operation Status)
            TopBar {
                Layout.fillWidth: true
                Layout.preferredHeight: Theme.topBarHeight
            }

            // Grid Content
            Item {
                Layout.fillWidth: true
                Layout.fillHeight: true

                GridLayout {
                    anchors.fill: parent
                    anchors.margins: Theme.padding
                    columns: 3
                    rows: 3
                    columnSpacing: Theme.spacing
                    rowSpacing: Theme.spacing

                    // Top Row
                    OperationBrief {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.rowSpan: 1
                    }

                    CipherCommandCenter {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.columnSpan: 1
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        spacing: Theme.spacing

                        CommunicationFeed {
                            Layout.fillWidth: true
                            Layout.preferredHeight: parent.height * 0.4
                        }
                        HardwareMonitor {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                        }
                    }

                    // Middle Row
                    BlackboardLiveView {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        Layout.columnSpan: 1
                    }

                    AgentWorkspace {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    EvidenceVault {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                    }

                    // Bottom Row (Timeline and Controls)
                    OperationTimeline {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        Layout.columnSpan: 2
                    }

                    AutonomyControls {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                    }
                }
            }
        }
    }
}
