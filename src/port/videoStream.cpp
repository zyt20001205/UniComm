#include "port/videoStream.h"

#include <QCamera>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QMediaCaptureSession>
#include <QMediaDevices>
#include <QProcess>
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

bool VideoStream::write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) {
    bool status = false;
    if (m_screenCapture) status = m_screenCapture->isActive();
    else if (m_cameraCapture) status = m_cameraCapture->isActive();
    // check port status
    if (!status) {
        emit appendLog(LogLevel::Error, QString("[%1]").arg(m_portConfig["portName"].toString()), "not opened");
        return {};
    }
    const auto arguments = QProcess::splitCommand(QString::fromUtf8(txData));
    if (arguments.isEmpty()) return false;
    const auto command = arguments.constFirst().toLower();
    if (command != "bundle" && command != "processed" && command != "raw" && command != "roi") return false;
    const auto rawImage = snapshot(30000);
    if (rawImage.isNull()) return {};

    const auto absolutePath = [](const QString &path) {
        return QDir::isAbsolutePath(path) ? QDir::cleanPath(path) : QDir(g_workspaceUrl.toLocalFile()).absoluteFilePath(path);
    };
    const auto imageSave = [&absolutePath](const QImage &image, const QString &path) {
        const auto filePath = absolutePath(path);
        if (!QDir().mkpath(QFileInfo(filePath).absolutePath())) return false;
        return image.save(filePath);
    };

    if (command == "raw") {
        if (arguments.size() > 2) return false;
        return imageSave(rawImage, arguments.value(1, "raw.png"));
    }

    if (command == "bundle") {
        if (arguments.size() > 2) return false;
        const auto results = m_imageProcess.process(rawImage);
        const auto directoryPath = absolutePath(arguments.value(1, "bundle"));
        if (!QDir().mkpath(directoryPath)) return false;
        const QDir directory(directoryPath);
        if (!rawImage.save(directory.filePath("raw.png"))) return false;

        QVariantList recognitionResults{};
        for (int i = 0; i < results.size(); ++i) {
            const auto &result = results.at(i);
            if (!result.roiFrame.save(directory.filePath(QString("roi-%1.png").arg(i + 1)))) return false;
            if (!result.pipelineFrame.save(directory.filePath(QString("processed-%1.png").arg(i + 1)))) return false;
            recognitionResults.append(result.result);
        }

        const QJsonObject metadata{
            {"width", rawImage.width()},
            {"height", rawImage.height()},
            {"results", QJsonArray::fromVariantList(recognitionResults)}
        };
        QFile resultFile(directory.filePath("result.json"));
        if (!resultFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) return false;
        const auto data = QJsonDocument(metadata).toJson(QJsonDocument::Indented);
        return resultFile.write(data) == data.size();
    }

    if (arguments.size() < 2 || arguments.size() > 3) return false;
    bool indexValid = false;
    const int index = arguments.at(1).toInt(&indexValid) - 1;
    if (!indexValid || index < 0) return false;
    const auto results = m_imageProcess.process(rawImage);
    if (index >= results.size()) return false;
    const auto defaultPath = QString("%1-%2.png").arg(command).arg(index + 1);
    const auto &image = command == "roi" ? results.at(index).roiFrame : results.at(index).pipelineFrame;
    return imageSave(image, arguments.value(2, defaultPath));
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
    const auto rawImage = timeout == 0 ? m_videoSink->videoFrame().toImage() : snapshot(timeout);
    if (rawImage.isNull()) return {};

    QVariantList results{};
    for (const auto &result: m_imageProcess.process(rawImage)) results.append(result.result);
    return results;
}

// private
QImage VideoStream::snapshot(const int timeout) const {
    QEventLoop eventLoop;
    QImage image{};
    connect(m_videoSink, &QVideoSink::videoFrameChanged, &eventLoop, [&eventLoop, &image](const QVideoFrame &frame) {
        image = frame.toImage();
        eventLoop.quit();
    });
    if (timeout > 0) QTimer::singleShot(timeout, &eventLoop, &QEventLoop::quit);
    eventLoop.exec();
    return image;
}
