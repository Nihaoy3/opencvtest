#include "camerathread.h"
#include <QDebug>

namespace {

/**
 * @brief 使用 SSD 人脸检测网络定位人脸框（Face ROI）。
 *
 * 术语说明：
 * - ROI (Region of Interest)：感兴趣区域。这里指“人脸区域”。
 * - Blob：DNN 输入张量（通常是 NCHW 排布），由原图归一化/缩放后得到。
 * - Confidence：检测置信度，数值越高表示越像人脸。
 */
std::vector<cv::Rect> detectFaces(cv::dnn::Net &net, const cv::Mat &frame)
{
    std::vector<cv::Rect> faces;

    // 容错：网络未加载成功或帧为空时，直接返回空结果。
    if (net.empty() || frame.empty()) {
        return faces;
    }

    // 1) 预处理：把 BGR 图像变成网络可接受的 blob。
    //    300x300 是该 SSD 模型训练时常见输入尺寸。
    //    Scalar(104,177,123) 是该模型常用的均值（mean subtraction）。
    cv::Mat blob = cv::dnn::blobFromImage(
        frame,
        1.0,
        cv::Size(300, 300),
        cv::Scalar(104.0, 177.0, 123.0),
        false,
        false
    );

    // 2) 前向推理（forward）。
    net.setInput(blob);
    cv::Mat output = net.forward();

    // 3) 解析输出：对 SSD 检测结果重排为 [N,7] 便于读取。
    // 每行通常含义：[image_id, class_id, confidence, x1, y1, x2, y2]
    cv::Mat detection(output.size[2], output.size[3], CV_32F, output.ptr<float>());

    for (int i = 0; i < detection.rows; ++i) {
        float confidence = detection.at<float>(i, 2);

        // 阈值过滤：低置信度的人脸框不要，避免误检导致美颜“糊错区域”。
        if (confidence < 0.6f) {
            continue;
        }

        // 网络输出坐标是相对比例（0~1），这里映射回像素坐标。
        int x1 = static_cast<int>(detection.at<float>(i, 3) * frame.cols);
        int y1 = static_cast<int>(detection.at<float>(i, 4) * frame.rows);
        int x2 = static_cast<int>(detection.at<float>(i, 5) * frame.cols);
        int y2 = static_cast<int>(detection.at<float>(i, 6) * frame.rows);

        // 与图像边界求交集，防止越界。
        cv::Rect face = cv::Rect(cv::Point(x1, y1), cv::Point(x2, y2))
                        & cv::Rect(0, 0, frame.cols, frame.rows);
        if (face.area() > 0) {
            faces.push_back(face);
        }
    }

    return faces;
}

/**
 * @brief 构建“软皮肤掩码”（soft skin mask）。
 *
 * 整体思路：
 * 1) 仅在人脸 ROI 内做皮肤检测（减少背景误判）。
 * 2) 在 YCrCb 颜色空间中做肤色阈值分割。
 * 3) 用椭圆模板约束到脸部中心区域（弱化头发/耳朵/背景干扰）。
 * 4) 形态学开运算去噪，再高斯模糊把边界变“软”（过渡自然）。
 */
cv::Mat buildSkinMask(const cv::Mat &frame, const std::vector<cv::Rect> &faces)
{
    // 单通道掩码，0=不美颜，255=强美颜。
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8UC1);

    for (const cv::Rect &face : faces) {
        // 取出人脸区域。
        cv::Mat faceRoi = frame(face);

        // BGR -> YCrCb。
        // 术语：Y 表示亮度，Cr/Cb 表示色度。肤色在 Cr/Cb 平面常有相对稳定分布。
        cv::Mat yCrCb;
        cv::cvtColor(faceRoi, yCrCb, cv::COLOR_BGR2YCrCb);

        // 基础肤色阈值（经验值区间）。
        cv::Mat skinInFace;
        cv::inRange(yCrCb, cv::Scalar(0, 133, 77), cv::Scalar(255, 173, 127), skinInFace);

        // 椭圆约束：只保留更接近“脸中央”的区域。
        cv::Mat ellipseMask = cv::Mat::zeros(face.size(), CV_8UC1);
        cv::ellipse(
            ellipseMask,
            cv::Point(face.width / 2, face.height / 2),
            cv::Size(static_cast<int>(face.width * 0.42), static_cast<int>(face.height * 0.52)),
            0,
            0,
            360,
            cv::Scalar(255),
            -1
        );

        // 皮肤阈值结果与椭圆模板做“与”运算。
        cv::bitwise_and(skinInFace, ellipseMask, skinInFace);

        // 形态学开运算：先腐蚀后膨胀，清除小噪点。
        cv::morphologyEx(
            skinInFace,
            skinInFace,
            cv::MORPH_OPEN,
            cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5))
        );

        // 高斯模糊：把掩码边缘软化，避免出现“硬边”或色块感。
        cv::GaussianBlur(skinInFace, skinInFace, cv::Size(0, 0), 5.0);

        // 合并回全局掩码：取逐像素最大值（更保守，不会互相抵消）。
        // 这里拆成两步写，避免某些编译环境把 max 误解析为 std::max。
        cv::Mat mergedFaceMask;
        ::cv::max(mask(face), skinInFace, mergedFaceMask);
        mergedFaceMask.copyTo(mask(face));
    }

    return mask;
}

} // namespace

CameraThread::CameraThread(QObject *parent) : QThread(parent), m_quit(false)
{
    // 1) 设置模型路径（当前仍使用固定路径）。
    m_model_proto = "/home/xin/桌面/opcvtest/deploy.prototxt";
    m_model_weight = "/home/xin/桌面/opcvtest/res10_300x300_ssd_iter_140000.caffemodel";

    // 2) 加载 Caffe SSD 人脸检测模型。
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
    // 打开摄像头（V4L2 后端）。
    if (!m_cap.open(0, cv::CAP_V4L2)) return;

    // 目标分辨率：1280x720。
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 1280);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 720);

    cv::Mat frame, smooth, final;

    while (!m_quit) {
        m_cap >> frame;
        if (frame.empty()) continue;

        // 参数钳制（clamp）：防止 UI 传入越界值。
        int beautyLevel = std::max(0, std::min(m_beautyLevel, 10));
        float sharpLevel = std::max(0.0f, std::min(m_sharpLevel, 1.0f));

        // Step A: 检测人脸 ROI。
        std::vector<cv::Rect> faces = detectFaces(m_net, frame);

        // Step B: 准备平滑图（双边滤波 bilateral filter）。
        // 双边滤波优点：同时考虑空间距离和像素差异，能在一定程度上“保边缘降噪”。
        int diameter = 5 + beautyLevel * 2;
        double sigma = 15.0 + beautyLevel * 10.0;
        cv::bilateralFilter(frame, smooth, diameter, sigma, sigma);

        // 默认不做人脸美颜时，先把原图复制给输出。
        final = frame.clone();

        // Step C: 仅在人脸皮肤区域做“按掩码融合”。
        if (!faces.empty() && beautyLevel > 0) {
            cv::Mat skinMask = buildSkinMask(frame, faces);

            // 8-bit 掩码 -> float 掩码，方便做线性混合。
            cv::Mat skinMaskF;
            skinMask.convertTo(skinMaskF, CV_32FC1, 1.0 / 255.0);

            // 控制整体美颜强度上限，避免过度磨皮。
            double blendStrength = std::min(0.85, 0.2 + beautyLevel * 0.07);
            skinMaskF *= blendStrength;

            // 单通道掩码扩展为 3 通道，匹配 BGR 图像。
            cv::Mat skinMask3;
            cv::cvtColor(skinMaskF, skinMask3, cv::COLOR_GRAY2BGR);

            cv::Mat frameF, smoothF;
            frame.convertTo(frameF, CV_32FC3);
            smooth.convertTo(smoothF, CV_32FC3);

            // 线性插值：out = frame*(1-mask) + smooth*mask
            cv::Mat blended = frameF.mul(1.0 - skinMask3) + smoothF.mul(skinMask3);
            blended.convertTo(final, CV_8UC3);
        }

        // Step D: 温和锐化（Unsharp Mask 思想）。
        // 先做轻微高斯模糊，再用 addWeighted 拉回细节。
        cv::Mat soft;
        cv::GaussianBlur(final, soft, cv::Size(0, 0), 1.2);
        double sharpenAmount = sharpLevel * 0.6;
        cv::addWeighted(final, 1.0 + sharpenAmount, soft, -sharpenAmount, 0, final);

        // OpenCV 摄像头帧默认是 BGR 排列。Qt6 支持 QImage::Format_BGR888，
        // 因此这里直接按 BGR 解释，避免额外 cvtColor 和潜在通道抖动。

        // 注意：QImage 此处引用 Mat 内存，因此 emit 前用 copy() 做深拷贝，
        // 避免下一帧覆盖导致显示异常。
        QImage img((const unsigned char*)(final.data), final.cols, final.rows, final.step, QImage::Format_BGR888);
        emit frameReady(img.copy());

        // 简单限速，减少 CPU 占用（约 100 FPS 上限）。
        msleep(10);
    }

    m_cap.release();
}
