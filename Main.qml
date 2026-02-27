import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MyOpenCV 1.0

ApplicationWindow {
    width: 1080
    height: 760
    visible: true
    title: "实时美颜控制台"
    color: "#10131a"

    property int beautyValue: 4
    property real sharpValue: 0.25
    property bool beautyEnabled: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 18
        spacing: 16

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 16
            color: "#1a202c"
            border.color: "#2a3346"
            border.width: 1
            clip: true

            CvItem {
                id: cameraItem
                anchors.fill: parent
                anchors.margins: 1

                Component.onCompleted: {
                    setBeauty(beautyValue)
                    setSharp(sharpValue)
                    setBeautyEnabled(beautyEnabled)
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 320
            Layout.fillHeight: true
            radius: 16
            color: "#171b26"
            border.color: "#2a3346"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 20
                spacing: 16

                Label {
                    text: "美颜参数"
                    color: "#f1f5f9"
                    font.pixelSize: 24
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 12
                    color: "#20283a"
                    border.color: "#334155"
                    border.width: 1
                    implicitHeight: 82

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label { text: "启用磨皮"; color: "#e2e8f0"; font.pixelSize: 16 }
                            Label { text: beautyEnabled ? "已开启" : "已关闭"; color: "#94a3b8"; font.pixelSize: 13 }
                        }

                        Switch {
                            checked: beautyEnabled
                            onToggled: {
                                beautyEnabled = checked
                                cameraItem.setBeautyEnabled(checked)
                            }
                        }
                    }
                }

                Label { text: "磨皮强度"; color: "#e2e8f0"; font.pixelSize: 16 }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 10
                    stepSize: 1
                    value: beautyValue
                    enabled: beautyEnabled
                    onValueChanged: {
                        beautyValue = Math.round(value)
                        cameraItem.setBeauty(beautyValue)
                    }
                }
                Label {
                    text: beautyValue.toString()
                    color: "#94a3b8"
                    font.pixelSize: 13
                }

                Label { text: "细节保留"; color: "#e2e8f0"; font.pixelSize: 16 }
                Slider {
                    Layout.fillWidth: true
                    from: 0
                    to: 1
                    stepSize: 0.01
                    value: sharpValue
                    onValueChanged: {
                        sharpValue = value
                        cameraItem.setSharp(value)
                    }
                }
                Label {
                    text: sharpValue.toFixed(2)
                    color: "#94a3b8"
                    font.pixelSize: 13
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 12
                    color: "#20283a"
                    border.color: "#334155"
                    border.width: 1

                    Label {
                        anchors.fill: parent
                        anchors.margins: 12
                        wrapMode: Text.WordWrap
                        color: "#94a3b8"
                        font.pixelSize: 13
                        text: "优化说明：\n1. 修复蓝色偏色：图像在输出前统一转换为 RGB。\n2. 磨皮仅作用于人脸皮肤区域，并只平滑亮度通道，避免肤色漂移。\n3. 面板改为深色分区布局，参数可视化更清晰。"
                    }
                }
            }
        }
    }
}
