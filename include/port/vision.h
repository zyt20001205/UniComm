#ifndef UNICOMM_VISION_H
#define UNICOMM_VISION_H

#include "port/basePort.h"
#include "port/module/imageProcess.h"

class QCamera;
class QEventLoop;
class QImageCapture;
class QMediaCaptureSession;
class QScreenCapture;
class QVideoSink;

class Vision final : public BasePort {
    Q_OBJECT

public:
    explicit Vision(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~Vision() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] QVariantList result(int timeout);

private:
    [[nodiscard]] QImage snapshot(int timeout) const;

    QMediaCaptureSession *m_mediaCaptureSession{};
    QVideoSink *m_videoSink{};
    QScreenCapture *m_screenCapture{};
    QCamera *m_cameraCapture{};
    // port config
    QJsonObject m_portConfig{};
    ImageProcess m_imageProcess{};
};

#endif //UNICOMM_VISION_H
