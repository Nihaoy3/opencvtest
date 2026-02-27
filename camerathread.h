#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QImage>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp> // 👈 必须引入 DNN 模块

class CameraThread : public QThread
{
    Q_OBJECT
public:
    explicit CameraThread(QObject *parent = nullptr);
    ~CameraThread() override;
    void stop();

public:
    void setBeautyLevel(int level) { m_beautyLevel = level; }
    void setSharpLevel(float level) { m_sharpLevel = level; }

signals:
    void frameReady(const QImage &image);

protected:
    void run() override;

private:
    cv::VideoCapture m_cap;
    bool m_quit;

    cv::dnn::Net m_net;
    std::string m_model_proto;
    std::string m_model_weight;

    int m_beautyLevel = 3;    // 默认磨皮 3 次
    float m_sharpLevel = 0.2; // 默认原图混合比例 0.2
};

#endif
