import QtQuick
import QtQuick.Window
import QtQuick.Controls // 👈 必须导入这个，才有 Slider
import MyOpenCV 1.0

Window {
    width: 800
    height: 700 // 稍微加高一点给按钮留位置
    visible: true
    title: "智能美颜控制台"

    CvItem {
        id: cameraItem
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 500 // 画面占上面
    }

    // 👈 控制面板
    Column {
        anchors.top: cameraItem.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 20
        spacing: 10

        // 磨皮强度控制
        Row {
            spacing: 10
            Text { text: "磨皮强度:"; color: "black"; anchors.verticalCenter: parent.verticalCenter }
            Slider {
                from: 0
                to: 10
                stepSize: 1
                value: 3
                onMoved: cameraItem.setBeauty(value) // 👈 滑动时实时调用 C++
            }
        }

        // 锐利度控制
        Row {
            spacing: 10
            Text { text: "清晰保留:"; color: "black"; anchors.verticalCenter: parent.verticalCenter }
            Slider {
                from: 0.0
                to: 1.0
                value: 0.2
                onMoved: cameraItem.setSharp(value) // 👈 滑动时实时调用 C++
            }
        }
    }
}
