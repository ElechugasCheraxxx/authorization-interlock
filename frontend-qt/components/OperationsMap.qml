import QtQuick
import "../theme"

Rectangle {
    id: mapRoot
    color: "#050505"
    border.color: Theme.accent
    clip: true

    property var transmitters: []
    property var obstacles: []
    property var signalResults: []

    function updateSimulation() {
        if (transmitters.length > 0) {
            signalResults = bridge.simulateSignalPropagation(transmitters, obstacles);
            canvas.requestPaint();
        }
    }

    Canvas {
        id: canvas
        anchors.fill: parent
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);

            // Draw Obstacles
            ctx.fillStyle = "rgba(255, 0, 0, 0.3)";
            for (var i = 0; i < obstacles.length; i++) {
                var obs = obstacles[i];
                ctx.beginPath();
                ctx.arc(obs.x * width, obs.y * height, 10, 0, 2 * Math.PI);
                ctx.fill();
            }

            // Draw Transmitters and Signal Intensity
            for (var j = 0; j < transmitters.length; j++) {
                var t = transmitters[j];
                var strength = signalResults[j] || 0.5;

                // Signal Glow
                var gradient = ctx.createRadialGradient(t.x * width, t.y * height, 0, t.x * width, t.y * height, 50 * strength);
                gradient.addColorStop(0, Qt.rgba(0, 1, 0, 0.4 * strength));
                gradient.addColorStop(1, "transparent");

                ctx.fillStyle = gradient;
                ctx.beginPath();
                ctx.arc(t.x * width, t.y * height, 50 * strength, 0, 2 * Math.PI);
                ctx.fill();

                // Transmitter Core
                ctx.fillStyle = Theme.primary;
                ctx.beginPath();
                ctx.arc(t.x * width, t.y * height, 3, 0, 2 * Math.PI);
                ctx.fill();
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: (mouse) => {
            if (mouse.button === Qt.LeftButton) {
                transmitters.push({x: mouse.x / width, y: mouse.y / height, z: 0.0});
            } else {
                obstacles.push({x: mouse.x / width, y: mouse.y / height, z: 0.0});
            }
            updateSimulation();
        }
    }

    Text {
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.margins: 5
        text: "L: Add Transmitter | R: Add Obstacle (Metal Accelerated)"
        color: Theme.textSecondary
        font.pixelSize: 8
        font.family: Theme.monoFont
    }
}
