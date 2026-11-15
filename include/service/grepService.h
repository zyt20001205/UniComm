#ifndef UNICOMM_GREPSERVICE_H
#define UNICOMM_GREPSERVICE_H

#include <QJsonObject>

#include "service/baseService.h"

class QProcess;

class GrepService final : public BaseService {
    Q_OBJECT

public:
    explicit GrepService(QObject *parent = nullptr);

    [[nodiscard]] QJsonObject grep(const QString &pattern);

private:
    QProcess *m_process{};
};

#endif //UNICOMM_GREPSERVICE_H
