#ifndef UNICOMM_VISA_H
#define UNICOMM_VISA_H

#include <QElapsedTimer>
#include <QJsonObject>
#include <visatype.h>

#include "basePort.h"
#include "port/module/ringBuffer.h"

class QTimer;

class Visa final : public BasePort {
    Q_OBJECT

public:
    explicit Visa(const QJsonObject &portConfig, QObject *parent = nullptr);

    ~Visa() override;

    [[nodiscard]] int type() override;

    [[nodiscard]] QJsonObject config() override;

    [[nodiscard]] QVariantHash info() override;

    [[nodiscard]] bool open() override;

    void close() override;

    void clear() override;

    void monitor(bool enabled) override;

    [[nodiscard]] bool write(const QByteArray &txData, const QString &logFormat, const QString &txSuffix) override;

    [[nodiscard]] QByteArray read(int length, int timeout, const QString &logFormat, bool peek) override;

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, int timeout, const QString &logFormat, bool peek) override;

signals:
    void connected();

    void disconnected();

private:
    [[nodiscard]] bool handleWrite(const QByteArray &f_txData);

    [[nodiscard]] QByteArray handleRead(int length, int timeout);

    [[nodiscard]] QByteArray handleReadUntil(const QByteArray &text, int timeout);

    void handleUpdate();

    void handleLog(int type, const QByteArray &data);

    ViSession m_visa{};
    QJsonObject m_portConfig{};
    RingBuffer m_buffer;
    QTimer *m_monitorTimer{};
    QElapsedTimer m_activeTimer{};
};

#endif //UNICOMM_VISA_H
