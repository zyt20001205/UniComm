#ifndef UNICOMM_OPENAIPROVIDER_H
#define UNICOMM_OPENAIPROVIDER_H

#include <QList>
#include <QNetworkRequest>
#include <QUrl>

#include "baseProvider.h"

class ProviderModelModel;

class OpenAIProvider final : public BaseProvider {
    Q_OBJECT

public:
    explicit OpenAIProvider(const QString &id, const QJsonObject &provider, QObject *parent = nullptr);

    ~OpenAIProvider() override = default;

    [[nodiscard]] QString idGet() const {
        return m_id;
    }

    [[nodiscard]] QString nameGet() const {
        return m_name;
    }

    [[nodiscard]] QUrl baseUrlGet() const {
        return m_baseUrl;
    }

    [[nodiscard]] QString chatEndpointGet() const {
        return m_chatEndpoint;
    }

    [[nodiscard]] QString modelEndpointGet() const {
        return m_modelEndpoint;
    }

    [[nodiscard]] ProviderModelModel *modelListGet() const {
        return m_modelList;
    }

    void configSet(const QJsonObject &config);

    [[nodiscard]] QNetworkRequest requestGet() const override {
        return m_request;
    }

    [[nodiscard]] QJsonObject requestBuild(const QString &model, const QJsonArray &messages, const QJsonArray &tools, bool stream) const override;

    void apikeyGet() override;

    void apikeySet(const QString &apikey) override;

    void apikeyRemove() override;

    void modelsGet() override;

    [[nodiscard]] Model modelGet(const QString &id) const override;

private:
    void modelAppend(const Model &model) const;

    void requestUpdate();

    QString m_id{};
    QString m_name{};
    QUrl m_baseUrl{};
    QString m_chatEndpoint{};
    QString m_modelEndpoint{};
    bool m_modelFetch{};
    QList<Model> m_models{};
    QNetworkRequest m_request{};
    ProviderModelModel *m_modelList{};
};

#endif //UNICOMM_OPENAIPROVIDER_H
