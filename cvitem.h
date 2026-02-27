#ifndef CVITEM_H
#define CVITEM_H

#include <QQuickPaintedItem>
#include <QImage>
#include "camerathread.h" // 👈 引入我们的后厨线程

class CvItem : public QQuickPaintedItem
{
    Q_OBJECT
public:
    explicit CvItem(QQuickItem *parent = nullptr);
    ~CvItem();
    void paint(QPainter *painter) override;

    Q_INVOKABLE void setBeauty(int val) { m_thread->setBeautyLevel(val); }
    Q_INVOKABLE void setSharp(float val) { m_thread->setSharpLevel(val); }

private slots:
    // 专门用来接“后厨”端过来图片的槽函数
    void updateImage(const QImage &image);

private:
    QImage m_image;
    CameraThread *m_thread; // 后台线程的老板键
};

#endif // CVITEM_H
