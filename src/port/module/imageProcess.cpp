#include "port/module/imageProcess.h"

#include <QJsonArray>
#include <QPainter>
#include <QPixmap>

#include "globals.h"
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
    auto roiArray = m_config["roi"].toArray();
    if (roiArray.isEmpty()) roiArray.append(QJsonArray{0, 0, frame.width(), frame.height()});
    for (const QJsonValue &value: roiArray) {
        ProcessResult processResult{};
        // roi
        processResult.roiFrame = roi(frame, value.toArray());
        // pipeline
        processResult.pipelineFrame = pipeline(processResult.roiFrame, m_config["pipeline"].toArray());
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

QImage ImageProcess::pipeline(const QImage &roiFrame, const QJsonArray &pipeline) {
    const auto &source = roiFrame.convertToFormat(QImage::Format_BGR888);
    cv::Mat frame(source.height(), source.width(), CV_8UC3, const_cast<uchar *>(source.constBits()), source.bytesPerLine());
    for (const auto &value: pipeline) {
        const auto &session = value.toObject();
        switch (session["type"].toInt()) {
            case ImagePipeline::Scale: {
                const float ratio = static_cast<float>(session["ratio"].toDouble(1));
                const int interpolation = session["interpolation"].toInt();
                frame = scale(frame, ratio, interpolation);
            }
            break;
            case ImagePipeline::Threshold: {
                const int thresh = session["thresh"].toInt();
                const int maxval = session["maxval"].toInt();
                const int mode = session["mode"].toInt();
                frame = threshold(frame, thresh, maxval, mode);
            }
            break;
            default: break;
        }
    }
    const auto format = frame.channels() == 1 ? QImage::Format_Grayscale8 : QImage::Format_BGR888;
    return QImage(frame.data, frame.cols, frame.rows, frame.step, format).copy();
}

QVariantHash ImageProcess::recognition(const QImage &pipelineFrame, const QJsonObject &recognition) {
    QVariantHash result{
        {"text", ""},
        {"items", QVariantList{}}
    };
    const auto mode = recognition["mode"].toInt();
    switch (mode) {
        case Recognition::OCR: {
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
        case Recognition::CornerShiTomasi: {
            const auto point = goodFeaturesToTrack(pipelineFrame);
            if (point != QPoint(-1, -1)) {
                result["text"] = QString("%1,%2").arg(point.x()).arg(point.y());
                result["items"] = QVariantList{QVariantHash{
                    {"x", point.x()}, {"y", point.y()}, {"width", 0}, {"height", 0}
                }};
            }
        }
        break;
        case Recognition::CornerHarris: {
            const auto point = cornerHarris(pipelineFrame);
            if (point != QPoint(-1, -1)) {
                result["text"] = QString("%1,%2").arg(point.x()).arg(point.y());
                result["items"] = QVariantList{QVariantHash{
                    {"x", point.x()}, {"y", point.y()}, {"width", 0}, {"height", 0}
                }};
            }
        }
        break;
        case Recognition::TemplateMatch: {
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
