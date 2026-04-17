import QtQuick
import QtQuick.Layouts
import GC.Proto

Rectangle {
    id: root
    color: "white"

    property double dataXMin: 0
    property double dataXMax: 1
    property double viewXMin: 0
    property double viewXMax: 1
    property int sampleCount: 0

    property double powerMax: 400
    property double hrMax: 200
    property double altMax: 400

    function resetView() {
        viewXMin = dataXMin
        viewXMax = dataXMax
    }
    function startAutoPan() {
        autoPan.running = true
    }

    GridLayout {
        anchors.fill: parent
        anchors.margins: 8
        columns: 2
        rows: 2
        columnSpacing: 0
        rowSpacing: 0

        // --- left: Y axes stacked ----------------------------------
        ColumnLayout {
            Layout.fillHeight: true
            Layout.preferredWidth: 60
            spacing: 0

            AxisItem {
                Layout.fillHeight: true
                Layout.preferredWidth: 60
                orientation: Qt.Vertical
                valueMin: 0
                valueMax: root.powerMax
                tickCount: 5
                label: "W / HR / m"
            }
        }

        // --- plot area ---------------------------------------------
        Item {
            id: plotArea
            Layout.fillWidth: true
            Layout.fillHeight: true

            LineCurveItem {
                id: powerCurve
                anchors.fill: parent
                color: "#c0392b"
                xMin: root.viewXMin
                xMax: root.viewXMax
                yMin: 0
                yMax: root.powerMax
            }
            LineCurveItem {
                id: hrCurve
                anchors.fill: parent
                color: "#2980b9"
                xMin: root.viewXMin
                xMax: root.viewXMax
                yMin: 0
                yMax: root.hrMax
            }
            LineCurveItem {
                id: altCurve
                anchors.fill: parent
                color: "#27ae60"
                xMin: root.viewXMin
                xMax: root.viewXMax
                yMin: 0
                yMax: root.altMax
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                property real dragStartViewXMin: 0
                property real dragStartViewXMax: 0
                property real dragStartMouseX: 0

                onPressed: (m) => {
                    plotArea.forceActiveFocus()
                    dragStartViewXMin = root.viewXMin
                    dragStartViewXMax = root.viewXMax
                    dragStartMouseX = m.x
                }
                onPositionChanged: (m) => {
                    if (!pressed) return
                    const pxSpan = width
                    if (pxSpan <= 0) return
                    const viewSpan = dragStartViewXMax - dragStartViewXMin
                    const dx = m.x - dragStartMouseX
                    const shift = -dx / pxSpan * viewSpan
                    root.viewXMin = dragStartViewXMin + shift
                    root.viewXMax = dragStartViewXMax + shift
                }
                onDoubleClicked: root.resetView()

                onWheel: (w) => {
                    const pxSpan = width
                    if (pxSpan <= 0) return
                    const anchor = root.viewXMin
                                 + (w.x / pxSpan) * (root.viewXMax - root.viewXMin)
                    const factor = w.angleDelta.y > 0 ? 1 / 1.2 : 1.2
                    const newMin = anchor - (anchor - root.viewXMin) * factor
                    const newMax = anchor + (root.viewXMax - anchor) * factor
                    root.viewXMin = newMin
                    root.viewXMax = newMax
                }
            }

            // FPS counter + status
            Text {
                anchors { right: parent.right; top: parent.top; margins: 6 }
                color: "#555"
                font.pixelSize: 11
                text: (frameCounter ? frameCounter.fps.toFixed(1) + " fps   " : "")
                      + "view [" + root.viewXMin.toFixed(0)
                      + ", " + root.viewXMax.toFixed(0)
                      + "]   samples=" + root.sampleCount
                      + "   drag=pan  wheel=zoom  dblclick=reset  space=auto-pan"
            }

            // Continuous pan: shifts viewport every tick so the
            // renderer rebuilds geometry each frame. Space toggles it.
            Timer {
                id: autoPan
                interval: 16
                repeat: true
                running: false
                property real velocity: 2000 // data units per second (scale with ride length)
                onTriggered: {
                    const span = root.viewXMax - root.viewXMin
                    const shift = velocity * (interval / 1000.0)
                    let newMin = root.viewXMin + shift
                    let newMax = root.viewXMax + shift
                    if (newMax > root.dataXMax) {
                        newMin = root.dataXMin
                        newMax = root.dataXMin + span
                    }
                    root.viewXMin = newMin
                    root.viewXMax = newMax
                }
            }

            focus: true
            Keys.onSpacePressed: autoPan.running = !autoPan.running
        }

        // --- bottom-left spacer -------------------------------------
        Item {
            Layout.preferredWidth: 60
            Layout.preferredHeight: 40
        }

        // --- bottom: X axis -----------------------------------------
        AxisItem {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            orientation: Qt.Horizontal
            valueMin: root.viewXMin
            valueMax: root.viewXMax
            tickCount: 8
            label: "time (s)"
        }
    }

}
