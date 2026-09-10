#ifndef UNICOMM_BASEPROVIDER_H
#define UNICOMM_BASEPROVIDER_H

#include <QJsonObject>
#include <QNetworkRequest>
#include <QObject>
#include <QString>
#include <QStringList>

class BaseProvider : public QObject {
    Q_OBJECT

public:
    struct Model {
        QString id{};
        QString name{};
        qint64 contextWindow{};
        qint64 maxOutputTokens{};
        QStringList input{};
    };

    explicit BaseProvider(QObject *parent = nullptr);

    ~BaseProvider() override = default;

    [[nodiscard]] virtual QNetworkRequest requestGet() const = 0;

    [[nodiscard]] virtual QJsonObject requestBuild(const QString &model, const QJsonArray &messages, const QJsonArray &tools, bool stream) const = 0;

    virtual void apikeyGet() = 0;

    virtual void apikeySet(const QString &apikey) = 0;

    virtual void apikeyRemove() = 0;

    virtual void modelsGet() = 0;

    [[nodiscard]] virtual Model modelGet(const QString &id) const = 0;

signals:
    void apikeyChanged(const QString &apikey);

    void modelsChanged();
};

#endif //UNICOMM_BASEPROVIDER_H
