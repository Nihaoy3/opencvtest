#include "cvitem.h"
#include <QPainter>

CvItem::CvItem(QQuickItem *parent) : QQuickPaintedItem(parent)
{
    // 1. 雇佣一个后台线程
    m_thread = new CameraThread(this);

    // 2. 签合同（连接信号与槽）：
    // 当 m_thread 发出 frameReady 信号时，立刻执行我们自己的 updateImage 接收函数
    connect(m_thread, &CameraThread::frameReady, this, &CvItem::updateImage);

    // 3. 告诉后台：开始疯狂干活吧！
    m_thread->start();
}

CvItem::~CvItem()
{
    m_thread->stop();
    m_thread->wait(); // 彻底销毁前等待线程安全退出
}

// 这个函数会在后台每次发出一张新图片时，被自动调用
void CvItem::updateImage(const QImage &image)
{
    m_image = image; // 把前台要展示的图片更新
    update();        // 呼叫 QML 引擎：画面变了，赶紧重绘 (paint)！
}

void CvItem::paint(QPainter *painter)
{
    if (!m_image.isNull()) {
        painter->drawImage(boundingRect().toRect(), m_image);
    }
}
