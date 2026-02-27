#include "camerathread.h"
#include <QDebug>

CameraThread::CameraThread(QObject *parent) : QThread(parent), m_quit(false)
{
    // 1. 设置模型文件路径
    m_model_proto = "/home/xin/桌面/opcvtest/deploy.prototxt";
    m_model_weight = "/home/xin/桌面/opcvtest/res10_300x300_ssd_iter_140000.caffemodel";

    // 2. 加载深度学习网络
    m_net = cv::dnn::readNetFromCaffe(m_model_proto, m_model_weight);

    if (m_net.empty()) {
        qDebug() << "❌ 无法加载 DNN 模型！请检查文件路径。";
    } else {
        qDebug() << "✅ SSD 深度学习模型加载成功！";
    }
}

CameraThread::~CameraThread() { stop(); wait(); }
void CameraThread::stop() { m_quit = true; }

void CameraThread::run()
{
    if (!m_cap.open(0, cv::CAP_V4L2)) return;
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame, blur, detail, final;

    while (!m_quit) {
        m_cap >> frame;
        if (frame.empty()) continue;
        // 1. 磨皮层（使用变量 m_beautyLevel）
        frame.copyTo(blur);
        for (int i = 0; i < m_beautyLevel; i++) {
            cv::Mat it;
            cv::bilateralFilter(blur, it, 7, 25, 25);
            it.copyTo(blur);
        }

        // 2. 提取细节层（高通滤波）
        // 细节 = 原图 - 高斯模糊图 + 128
        cv::Mat gray;
        cv::GaussianBlur(frame, gray, cv::Size(0, 0), 3);
        detail = frame - gray + 128;

        // 3. 混合（使用变量 m_sharpLevel）
        // 这样皮肤被磨平了，但眼睛和头发的轮廓会因为 detail 层而重新清晰
        // 让 m_sharpLevel 控制原图保留的比例，比例越高越清晰（但也越不美颜）
        cv::addWeighted(blur, 1.0 - m_sharpLevel, frame, m_sharpLevel, 0, final);

        // 4. 略微提升锐度
        cv::Mat kernel = (cv::Mat_<float>(3,3) << 0, -1, 0, -1, 5, -1, 0, -1, 0);
        cv::filter2D(final, final, final.depth(), kernel);

        // --- ----------------------- ---

        cv::cvtColor(final, final, cv::COLOR_BGR2RGB);
        QImage img((const unsigned char*)(final.data), final.cols, final.rows, final.step, QImage::Format_RGB888);
        emit frameReady(img.copy());
        msleep(10);
    }
    m_cap.release();
}
