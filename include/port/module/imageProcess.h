#ifndef UNICOMM_IMAGEPROCESS_H
#define UNICOMM_IMAGEPROCESS_H

#include <QImage>
#include <QJsonObject>
#include <QList>
#include <QPoint>
#include <QUrl>
#include <QVariantHash>

#include "opencv2/core/mat.hpp"

namespace tesseract {
    class TessBaseAPI;
}

class ImageProcess final {
public:
    struct ProcessResult {
        QImage roiFrame;
        QImage pipelineFrame;
        QVariantHash result;
    };

    ImageProcess();

    ~ImageProcess();

    void configSet(const QJsonObject &config) {
        m_config = config;
    }

    [[nodiscard]] QList<ProcessResult> process(const QImage &frame);

private:
    [[nodiscard]] static QImage roi(const QImage &frame, const QJsonArray &roi);

    [[nodiscard]] static QImage pipeline(const QImage &roiFrame, const QJsonArray &pipeline);

    [[nodiscard]] QVariantHash recognition(const QImage &pipelineFrame, const QJsonObject &recognition);

    [[nodiscard]] static cv::Mat scale(const cv::Mat &input, float ratio, int interpolation);

    [[nodiscard]] static cv::Mat threshold(const cv::Mat &input, int thresh, int maxval, int mode);

    [[nodiscard]] static QPoint goodFeaturesToTrack(const QImage &image);

    [[nodiscard]] static QPoint cornerHarris(const QImage &image);

    [[nodiscard]] static QPoint templateMatch(const QImage &image, const QImage &t_image);

    QJsonObject m_config{};
    QUrl m_templateUrl{};
    QImage m_template{};
    tesseract::TessBaseAPI *m_ocrEngine{};
};

#endif //UNICOMM_IMAGEPROCESS_H
