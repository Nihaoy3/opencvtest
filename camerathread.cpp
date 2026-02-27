#include "camerathread.h"
#include <QDebug>

namespace {
std::vector<cv::Rect> detectFaces(cv::dnn::Net &net, const cv::Mat &frame)
{
    std::vector<cv::Rect> faces;
    if (net.empty() || frame.empty()) {
        return faces;
    }

    cv::Mat blob = cv::dnn::blobFromImage(frame, 1.0, cv::Size(300, 300),
                                          cv::Scalar(104.0, 177.0, 123.0), false, false);
    net.setInput(blob);
    cv::Mat output = net.forward();
    cv::Mat detection(output.size[2], output.size[3], CV_32F, output.ptr<float>());

    for (int i = 0; i < detection.rows; ++i) {
        float confidence = detection.at<float>(i, 2);
        if (confidence < 0.6f) {
            continue;
        }

        int x1 = static_cast<int>(detection.at<float>(i, 3) * frame.cols);
        int y1 = static_cast<int>(detection.at<float>(i, 4) * frame.rows);
        int x2 = static_cast<int>(detection.at<float>(i, 5) * frame.cols);
        int y2 = static_cast<int>(detection.at<float>(i, 6) * frame.rows);

        cv::Rect face = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2)) & cv::Rect(0, 0, frame.cols, frame.rows);
        if (face.area() > 0) {
            faces.push_back(face);
        }
    }

    return faces;
}

cv::Mat buildSkinMask(const cv::Mat &frame, const std::vector<cv::Rect> &faces)
{
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);
    for (const cv::Rect &face : faces) {
        cv::Mat faceRoi = frame(face);
        cv::Mat yCrCb;
        cv::cvtColor(faceRoi, yCrCb, cv::COLOR_BGR2YCrCb);

        cv::Mat skinInFace;
        cv::inRange(yCrCb, cv::Scalar(0, 133, 77), cv::Scalar(255, 173, 127), skinInFace);

        cv::Mat ellipseMask = cv::Mat::zeros(face.size(), CV_8UC1);
        cv::ellipse(ellipseMask,
                    cv::Point(face.width / 2, face.height / 2),
                    cv::Size(static_cast<int>(face.width * 0.42), static_cast<int>(face.height * 0.52)),
                    0, 0, 360, cv::Scalar(255), -1);

        cv::bitwise_and(skinInFace, ellipseMask, skinInFace);

        cv::morphologyEx(skinInFace, skinInFace, cv::MORPH_OPEN,
                         cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));
        cv::GaussianBlur(skinInFace, skinInFace, cv::Size(0, 0), 5.0);

        cv::max(mask(face), skinInFace, mask(face));
    }

    return mask;
}
}

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

    cv::Mat frame, smooth, final;

    while (!m_quit) {
        m_cap >> frame;
        if (frame.empty()) continue;
        int beautyLevel = std::max(0, std::min(m_beautyLevel, 10));
        float sharpLevel = std::max(0.0f, std::min(m_sharpLevel, 1.0f));

        std::vector<cv::Rect> faces = detectFaces(m_net, frame);

        // 1. 磨皮层：改为一次较强双边滤波，减少反复滤波导致的塑料感
        int diameter = 5 + beautyLevel * 2;
        double sigma = 15.0 + beautyLevel * 10.0;
        cv::bilateralFilter(frame, smooth, diameter, sigma, sigma);

        final = frame.clone();

        if (!faces.empty() && beautyLevel > 0) {
            cv::Mat skinMask = buildSkinMask(frame, faces);
            cv::Mat skinMaskF;
            skinMask.convertTo(skinMaskF, CV_32FC1, 1.0 / 255.0);

            double blendStrength = std::min(0.85, 0.2 + beautyLevel * 0.07);
            skinMaskF *= blendStrength;

            cv::Mat skinMask3;
            cv::cvtColor(skinMaskF, skinMask3, cv::COLOR_GRAY2BGR);

            cv::Mat frameF, smoothF;
            frame.convertTo(frameF, CV_32FC3);
            smooth.convertTo(smoothF, CV_32FC3);

            cv::Mat blended = frameF.mul(1.0 - skinMask3) + smoothF.mul(skinMask3);
            blended.convertTo(final, CV_8UC3);
        }

        // 2. 清晰保留：温和反锐化，只增强边缘不过度发硬
        cv::Mat soft;
        cv::GaussianBlur(final, soft, cv::Size(0, 0), 1.2);
        double sharpenAmount = sharpLevel * 0.6;
        cv::addWeighted(final, 1.0 + sharpenAmount, soft, -sharpenAmount, 0, final);

        // --- ----------------------- ---

        cv::cvtColor(final, final, cv::COLOR_BGR2RGB);
        QImage img((const unsigned char*)(final.data), final.cols, final.rows, final.step, QImage::Format_RGB888);
        emit frameReady(img.copy());
        msleep(10);
    }
    m_cap.release();
}
