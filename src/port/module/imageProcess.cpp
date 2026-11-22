#include "port/module/imageProcess.h"

#include <QJsonArray>
#include <QPainter>
#include <QPixmap>

#include <cmath>

#include "types/portTypes.h"
#include "opencv2/imgproc.hpp"
#include "tesseract/baseapi.h"
#include "tesseract/resultiterator.h"

// public
ImageProcess::ImageProcess() {
    m_ocrEngine = new tesseract::TessBaseAPI();
    const QByteArray charsetBytes = "eng";
    m_ocrEngine->Init(nullptr, charsetBytes.constData());
}

ImageProcess::~ImageProcess() {
    if (m_ocrEngine) {
        m_ocrEngine->Clear();
        m_ocrEngine->End();
        delete m_ocrEngine;
    }
}

QList<ImageProcess::ProcessResult> ImageProcess::process(const QImage &frame) {
    QList<ProcessResult> results{};
    const auto steps = m_config["pipeline"].toArray();
    auto rois = m_config["rois"].toArray();
    if (rois.isEmpty()) rois.append(QJsonArray{0, 0, frame.width(), frame.height()});
    for (const QJsonValue &value: rois) {
        ProcessResult processResult{};
        // roi
        processResult.roiFrame = roi(frame, value.toArray());
        // pipeline
        processResult.pipelineFrame = processResult.roiFrame.convertToFormat(QImage::Format_BGR888);
        if (!steps.isEmpty()) {
            const auto &source = processResult.pipelineFrame;
            const cv::Mat input(source.height(), source.width(), CV_8UC3, const_cast<uchar *>(source.constBits()), source.bytesPerLine());
            const auto output = pipeline(input, steps);
            const auto format = output.channels() == 1 ? QImage::Format_Grayscale8 : QImage::Format_BGR888;
            processResult.pipelineFrame = QImage(output.data, output.cols, output.rows, output.step, format).copy();
        }
        // recognition
        processResult.result = recognition(processResult.pipelineFrame.convertToFormat(QImage::Format_Grayscale8), m_config["recognition"].toObject());
        processResult.result["width"] = processResult.pipelineFrame.width();
        processResult.result["height"] = processResult.pipelineFrame.height();
        // append
        results.append(processResult);
    }
    return results;
}

// private
QImage ImageProcess::roi(const QImage &frame, const QJsonArray &roi) {
    // rect
    if (roi.size() == 4) {
        const auto &rect = QRect(roi[0].toInt(), roi[1].toInt(), roi[2].toInt(), roi[3].toInt());
        return frame.copy(rect);
    }
    // poly
    if (roi.size() == 8) {
        // src poly
        const auto &src = QPolygon{
            QPoint(roi[0].toInt(), roi[1].toInt()),
            QPoint(roi[2].toInt(), roi[3].toInt()),
            QPoint(roi[4].toInt(), roi[5].toInt()),
            QPoint(roi[6].toInt(), roi[7].toInt())
        };
        // dst poly
        const int w = qRound(qMax(QLineF(src[0], src[1]).length(), QLineF(src[2], src[3]).length()));
        const int h = qRound(qMax(QLineF(src[0], src[3]).length(), QLineF(src[1], src[2]).length()));
        const auto &dst = QPolygon{
            QPoint(0, 0),
            QPoint(w, 0),
            QPoint(w, h),
            QPoint(0, h)
        };
        // perform transform
        QTransform transform;
        QTransform::quadToQuad(src, dst, transform);
        QImage result(w, h, frame.format());
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.setTransform(transform);
        painter.drawImage(0, 0, frame);
        return result;
    }
    return {};
}

cv::Mat ImageProcess::pipeline(cv::Mat frame, const QJsonArray &steps) {
    for (const auto &value: steps) {
        const auto step = value.toObject();
        switch (static_cast<ImageOperation::Value>(step["type"].toInt())) {
            case ImageOperation::Scale: {
                const float ratio = static_cast<float>(step["ratio"].toDouble(1));
                const int interpolation = step["interpolation"].toInt();
                frame = scale(frame, ratio, interpolation);
            }
            break;
            case ImageOperation::Rotate:
                frame = rotate(frame, step["angle"].toDouble());
                break;
            case ImageOperation::Flip:
                frame = flip(frame, step["direction"].toInt(1));
                break;
            case ImageOperation::Blur:
                frame = blur(frame, step["method"].toInt(), step["kernelSize"].toInt(3));
                break;
            case ImageOperation::Threshold: {
                const int thresh = step["thresh"].toInt();
                const int maxval = step["maxval"].toInt();
                const int mode = step["mode"].toInt();
                frame = threshold(frame, thresh, maxval, mode);
            }
            break;
            case ImageOperation::ColorMask: {
                const auto hue = step["hue"].toArray();
                const auto saturation = step["saturation"].toArray();
                const auto valueRange = step["value"].toArray();
                frame = colorMask(frame, {hue.at(0).toDouble(), saturation.at(0).toDouble(), valueRange.at(0).toDouble()},
                                  {hue.at(1).toDouble(), saturation.at(1).toDouble(), valueRange.at(1).toDouble()});
            }
            break;
            case ImageOperation::Morphology:
                frame = morphology(frame, step["method"].toInt(cv::MORPH_OPEN), step["kernelSize"].toInt(3));
                break;
            default: break;
        }
    }
    return frame;
}

QVariantHash ImageProcess::recognition(const QImage &pipelineFrame, const QJsonObject &recognition) {
    QVariantHash result{
        {"text", ""},
        {"items", QVariantList{}}
    };
    const auto mode = static_cast<RecognitionMethod::Value>(recognition["mode"].toInt());
    switch (mode) {
        case RecognitionMethod::OCR: {
            m_ocrEngine->SetImage(pipelineFrame.bits(), pipelineFrame.width(), pipelineFrame.height(), 1, pipelineFrame.bytesPerLine());
            m_ocrEngine->Recognize(nullptr);

            char *text = m_ocrEngine->GetUTF8Text();
            result["text"] = QString::fromUtf8(text).trimmed();
            delete[] text;

            QVariantList items{};
            auto *iterator = m_ocrEngine->GetIterator();
            if (iterator) {
                constexpr auto level = tesseract::RIL_WORD;
                do {
                    char *itemText = iterator->GetUTF8Text(level);
                    int left{}, top{}, right{}, bottom{};
                    if (itemText && iterator->BoundingBox(level, &left, &top, &right, &bottom)) {
                        items.append(QVariantHash{
                            {"x", left},
                            {"y", top},
                            {"width", right - left},
                            {"height", bottom - top},
                            {"text", QString::fromUtf8(itemText).trimmed()},
                            {"confidence", iterator->Confidence(level)}
                        });
                    }
                    delete[] itemText;
                } while (iterator->Next(level));
                delete iterator;
            }
            result["items"] = items;
        }
        break;
        case RecognitionMethod::ShiTomasiCorner: {
            const auto point = goodFeaturesToTrack(pipelineFrame);
            if (point != QPoint(-1, -1)) {
                result["text"] = QString("%1,%2").arg(point.x()).arg(point.y());
                result["items"] = QVariantList{QVariantHash{
                    {"x", point.x()}, {"y", point.y()}, {"width", 0}, {"height", 0}
                }};
            }
        }
        break;
        case RecognitionMethod::HarrisCorner: {
            const auto point = cornerHarris(pipelineFrame);
            if (point != QPoint(-1, -1)) {
                result["text"] = QString("%1,%2").arg(point.x()).arg(point.y());
                result["items"] = QVariantList{QVariantHash{
                    {"x", point.x()}, {"y", point.y()}, {"width", 0}, {"height", 0}
                }};
            }
        }
        break;
        case RecognitionMethod::TemplateMatch: {
            const auto templateUrl = recognition["template"].toString();
            if (m_templateUrl != templateUrl) {
                m_templateUrl = templateUrl;
                m_template = QImage(QUrl(templateUrl).toLocalFile()).convertToFormat(QImage::Format_Grayscale8);
            }
            const QPoint point = templateMatch(pipelineFrame, m_template);
            if (point != QPoint(-1, -1)) {
                result["text"] = QString("%1,%2").arg(point.x()).arg(point.y());
                result["items"] = QVariantList{QVariantHash{
                    {"x", point.x()}, {"y", point.y()}, {"width", 0}, {"height", 0}
                }};
            }
        }
        break;
        default: break;
    }
    return result;
}

// pipeline
cv::Mat ImageProcess::scale(const cv::Mat &input, const float ratio, const int interpolation) {
    const int newWidth = static_cast<int>(input.cols * ratio);
    const int newHeight = static_cast<int>(input.rows * ratio);
    cv::Mat output{};
    cv::resize(input, output, cv::Size(newWidth, newHeight), 0, 0, interpolation);
    return output;
}

cv::Mat ImageProcess::rotate(const cv::Mat &input, const double angle) {
    if (angle == 0) return input;
    cv::Mat output{};
    if (angle == 90 || angle == -90 || std::abs(angle) == 180) {
        const auto direction = angle == 90 ? cv::ROTATE_90_COUNTERCLOCKWISE : angle == -90 ? cv::ROTATE_90_CLOCKWISE : cv::ROTATE_180;
        cv::rotate(input, output, direction);
        return output;
    }

    const cv::Point2f center((input.cols - 1) * 0.5f, (input.rows - 1) * 0.5f);
    auto matrix = cv::getRotationMatrix2D(center, angle, 1);
    const double cosine = std::abs(matrix.at<double>(0, 0));
    const double sine = std::abs(matrix.at<double>(0, 1));
    const cv::Size size(static_cast<int>(std::ceil(input.cols * cosine + input.rows * sine)),
                        static_cast<int>(std::ceil(input.cols * sine + input.rows * cosine)));
    // Center the rotated image in the expanded canvas.
    matrix.at<double>(0, 2) += (size.width - input.cols) * 0.5;
    matrix.at<double>(1, 2) += (size.height - input.rows) * 0.5;
    cv::warpAffine(input, output, matrix, size, cv::INTER_LINEAR);
    return output;
}

cv::Mat ImageProcess::flip(const cv::Mat &input, const int direction) {
    cv::Mat output{};
    cv::flip(input, output, direction);
    return output;
}

cv::Mat ImageProcess::blur(const cv::Mat &input, const int method, const int kernelSize) {
    cv::Mat output{};
    if (method == 0) cv::GaussianBlur(input, output, cv::Size(kernelSize, kernelSize), 0);
    else cv::medianBlur(input, output, kernelSize);
    return output;
}

cv::Mat ImageProcess::threshold(const cv::Mat &input, const int thresh, const int maxval, const int mode) {
    cv::Mat output{};
    if (input.channels() != 1) {
        cv::Mat gray{};
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
        cv::threshold(gray, output, thresh, maxval, mode);
    } else {
        cv::threshold(input, output, thresh, maxval, mode);
    }
    return output;
}

cv::Mat ImageProcess::colorMask(const cv::Mat &input, const cv::Scalar &lower, const cv::Scalar &upper) {
    cv::Mat bgr = input;
    if (input.channels() == 1) cv::cvtColor(input, bgr, cv::COLOR_GRAY2BGR);
    cv::Mat hsv{}, mask{};
    cv::cvtColor(bgr, hsv, cv::COLOR_BGR2HSV);
    cv::inRange(hsv, lower, upper, mask);
    return mask;
}

cv::Mat ImageProcess::morphology(const cv::Mat &input, const int method, const int kernelSize) {
    const auto kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(kernelSize, kernelSize));
    cv::Mat output{};
    cv::morphologyEx(input, output, method, kernel);
    return output;
}

// recognition
QPoint ImageProcess::goodFeaturesToTrack(const QImage &image) {
    const cv::Mat gray(image.height(), image.width(), CV_8UC1, const_cast<uchar *>(image.bits()), image.bytesPerLine());

    std::vector<cv::Point2f> corners{};
    cv::goodFeaturesToTrack(gray, corners, 1, 0.01, 10);
    if (corners.empty()) return {-1, -1};
    return {qRound(corners.front().x), qRound(corners.front().y)};
}

QPoint ImageProcess::cornerHarris(const QImage &image) {
    const cv::Mat gray(image.height(), image.width(), CV_8UC1, const_cast<uchar *>(image.bits()), image.bytesPerLine());

    cv::Mat response{};
    cv::cornerHarris(gray, response, 2, 3, 0.04);
    double maxValue = 0;
    cv::Point maxLocation{};
    cv::minMaxLoc(response, nullptr, &maxValue, nullptr, &maxLocation);
    if (maxValue <= 0) return {-1, -1};
    return {maxLocation.x, maxLocation.y};
}

QPoint ImageProcess::templateMatch(const QImage &image, const QImage &t_image) {
    const cv::Mat gray(image.height(), image.width(), CV_8UC1, const_cast<uchar *>(image.bits()), image.bytesPerLine());
    const cv::Mat t_gray(t_image.height(), t_image.width(), CV_8UC1, const_cast<uchar *>(t_image.bits()), t_image.bytesPerLine());
    if (t_image.width() > image.width() || t_image.height() > image.height()) return {-1, -1};

    cv::Mat response{};
    cv::matchTemplate(gray, t_gray, response, cv::TM_CCOEFF_NORMED);

    double maxValue = 0;
    cv::Point maxLocation{};
    cv::minMaxLoc(response, nullptr, &maxValue, nullptr, &maxLocation);
    if (maxValue <= 0.8) return {-1, -1};
    return {maxLocation.x + t_gray.cols / 2, maxLocation.y + t_gray.rows / 2};
}
