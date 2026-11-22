#ifndef UNICOMM_PORTMANAGEWINDOW_H
#define UNICOMM_PORTMANAGEWINDOW_H

#include <QFuture>
#include <QQuickImageProvider>
#include <QQuickView>
#include <QStandardItemModel>
#include <QStringList>
#include <QVariantList>
#include <QVideoSink>

#include "port/module/imageProcess.h"
#include "types/portTypes.h"

class QCamera;
class QMediaCaptureSession;
class QScreenCapture;

class ImageProvider;

class PortManageWindow : public QQuickView {
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("PortManageWindow is created by C++.")
    Q_PROPERTY(QVariantHash config MEMBER m_config NOTIFY configChanged)
    Q_PROPERTY(QVariantList serialPorts READ serialPortsGet NOTIFY serialPortsChanged)
    Q_PROPERTY(QStringList localHosts READ localHostsGet NOTIFY localHostsChanged)
    Q_PROPERTY(QStringList visionSources READ visionSourcesGet NOTIFY visionSourcesChanged)
    Q_PROPERTY(QVariantList bluetoothPeripherals READ bluetoothPeripheralsGet NOTIFY bluetoothPeripheralsChanged)
    Q_PROPERTY(QAbstractItemModel *bluetoothServices READ bluetoothServicesGet CONSTANT)
    Q_PROPERTY(QString bluetoothServiceAddress READ bluetoothServiceAddressGet NOTIFY bluetoothServicesChanged)

public:
    PortManageWindow();

    ~PortManageWindow() override;

    void propertySet();

    void open(const QJsonObject &portConfig = QJsonObject());

    Q_INVOKABLE void typeSelect(PortType::Value type);

    Q_INVOKABLE void save();

    Q_INVOKABLE void bleConnect(const QVariant &peripheral);

    Q_INVOKABLE void visionCapture(QVideoSink *videoSink);

    Q_INVOKABLE [[nodiscard]] QVariantHash previewLoad(int index) const;

signals:
    void configChanged();

    void serialPortsChanged();

    void localHostsChanged();

    void visionSourcesChanged();

    void bluetoothPeripheralsChanged();

    void bluetoothServicesChanged();

    void insertPort(int index, const QJsonObject &portConfig, const QString &undoGroupId);

    void editPort(const QString &oldPortName, const QJsonObject &portConfig);

private:
    [[nodiscard]] QVariantList serialPortsGet() const;

    [[nodiscard]] QStringList localHostsGet() const;

    [[nodiscard]] QStringList visionSourcesGet() const;

    [[nodiscard]] QVariantList bluetoothPeripheralsGet() const;

    [[nodiscard]] QAbstractItemModel *bluetoothServicesGet() const;

    [[nodiscard]] QString bluetoothServiceAddressGet() const;

    void serialPortRefresh();

    void localHostRefresh();

    void visionRefresh();

    void bleRefresh();

    QVariantList m_serialPorts{};
    QStringList m_localHosts{};
    QStringList m_visionSources{};
    QVariantList m_bluetoothPeripherals{};
    QStandardItemModel *m_bluetoothServices{};
    QString m_bluetoothServiceAddress{};
    QFuture<void> m_bleScanFuture{};
    QFuture<QVariantList> m_bleConnectFuture{};
    QMediaCaptureSession *m_mediaCaptureSession{};
    QScreenCapture *m_screenCapture{};
    QCamera *m_cameraCapture{};
    ImageProvider *m_imageProvider{};
    QString m_oldPortName{};
    QVariantHash m_config{};
};

class ImageProvider final: public QQuickImageProvider {
    Q_OBJECT

public:
    explicit ImageProvider();

    ~ImageProvider() = default;

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override;

    void preview(const QVideoSink *videoSink, const QJsonObject &config);

    [[nodiscard]] QVariantHash recognition() const;

private:
    QPixmap m_preview{};
    QVariantHash m_recognition{};
    ImageProcess m_imageProcess{};
};

#endif //UNICOMM_PORTMANAGEWINDOW_H
