#ifndef UNICOMM_THEMEMANAGER_H
#define UNICOMM_THEMEMANAGER_H

#include <QJsonObject>

class ThemeManager final : public QObject {
    Q_OBJECT
    Q_PROPERTY(int mode READ modeGet CONSTANT)
    Q_PROPERTY(QString fore READ foreGet CONSTANT)
    Q_PROPERTY(QString foreHover READ foreHoverGet CONSTANT)
    Q_PROPERTY(QString forePressed READ forePressedGet CONSTANT)
    Q_PROPERTY(QString foreSelected READ foreSelectedGet CONSTANT)
    Q_PROPERTY(QString foreDisabled READ foreDisabledGet CONSTANT)

    Q_PROPERTY(QString back READ backGet CONSTANT)
    Q_PROPERTY(QString backHover READ backHoverGet CONSTANT)
    Q_PROPERTY(QString backPressed READ backPressedGet CONSTANT)
    Q_PROPERTY(QString backSelected READ backSelectedGet CONSTANT)

    Q_PROPERTY(QString stroke READ strokeGet CONSTANT)
    Q_PROPERTY(QString strokePressed READ strokePressedGet CONSTANT)

    Q_PROPERTY(QString brandFore READ brandForeGet CONSTANT)

    Q_PROPERTY(QString brandBack READ brandBackGet CONSTANT)
    Q_PROPERTY(QString brandBackSelected READ brandBackSelectedGet CONSTANT)

    Q_PROPERTY(QString brandStroke READ brandStrokeGet CONSTANT)
    Q_PROPERTY(QString brandLink READ brandLinkGet CONSTANT)

    Q_PROPERTY(QString successFore2 READ successFore2Get CONSTANT)
    Q_PROPERTY(QString successBack2 READ successBack2Get CONSTANT)
    Q_PROPERTY(QString successFore3 READ successFore3Get CONSTANT)
    Q_PROPERTY(QString successBack3 READ successBack3Get CONSTANT)

    Q_PROPERTY(QString warningFore2 READ warningFore2Get CONSTANT)
    Q_PROPERTY(QString warningBack2 READ warningBack2Get CONSTANT)
    Q_PROPERTY(QString warningFore3 READ warningFore3Get CONSTANT)
    Q_PROPERTY(QString warningBack3 READ warningBack3Get CONSTANT)

    Q_PROPERTY(QString dangerFore2 READ dangerFore2Get CONSTANT)
    Q_PROPERTY(QString dangerBack2 READ dangerBack2Get CONSTANT)
    Q_PROPERTY(QString dangerFore3 READ dangerFore3Get CONSTANT)
    Q_PROPERTY(QString dangerBack3 READ dangerBack3Get CONSTANT)

public:
    explicit ThemeManager(QObject *parent = nullptr);

    [[nodiscard]] int modeGet() const {
        return m_mode;
    }

    [[nodiscard]] QString foreGet() const {
        return m_palette["fore"].toString();
    }

    [[nodiscard]] QString foreHoverGet() const {
        return m_palette["foreHover"].toString();
    }

    [[nodiscard]] QString forePressedGet() const {
        return m_palette["forePressed"].toString();
    }

    [[nodiscard]] QString foreSelectedGet() const {
        return m_palette["foreSelected"].toString();
    }

    [[nodiscard]] QString foreDisabledGet() const {
        return m_palette["foreDisabled"].toString();
    }

    [[nodiscard]] QString backGet() const {
        return m_palette["back"].toString();
    }

    [[nodiscard]] QString backHoverGet() const {
        return m_palette["backHover"].toString();
    }

    [[nodiscard]] QString backPressedGet() const {
        return m_palette["backPressed"].toString();
    }

    [[nodiscard]] QString backSelectedGet() const {
        return m_palette["backSelected"].toString();
    }

    [[nodiscard]] QString strokeGet() const {
        return m_palette["stroke"].toString();
    }

    [[nodiscard]] QString strokePressedGet() const {
        return m_palette["strokePressed"].toString();
    }

    [[nodiscard]] QString brandForeGet() const {
        return m_palette["brandFore"].toString();
    }

    [[nodiscard]] QString brandBackGet() const {
        return m_palette["brandBack"].toString();
    }

    [[nodiscard]] QString brandBackSelectedGet() const {
        return m_palette["brandBackSelected"].toString();
    }

    [[nodiscard]] QString brandStrokeGet() const {
        return m_palette["brandStroke"].toString();
    }

    [[nodiscard]] QString brandLinkGet() const {
        return m_palette["brandLink"].toString();
    }

    [[nodiscard]] QString successFore2Get() const {
        return m_palette["successFore2"].toString();
    }

    [[nodiscard]] QString successBack2Get() const {
        return m_palette["successBack2"].toString();
    }

    [[nodiscard]] QString successFore3Get() const {
        return m_palette["successFore3"].toString();
    }

    [[nodiscard]] QString successBack3Get() const {
        return m_palette["successBack3"].toString();
    }

    [[nodiscard]] QString warningFore2Get() const {
        return m_palette["warningFore2"].toString();
    }

    [[nodiscard]] QString warningBack2Get() const {
        return m_palette["warningBack2"].toString();
    }

    [[nodiscard]] QString warningFore3Get() const {
        return m_palette["warningFore3"].toString();
    }

    [[nodiscard]] QString warningBack3Get() const {
        return m_palette["warningBack3"].toString();
    }

    [[nodiscard]] QString dangerFore2Get() const {
        return m_palette["dangerFore2"].toString();
    }

    [[nodiscard]] QString dangerBack2Get() const {
        return m_palette["dangerBack2"].toString();
    }

    [[nodiscard]] QString dangerFore3Get() const {
        return m_palette["dangerFore3"].toString();
    }

    [[nodiscard]] QString dangerBack3Get() const {
        return m_palette["dangerBack3"].toString();
    }

    [[nodiscard]] const QJsonObject &configGet() const {
        return m_config;
    }

private:
    int m_mode{};
    QJsonObject m_palette{};
    QJsonObject m_config{};
};

#endif //UNICOMM_THEMEMANAGER_H
