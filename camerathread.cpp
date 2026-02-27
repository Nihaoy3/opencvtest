#include "camerathread.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QStringList>

namespace {

std::vector<cv::Rect> detectFaces(cv::dnn::Net &net, const cv::Mat &frame)
{
    std::vector<cv::Rect> faces;
    if (net.empty() || frame.empty()) {
        return faces;
    }

    cv::Mat blob = cv::dnn::blobFromImage(
        frame,
        1.0,
        cv::Size(300, 300),
        cv::Scalar(104.0, 177.0, 123.0),
        false,
        false);

    net.setInput(blob);
    cv::Mat output = net.forward();
    cv::Mat detection(output.size[2], output.size[3], CV_32F, output.ptr<float>());

    for (int i = 0; i < detection.rows; ++i) {
        const float confidence = detection.at<float>(i, 2);
        if (confidence < 0.62f) {
            continue;
        }

        int x1 = static_cast<int>(detection.at<float>(i, 3) * frame.cols);
        int y1 = static_cast<int>(detection.at<float>(i, 4) * frame.rows);
        int x2 = static_cast<int>(detection.at<float>(i, 5) * frame.cols);
        int y2 = static_cast<int>(detection.at<float>(i, 6) * frame.rows);

        cv::Rect face = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2))
                        & cv::Rect(0, 0, frame.cols, frame.rows);
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
        cv::inRange(yCrCb, cv::Scalar(0, 130, 80), cv::Scalar(255, 180, 135), skinInFace);

        cv::Mat ellipseMask = cv::Mat::zeros(face.size(), CV_8UC1);
        cv::ellipse(
            ellipseMask,
            cv::Point(face.width / 2, face.height / 2),
            cv::Size(static_cast<int>(face.width * 0.45), static_cast<int>(face.height * 0.58)),
            0,
            0,
            360,
            cv::Scalar(255),
            -1);

        cv::bitwise_and(skinInFace, ellipseMask, skinInFace);
        cv::morphologyEx(
            skinInFace,
            skinInFace,
            cv::MORPH_OPEN,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5)));

        cv::GaussianBlur(skinInFace, skinInFace, cv::Size(0, 0), 6.0);

        cv::Mat mergedFaceMask;
        cv::max(mask(face), skinInFace, mergedFaceMask);
        mergedFaceMask.copyTo(mask(face));
    }

    return mask;
}

cv::Mat applyBeautyInLab(const cv::Mat &frame, const cv::Mat &skinMask, int beautyLevel)
{
    cv::Mat lab;
    cv::cvtColor(frame, lab, cv::COLOR_BGR2Lab);

    std::vector<cv::Mat> channels;
    cv::split(lab, channels);

    const int diameter = 7 + beautyLevel * 2;
    const double sigma = 22.0 + beautyLevel * 10.0;

    cv::Mat smoothL;
    cv::bilateralFilter(channels[0], smoothL, diameter, sigma, sigma);

    cv::Mat maskF;
    skinMask.convertTo(maskF, CV_32FC1, 1.0 / 255.0);
    const float blendStrength = std::min(0.9f, 0.25f + beautyLevel * 0.06f);
    maskF *= blendStrength;

    cv::Mat lSrcF;
    cv::Mat lSmoothF;
    channels[0].convertTo(lSrcF, CV_32FC1);
    smoothL.convertTo(lSmoothF, CV_32FC1);

    cv::Mat lOutF = lSrcF.mul(1.0f - maskF) + lSmoothF.mul(maskF);
    lOutF.convertTo(channels[0], CV_8UC1);

    cv::merge(channels, lab);

    cv::Mat beautified;
    cv::cvtColor(lab, beautified, cv::COLOR_Lab2BGR);
    return beautified;
}

QString locateModel(const QString &name)
{
    const QString appDir = QCoreApplication::applicationDirPath();

    QStringList roots;
    roots << appDir << QDir::currentPath();

#ifdef APP_SOURCE_DIR
    roots << QStringLiteral(APP_SOURCE_DIR);
#endif

    QDir walker(appDir);
    for (int i = 0; i < 6; ++i) {
        roots << walker.absolutePath();
        if (!walker.cdUp()) {
            break;
        }
    }

    for (const QString &root : roots) {
        const QString candidate = QDir(root).filePath(name);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return QString();
}

} // namespace

CameraThread::CameraThread(QObject *parent)
    : QThread(parent), m_quit(false)
{
    const QString modelProto = locateModel("deploy.prototxt");
    const QString modelWeight = locateModel("res10_300x300_ssd_iter_140000.caffemodel");

    m_model_proto = modelProto.toStdString();
    m_model_weight = modelWeight.toStdString();

    if (modelProto.isEmpty() || modelWeight.isEmpty()) {
        qDebug() << "❌ 未找到人脸检测模型文件，已关闭人脸磨皮功能。"
                 << "proto=" << modelProto << "weight=" << modelWeight;
        return;
    }

    try {
        m_net = cv::dnn::readNetFromCaffe(m_model_proto, m_model_weight);
    } catch (const cv::Exception &e) {
        qDebug() << "❌ 读取 DNN 模型失败：" << e.what();
        return;
    }

    if (m_net.empty()) {
        qDebug() << "❌ 无法加载 DNN 模型：" << modelProto << modelWeight;
    } else {
        qDebug() << "✅ SSD 深度学习模型加载成功：" << modelProto;
    }
}

CameraThread::~CameraThread()
{
    stop();
    wait();
}

void CameraThread::stop()
{
    QMutexLocker locker(&m_paramMutex);
    m_quit = true;
}

void CameraThread::setBeautyLevel(int level)
{
    QMutexLocker locker(&m_paramMutex);
    m_beautyLevel = level;
}

void CameraThread::setSharpLevel(float level)
{
    QMutexLocker locker(&m_paramMutex);
    m_sharpLevel = level;
}

void CameraThread::setBeautyEnabled(bool enabled)
{
    QMutexLocker locker(&m_paramMutex);
    m_beautyEnabled = enabled;
}

void CameraThread::run()
{
    if (!m_cap.open(0, cv::CAP_V4L2)) {
        return;
    }

    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame;

    while (true) {
        {
            QMutexLocker locker(&m_paramMutex);
            if (m_quit) {
                break;
            }
        }

        m_cap >> frame;
        if (frame.empty()) {
            continue;
        }

        int beautyLevel;
        float sharpLevel;
        bool beautyEnabled;

        {
            QMutexLocker locker(&m_paramMutex);
            beautyLevel = std::max(0, std::min(m_beautyLevel, 10));
            sharpLevel = std::max(0.0f, std::min(m_sharpLevel, 1.0f));
            beautyEnabled = m_beautyEnabled;
        }

        cv::Mat final = frame.clone();
        std::vector<cv::Rect> faces = detectFaces(m_net, frame);

        if (beautyEnabled && beautyLevel > 0 && !faces.empty()) {
            cv::Mat skinMask = buildSkinMask(frame, faces);
            final = applyBeautyInLab(frame, skinMask, beautyLevel);
        }

        cv::Mat soft;
        cv::GaussianBlur(final, soft, cv::Size(0, 0), 1.2);
        const double sharpenAmount = sharpLevel * 0.55;
        cv::addWeighted(final, 1.0 + sharpenAmount, soft, -sharpenAmount, 0, final);

        cv::Mat rgbFrame;
        cv::cvtColor(final, rgbFrame, cv::COLOR_BGR2RGB);
        QImage img(
            rgbFrame.data,
            rgbFrame.cols,
            rgbFrame.rows,
            rgbFrame.step,
            QImage::Format_RGB888);

        emit frameReady(img.copy());
        msleep(10);
    }

    m_cap.release();
}
