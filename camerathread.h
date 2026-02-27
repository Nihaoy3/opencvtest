#ifndef CAMERATHREAD_H
#define CAMERATHREAD_H

#include <QThread>
#include <QImage>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>

class CameraThread : public QThread
{
    Q_OBJECT
public:
    explicit CameraThread(QObject *parent = nullptr);
    ~CameraThread() override;

    void stop();
    void setBeautyLevel(int level);
    void setSharpLevel(float level);
    void setBeautyEnabled(bool enabled);

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

    QMutex m_paramMutex;
    int m_beautyLevel = 3;
    float m_sharpLevel = 0.2f;
    bool m_beautyEnabled = true;
};

#endif
