#ifndef CVITEM_H
#define CVITEM_H

#include <QImage>
#include <QQuickPaintedItem>

#include "camerathread.h"

class CvItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit CvItem(QQuickItem *parent = nullptr);
    ~CvItem() override;

    void paint(QPainter *painter) override;

    Q_INVOKABLE void setBeauty(int val) { m_thread->setBeautyLevel(val); }
    Q_INVOKABLE void setSharp(float val) { m_thread->setSharpLevel(val); }
    Q_INVOKABLE void setBeautyEnabled(bool enabled) { m_thread->setBeautyEnabled(enabled); }
    Q_INVOKABLE void setWhiten(float val) { m_thread->setWhitenLevel(val); }
    Q_INVOKABLE void setDetectConfidence(float val) { m_thread->setDetectConfidence(val); }
    Q_INVOKABLE void setOverlayEnabled(bool enabled) { m_thread->setOverlayEnabled(enabled); }

private slots:
    void updateImage(const QImage &image);

private:
    QImage m_image;
    CameraThread *m_thread = nullptr;
};

#endif
