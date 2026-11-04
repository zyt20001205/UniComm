#ifndef UNICOMM_RINGBUFFER_H
#define UNICOMM_RINGBUFFER_H

#include <QByteArray>
#include <QMutex>

class RingBuffer final {
public:
    struct Statistics {
        qsizetype used{};
        quint64 readCount{};
        quint64 readBytes{};
        quint64 writeCount{};
        quint64 writeBytes{};
    };

    explicit RingBuffer(qsizetype capacity);

    [[nodiscard]] qsizetype write(const QByteArray &data);

    [[nodiscard]] QByteArray read(qsizetype length, bool peek = false);

    [[nodiscard]] QByteArray readUntil(const QByteArray &text, bool peek = false);

    [[nodiscard]] qsizetype used();

    [[nodiscard]] qsizetype distance(const QByteArray &text);

    [[nodiscard]] Statistics statistics();

    void resetStatistics();

    void clear();

private:
    [[nodiscard]] QByteArray readLocked(qsizetype length, bool peek);

    [[nodiscard]] qsizetype distanceLocked(const QByteArray &text) const;

    const qsizetype m_capacity{};
    QByteArray m_buffer{};
    qsizetype m_readPos{};
    qsizetype m_writePos{};
    qsizetype m_used{};
    quint64 m_readCount{};
    quint64 m_readBytes{};
    quint64 m_writeCount{};
    quint64 m_writeBytes{};
    QMutex m_mutex{};
};

#endif //UNICOMM_RINGBUFFER_H
