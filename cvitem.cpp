#include "cvitem.h"

#include <QPainter>

CvItem::CvItem(QQuickItem *parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::FramebufferObject);

    m_thread = new CameraThread(this);
    connect(m_thread, &CameraThread::frameReady, this, &CvItem::updateImage);
    m_thread->start();
}

CvItem::~CvItem()
{
    m_thread->stop();
    m_thread->wait();
}

void CvItem::updateImage(const QImage &image)
{
    m_image = image;
    update();
}

void CvItem::paint(QPainter *painter)
{
    if (!m_image.isNull()) {
        painter->drawImage(boundingRect().toRect(), m_image);
    }
}
