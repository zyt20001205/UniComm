#ifndef UNICOMM_THEMEMANAGER_H
#define UNICOMM_THEMEMANAGER_H

#include <QJsonObject>

class ThemeManager final : public QObject {
    Q_OBJECT

public:
    explicit ThemeManager(QObject *parent = nullptr);

    [[nodiscard]] const QJsonObject &configGet() const {
        return m_config;
    }

private:
    QJsonObject m_config{};
};

#endif //UNICOMM_THEMEMANAGER_H
