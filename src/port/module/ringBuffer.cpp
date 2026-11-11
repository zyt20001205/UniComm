#include "port/module/ringBuffer.h"

#include <QVarLengthArray>

// public
RingBuffer::RingBuffer(const qsizetype capacity)
    : m_capacity(capacity > 0 ? capacity : 0),
      m_buffer(m_capacity, 0) {
}

qsizetype RingBuffer::write(const QByteArray &data) {
    const qsizetype length = data.size();
    {
        QMutexLocker locker(&m_mutex);
        if (length == 0 || length > m_capacity - m_used) return 0;
        const qsizetype firstChunk = qMin(length, m_capacity - m_writePos);
        const qsizetype secondChunk = length - firstChunk;
        if (firstChunk > 0) m_buffer.replace(m_writePos, firstChunk, data.constData(), firstChunk);
        if (secondChunk > 0) m_buffer.replace(static_cast<qsizetype>(0), secondChunk, data.constData() + firstChunk, secondChunk);

        m_writePos = (m_writePos + length) % m_capacity;
        m_used += length;
    }
    return length;
}

QByteArray RingBuffer::read(const qsizetype length, const bool peek) {
    QMutexLocker locker(&m_mutex);
    return readLocked(length, peek);
}

QByteArray RingBuffer::readUntil(const QByteArray &text, const bool peek) {
    QMutexLocker locker(&m_mutex);
    const qsizetype length = distanceLocked(text);
    if (length < 0) return {};
    return readLocked(length, peek);
}

qsizetype RingBuffer::used() {
    QMutexLocker locker(&m_mutex);
    return m_used;
}

qsizetype RingBuffer::distance(const QByteArray &text) {
    QMutexLocker locker(&m_mutex);
    return distanceLocked(text);
}

void RingBuffer::clear() {
    QMutexLocker locker(&m_mutex);
    m_buffer.fill(0);
    m_readPos = 0;
    m_writePos = 0;
    m_used = 0;
}

// private
QByteArray RingBuffer::readLocked(qsizetype length, const bool peek) {
    if (length < 0 || length > m_used) return {};
    if (length == 0) length = m_used;
    if (length == 0 || m_capacity == 0) return {};
    QByteArray data;
    data.reserve(length);
    const qsizetype firstChunk = qMin(length, m_capacity - m_readPos);
    const qsizetype secondChunk = length - firstChunk;
    if (firstChunk > 0) data.append(m_buffer.constData() + m_readPos, firstChunk);
    if (secondChunk > 0) data.append(m_buffer.constData(), secondChunk);

    if (!peek) {
        m_readPos = (m_readPos + length) % m_capacity;
        m_used -= length;
    }
    return data;
}

qsizetype RingBuffer::distanceLocked(const QByteArray &text) const {
    const qsizetype textSize = text.size();
    if (textSize == 0 || textSize > m_used || m_capacity == 0) return -1;

    const char *pattern = text.constData();
    QVarLengthArray<qsizetype, 64> prefix(textSize, 0);
    for (qsizetype i = 1, matched = 0; i < textSize; ++i) {
        while (matched > 0 && pattern[i] != pattern[matched]) matched = prefix[matched - 1];
        if (pattern[i] == pattern[matched]) ++matched;
        prefix[i] = matched;
    }

    qsizetype matched = 0;
    qsizetype bufferIndex = m_readPos;
    for (qsizetype offset = 0; offset < m_used; ++offset) {
        const char value = m_buffer[bufferIndex];
        while (matched > 0 && value != pattern[matched]) matched = prefix[matched - 1];
        if (value == pattern[matched]) ++matched;
        if (matched == textSize) return offset + 1;

        ++bufferIndex;
        if (bufferIndex == m_capacity) bufferIndex = 0;
    }
    return -1;
}
