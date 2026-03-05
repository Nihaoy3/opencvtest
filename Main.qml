import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import MyOpenCV 1.0

ApplicationWindow {
    width: 1080
    height: 760
    visible: true
    title: "实时美颜控制台"
    color: "#edf1f6"

    property int beautyValue: 4
    property real sharpValue: 0.25
    property bool beautyEnabled: true
    property real whitenValue: 0.2
    property real detectConfidence: 0.62
    property bool overlayEnabled: true

    RowLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 18

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 22
            clip: true
            border.color: "#cbd8e4"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f7fbff" }
                GradientStop { position: 1.0; color: "#eef4fb" }
            }

            Behavior on scale {
                NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
            }

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onEntered: parent.scale = 1.005
                onExited: parent.scale = 1.0
            }

            CvItem {
                id: cameraItem
                anchors.fill: parent
                anchors.margins: 2

                Component.onCompleted: {
                    setBeauty(beautyValue)
                    setSharp(sharpValue)
                    setBeautyEnabled(beautyEnabled)
                    setWhiten(whitenValue)
                    setDetectConfidence(detectConfidence)
                    setOverlayEnabled(overlayEnabled)
                }
            }
        }

        Rectangle {
            Layout.preferredWidth: 340
            Layout.fillHeight: true
            radius: 22
            border.color: "#cfd9e8"
            border.width: 1
            gradient: Gradient {
                GradientStop { position: 0.0; color: "#f8fbff" }
                GradientStop { position: 1.0; color: "#edf3fa" }
            }

            Behavior on opacity {
                NumberAnimation { duration: 220; easing.type: Easing.OutQuad }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 22
                spacing: 16

                Label {
                    text: "美颜参数"
                    color: "#253145"
                    font.pixelSize: 25
                    font.bold: true
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 90

                    Behavior on color {
                        ColorAnimation { duration: 180 }
                    }

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label { text: "启用磨皮"; color: "#324156"; font.pixelSize: 16 }
                            Label { text: beautyEnabled ? "已开启" : "已关闭"; color: "#7b8ba0"; font.pixelSize: 13 }
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

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 90

                    RowLayout {
                        anchors.fill: parent
                        anchors.margins: 14

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 6
                            Label { text: "显示人脸框"; color: "#324156"; font.pixelSize: 16 }
                            Label { text: overlayEnabled ? "已显示" : "已隐藏"; color: "#7b8ba0"; font.pixelSize: 13 }
                        }

                        Switch {
                            checked: overlayEnabled
                            onToggled: {
                                overlayEnabled = checked
                                cameraItem.setOverlayEnabled(checked)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 138

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Label { text: "磨皮强度"; color: "#324156"; font.pixelSize: 16 }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 10
                            stepSize: 1
                            value: beautyValue
                            enabled: beautyEnabled
                            onMoved: {
                                beautyValue = Math.round(value)
                                cameraItem.setBeauty(beautyValue)
                            }
                        }
                        Label {
                            text: beautyValue.toString()
                            color: "#7b8ba0"
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 138

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Label { text: "美白程度"; color: "#324156"; font.pixelSize: 16 }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: whitenValue
                            onMoved: {
                                whitenValue = value
                                cameraItem.setWhiten(value)
                            }
                        }
                        Label {
                            text: whitenValue.toFixed(2)
                            color: "#7b8ba0"
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 138

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Label { text: "人脸检测灵敏度"; color: "#324156"; font.pixelSize: 16 }
                        Slider {
                            Layout.fillWidth: true
                            from: 0.35
                            to: 0.9
                            stepSize: 0.01
                            value: detectConfidence
                            onMoved: {
                                detectConfidence = value
                                cameraItem.setDetectConfidence(value)
                            }
                        }
                        Label {
                            text: detectConfidence.toFixed(2)
                            color: "#7b8ba0"
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    radius: 14
                    color: "#ffffff"
                    border.color: "#d9e3ef"
                    border.width: 1
                    implicitHeight: 138

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 14
                        spacing: 8

                        Label { text: "细节保留"; color: "#324156"; font.pixelSize: 16 }
                        Slider {
                            Layout.fillWidth: true
                            from: 0
                            to: 1
                            stepSize: 0.01
                            value: sharpValue
                            onMoved: {
                                sharpValue = value
                                cameraItem.setSharp(value)
                            }
                        }
                        Label {
                            text: sharpValue.toFixed(2)
                            color: "#7b8ba0"
                            font.pixelSize: 13
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: 14
                    color: "#f8fbff"
                    border.color: "#d9e3ef"
                    border.width: 1

                    Label {
                        anchors.fill: parent
                        anchors.margins: 12
                        wrapMode: Text.WordWrap
                        color: "#74879d"
                        font.pixelSize: 13
                        text: "优化说明：\n1. 新增美白程度、人脸检测灵敏度、人脸框显示等可调参数。\n2. 保持低饱和柔和色调，并延续轻量动画反馈。\n3. 支持在美颜基础上独立微调检测与显示策略。"
                    }
                }
            }
        }
    }
}
