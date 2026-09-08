#include "port/videoStream.h"

#include <QCamera>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QScreenCapture>
#include <QTimer>
#include <QVideoFrame>
#include <QVideoSink>

#include "globals.h"

// public
VideoStream::VideoStream(const QJsonObject &portConfig, QObject *parent)
    : BasePort(parent),
      m_portConfig(portConfig) {
    m_imageProcess.configSet(m_portConfig);
}

VideoStream::~VideoStream() {
    close();
}

int VideoStream::type() {
    return PortType::VideoStream;
}

QJsonObject VideoStream::config() {
    return m_portConfig;
}

bool VideoStream::open() {
    // port init
    if (m_mediaCaptureSession == nullptr) {
        m_mediaCaptureSession = new QMediaCaptureSession(this);
        m_videoSink = new QVideoSink(this);
        m_mediaCaptureSession->setVideoSink(m_videoSink);
        const auto &portName = m_portConfig["portName"].toString();
        for (QScreen *screen: QGuiApplication::screens()) {
            if (portName == screen->name()) {
                m_screenCapture = new QScreenCapture(this);
                m_screenCapture->setScreen(screen);
                m_mediaCaptureSession->setScreenCapture(m_screenCapture);
                break;
            }
        }
        for (const QCameraDevice &camera: QMediaDevices::videoInputs()) {
            if (portName == camera.description()) {
                m_cameraCapture = new QCamera(camera, this);
                m_mediaCaptureSession->setCamera(m_cameraCapture);
                break;
            }
        }
    }
    if (!m_screenCapture && !m_cameraCapture) {
        const QVariantHash session{{"active", false}};
        emit refreshPort(m_portConfig["portName"].toString(), session);
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
        return false;
    }
    // port open
    QEventLoop eventLoop;
    bool frameReady = false;
    connect(m_videoSink, &QVideoSink::videoFrameChanged, &eventLoop, [&eventLoop, &frameReady](const QVideoFrame &frame) {
        if (!frame.isValid()) return;
        frameReady = true;
        eventLoop.quit();
    });
    if (m_screenCapture) {
        connect(m_screenCapture, &QScreenCapture::errorOccurred, &eventLoop, &QEventLoop::quit);
        m_screenCapture->start();
    } else {
        connect(m_cameraCapture, &QCamera::errorOccurred, &eventLoop, &QEventLoop::quit);
        m_cameraCapture->start();
    }
    QTimer::singleShot(30000, &eventLoop, &QEventLoop::quit);
    if (!frameReady) eventLoop.exec();
    if (!frameReady) {
        if (m_screenCapture) m_screenCapture->stop();
        else m_cameraCapture->stop();
        const QVariantHash session{{"active", false}};
        emit refreshPort(m_portConfig["portName"].toString(), session);
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "open failed");
        return false;
    }
    const QVariantHash session{{"active", true}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "opened");
    return true;
}

void VideoStream::close() {
    // port close
    if (m_screenCapture) m_screenCapture->stop();
    else if (m_cameraCapture) m_cameraCapture->stop();
    clear();
    const QVariantHash session{{"active", false}};
    emit refreshPort(m_portConfig["portName"].toString(), session);
    emit appendLog(LogLevel::Info, QString("[%1]").arg(m_portConfig["portName"].toString()), "closed");
}

void VideoStream::clear() {
}

QVariantHash VideoStream::info() {
    return {};
}

// TODO: register commands to video stream
bool VideoStream::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    bool status = false;
    if (m_screenCapture) status = m_screenCapture->isActive();
    else if (m_cameraCapture) status = m_cameraCapture->isActive();
    // check port status
    if (!status) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    const auto rawFrame = m_videoSink->videoFrame();
    const auto rawImage = rawFrame.toImage();
    if (rawImage.isNull()) return {};

    const auto command = QString::fromUtf8(txData);
    if (command == "raw") {
        const QString fileName = g_workspaceUrl.toLocalFile() + "/raw.png";
        return rawImage.save(fileName);
    }
    if (command == "processed") {
        return {};
    }
    return {};
}

QVariantList VideoStream::result(const int timeout) {
    bool status = false;
    if (m_screenCapture) status = m_screenCapture->isActive();
    else if (m_cameraCapture) status = m_cameraCapture->isActive();
    // check port status
    if (!status) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    if (timeout != 0) {
        QEventLoop eventLoop;
        bool frameChanged = false;
        connect(m_videoSink, &QVideoSink::videoFrameChanged, &eventLoop, [&eventLoop, &frameChanged] {
            frameChanged = true;
            eventLoop.quit();
        });
        if (timeout > 0) QTimer::singleShot(timeout, &eventLoop, &QEventLoop::quit);
        eventLoop.exec();
        if (!frameChanged) return {};
    }
    const auto rawFrame = m_videoSink->videoFrame();
    const auto rawImage = rawFrame.toImage();
    if (rawImage.isNull()) return {};

    QVariantList results{};
    for (const auto &result: m_imageProcess.process(rawImage)) results.append(result.result);
    return results;
}
