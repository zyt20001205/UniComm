#ifndef UNICOMM_PORTSETTING_H
#define UNICOMM_PORTSETTING_H

#include <kddockwidgets/qtwidgets/views/DockWidget.h>
#include <QJsonObject>
#include <QPixmap>
#include <QQuickImageProvider>
#include <QStandardItemModel>

#include "port/module/imageProcess.h"

class QQuickView;
class QCamera;
class QMediaCaptureSession;
class QQuickItem;
class QScreenCapture;
class QThread;
class QVideoSink;

class BluetoothDiscovery;
class ImageProvider;
class RoiModel;
class PipelineModel;

class PortSetting final : public QObject {
    Q_OBJECT

public:
    explicit PortSetting(QWidget *parent = nullptr);

    ~PortSetting() override;

    void propertySet(const QVariantHash &objects);

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    void portSettingImport(const QJsonObject &portConfig = QJsonObject());

    Q_INVOKABLE void portSettingExport();

    Q_INVOKABLE void dialogResize(int width, int height) const;

    Q_INVOKABLE void bluetoothScan(const QString &adapterAddress);

    Q_INVOKABLE void bluetoothDiscover(const QString &adapterAddress, const QString &peripheralAddress);

    Q_INVOKABLE void bluetoothServiceSelect(const QString &serviceUuid);

    Q_INVOKABLE void visionCapture();

    Q_INVOKABLE void previewLoad(int index) const;

    Q_INVOKABLE void roiInsert(const QVariantList &roi) const;

    Q_INVOKABLE void roiRemove(int index) const;

    Q_INVOKABLE void roiSwap(int src, int dst) const;

    Q_INVOKABLE void pipelineInsert(const QVariantHash &session) const;

    Q_INVOKABLE void pipelineRemove(int index) const;

    Q_INVOKABLE void pipelineSwap(int src, int dst) const;

signals:
    void insertPort(int index, const QJsonObject &portConfig, const QString &undoGroupId);

    void editPort(const QString &oldPortName, const QJsonObject &portConfig);

private:
    void serialPortRefresh() const;

    void localHostRefresh() const;

    void visionRefresh() const;

    void bluetoothAdapterRefresh() const;

    void bluetoothAdaptersUpdate(const QVariantList &adapters);

    void bluetoothPeripheralsUpdate(const QVariantList &peripherals);

    void bluetoothServicesUpdate(const QVariantList &services);

    void bluetoothStatusUpdate(const QString &status) const;

    void bluetoothBusyUpdate(bool busy) const;

    void processRefresh(const QJsonObject &portConfig) const;

    QQuickView *m_window{};
    QStandardItemModel *m_serialPortStandardItemModel{};
    QStandardItemModel *m_localHostStandardItemModel{};
    QStandardItemModel *m_visionStandardItemModel{};
    QStandardItemModel *m_bluetoothAdapterStandardItemModel{};
    QStandardItemModel *m_bluetoothPeripheralStandardItemModel{};
    QStandardItemModel *m_bluetoothServiceStandardItemModel{};
    QStandardItemModel *m_bluetoothTxCharacteristicStandardItemModel{};
    QStandardItemModel *m_bluetoothRxCharacteristicStandardItemModel{};
    QThread *m_bluetoothThread{};
    BluetoothDiscovery *m_bluetoothDiscovery{};
    QMediaCaptureSession *m_mediaCaptureSession{};
    QScreenCapture *m_screenCapture{};
    QCamera *m_cameraCapture{};
    RoiModel *m_roiModel{};
    PipelineModel *m_pipelineModel{};
    ImageProvider *m_imageProvider{};
    QString m_oldPortName{};
    QVariantList m_bluetoothServices{};
    QJsonObject m_bluetoothConfig{};

    QObject *m_root{};
    QObject *m_swipeView{};
    // serial port
    QObject *m_serialPortNameComboBox{};
    QObject *m_serialPortBaudRateSpinBox{};
    QObject *m_serialPortDataBitsComboBox{};
    QObject *m_serialPortParityComboBox{};
    QObject *m_serialPortStopBitsComboBox{};
    // vision
    QObject *m_visionNameComboBox{};
    // tcp client
    QObject *m_tcpClientNameTextField{};
    QObject *m_tcpClientRemoteHostTextField{};
    QObject *m_tcpClientRemotePortSpinBox{};
    // tcp server
    QObject *m_tcpServerNameTextField{};
    QObject *m_tcpServerLocalHostComboBox{};
    QObject *m_tcpServerLocalPortSpinBox{};
    // ssl client
    QObject *m_sslClientNameTextField{};
    QObject *m_sslClientRemoteHostTextField{};
    QObject *m_sslClientRemotePortSpinBox{};
    // ssl server
    QObject *m_sslServerNameTextField{};
    QObject *m_sslServerLocalHostComboBox{};
    QObject *m_sslServerLocalPortSpinBox{};
    QObject *m_sslServerCertificateTextField{};
    QObject *m_sslServerPrivateKeyTextField{};
    // web socket client
    QObject *m_webSocketClientNameTextField{};
    QObject *m_webSocketClientUrlTextField{};
    QObject *m_webSocketClientMessageTypeComboBox{};
    // web socket server
    QObject *m_webSocketServerNameTextField{};
    QObject *m_webSocketServerLocalHostComboBox{};
    QObject *m_webSocketServerLocalPortSpinBox{};
    QObject *m_webSocketServerSecureSwitch{};
    QObject *m_webSocketServerCertificateTextField{};
    QObject *m_webSocketServerPrivateKeyTextField{};
    QObject *m_webSocketServerMessageTypeComboBox{};
    // udp socket
    QObject *m_udpSocketNameTextField{};
    QObject *m_udpSocketLocalHostComboBox{};
    QObject *m_udpSocketLocalPortSpinBox{};
    QObject *m_udpSocketRemoteHostTextField{};
    QObject *m_udpSocketRemotePortSpinBox{};
    // bluetooth le
    QObject *m_bluetoothNameTextField{};
    QObject *m_bluetoothAdapterComboBox{};
    QObject *m_bluetoothPeripheralComboBox{};
    QObject *m_bluetoothServiceComboBox{};
    QObject *m_bluetoothTxCharacteristicComboBox{};
    QObject *m_bluetoothRxCharacteristicComboBox{};
    QObject *m_bluetoothWriteTypeComboBox{};
    QObject *m_bluetoothSubscribeTypeComboBox{};
    QObject *m_bluetoothStatusLabel{};
    // format
    QObject *m_logFormatComboBox{};
    QObject *m_txSuffixComboBox{};
    QObject *m_bufferSizeSpinBox{};
    // image
    QVideoSink *m_videoSink{};
    QObject *m_previewImage{};
    QObject *m_recognitionComboBox{};
    QObject *m_templateTextField{};
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

class RoiModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit RoiModel(QObject *parent = nullptr);

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

    signals:
        void emptyChanged();
};

class PipelineModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(bool empty READ emptyGet NOTIFY emptyChanged)

public:
    explicit PipelineModel(QObject *parent = nullptr);

    [[nodiscard]] bool emptyGet() const {
        return rowCount() == 0;
    }

    signals:
        void emptyChanged();
};

#endif //UNICOMM_PORTSETTING_H
