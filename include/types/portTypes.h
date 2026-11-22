#ifndef UNICOMM_PORTTYPES_H
#define UNICOMM_PORTTYPES_H

#include <QMetaObject>
#include <QtQmlIntegration/qqmlintegration.h>

namespace PortType {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value : int {
        SerialPort,
        Vision,
        TcpClient,
        TcpServer,
        SslClient,
        SslServer,
        WebSocketClient,
        WebSocketServer,
        UdpSocket,
        BluetoothLe
    };

    Q_ENUM_NS(Value)
}

namespace ImageOperation {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value : int {
        Scale,
        Rotate,
        Flip,
        Blur,
        Threshold,
        ColorMask,
        Morphology
    };

    Q_ENUM_NS(Value)
}

namespace RecognitionMethod {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value : int {
        OCR,
        ShiTomasiCorner,
        HarrisCorner,
        TemplateMatch
    };

    Q_ENUM_NS(Value)
}

namespace WebSocketMessageType {
    Q_NAMESPACE

    QML_ELEMENT

    enum Value : int {
        Text = 1,
        Binary = 2
    };

    Q_ENUM_NS(Value)
}

#endif //UNICOMM_PORTTYPES_H
