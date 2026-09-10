#include "agent/provider/openAIProvider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <qt6keychain/keychain.h>

#include "globals.h"
#include "agent/provider/providerModule.h"

OpenAIProvider::OpenAIProvider(const QString &id, const QJsonObject &provider, QObject *parent)
    : BaseProvider(parent),
      m_id(id),
      m_modelList(new ProviderModelModel(this)) {
    m_request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    configSet(provider);
}

void OpenAIProvider::configSet(const QJsonObject &config) {
    m_name = config.value("name").toString();
    m_baseUrl = config.value("api").toString();
    m_chatEndpoint = config.value("chatEndpoint").toString("/chat/completions");
    m_modelEndpoint = config.value("modelsEndpoint").toString("/models");
    m_modelFetch = !m_modelEndpoint.isEmpty();
    m_models.clear();

    const auto models = config.value("models").toObject();
    for (auto iterator = models.constBegin(); iterator != models.constEnd(); ++iterator) {
        const auto object = iterator.value().toObject();
        const auto limit = object.value("limit").toObject();
        QStringList input{};
        for (const auto &value: object.value("modalities").toObject().value("input").toArray()) input.append(value.toString());
        m_models.append(Model{
            .id = iterator.key(),
            .name = object.value("name").toString(iterator.key()),
            .contextWindow = limit.value("context").toInteger(),
            .maxOutputTokens = limit.value("output").toInteger(),
            .input = input
        });
    }
    requestUpdate();
}

QJsonObject OpenAIProvider::requestBuild(const QString &model, const QJsonArray &messages, const QJsonArray &tools, const bool stream) const {
    QJsonObject body{
        {"model", model},
        {"messages", messages},
        {"stream", stream}
    };
    if (!tools.isEmpty()) body["tools"] = tools;
    if (stream) body["stream_options"] = QJsonObject{{"include_usage", true}};
    return body;
}

void OpenAIProvider::apikeyGet() {
    const auto job = new QKeychain::ReadPasswordJob("UniComm");
    job->setKey("provider/" + m_id + "/api-key");
    connect(job, &QKeychain::Job::finished, this, [this](QKeychain::Job *j) {
        QString apikey{};
        if (j->error() == QKeychain::NoError) {
            const auto *readJob = static_cast<QKeychain::ReadPasswordJob *>(j);
            apikey = readJob->textData();
            if (!apikey.isEmpty()) m_request.setRawHeader("Authorization", "Bearer " + apikey.toUtf8());
        }
        modelsGet();
        emit apikeyChanged(apikey);
    });
    job->start();
}

void OpenAIProvider::apikeySet(const QString &apikey) {
    const auto job = new QKeychain::WritePasswordJob("UniComm");
    job->setKey("provider/" + m_id + "/api-key");
    job->setTextData(apikey);
    connect(job, &QKeychain::Job::finished, this, [this, apikey](const QKeychain::Job *j) {
        if (j->error() == QKeychain::NoError) {
            m_request.setRawHeader("Authorization", "Bearer " + apikey.toUtf8());
            modelsGet();
        }
        emit apikeyChanged(apikey);
    });
    job->start();
}

void OpenAIProvider::apikeyRemove() {
    const auto job = new QKeychain::DeletePasswordJob("UniComm");
    job->setKey("provider/" + m_id + "/api-key");
    job->start();
}

void OpenAIProvider::modelsGet() {
    m_modelList->clear();
    // read from config
    if (!m_modelFetch) {
        for (const auto &model: m_models) modelAppend(model);
        emit modelsChanged();
    }
    // request from /models
    else {
        auto request = m_request;
        request.setUrl(QUrl(m_baseUrl.toString() + m_modelEndpoint));
        auto *reply = g_networkAccessManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply] {
            const auto models = QJsonDocument::fromJson(reply->readAll()).object().value("data").toArray();
            for (const auto &value: models) {
                modelAppend(modelGet(value.toObject().value("id").toString()));
            }
            reply->deleteLater();
            emit modelsChanged();
        });
    }
}

BaseProvider::Model OpenAIProvider::modelGet(const QString &id) const {
    for (const auto &model: m_models) {
        if (model.id == id) return model;
    }
    return Model{.id = id, .name = id};
}

void OpenAIProvider::modelAppend(const Model &model) const {
    auto *modelItem = new QStandardItem(model.name); // NOLINT
    modelItem->setData(model.id, ProviderModelModel::IdRole);
    modelItem->setData(model.id, ProviderModelModel::ModelIdRole);

    auto *contextItem = new QStandardItem(); // NOLINT
    contextItem->setData(model.contextWindow, Qt::DisplayRole);

    auto *outputItem = new QStandardItem(); // NOLINT
    outputItem->setData(model.maxOutputTokens, Qt::DisplayRole);

    auto *inputItem = new QStandardItem(); // NOLINT
    inputItem->setData(model.input, ProviderModelModel::InputRole);

    auto *primaryItem = new QStandardItem(); // NOLINT
    primaryItem->setData(model.id, ProviderModelModel::ModelIdRole);
    primaryItem->setData(false, ProviderModelModel::PrimaryRole);

    auto *subagentItem = new QStandardItem(); // NOLINT
    subagentItem->setData(model.id, ProviderModelModel::ModelIdRole);
    subagentItem->setData(false, ProviderModelModel::SubagentRole);

    auto *visionItem = new QStandardItem(); // NOLINT
    visionItem->setData(model.id, ProviderModelModel::ModelIdRole);
    visionItem->setData(false, ProviderModelModel::VisionRole);

    auto *actionItem = new QStandardItem(); // NOLINT
    actionItem->setData(model.id, ProviderModelModel::ModelIdRole);
    m_modelList->appendRow({modelItem, contextItem, outputItem, inputItem, primaryItem, subagentItem, visionItem, actionItem});
}

void OpenAIProvider::requestUpdate() {
    m_request.setUrl(QUrl(m_baseUrl.toString() + m_chatEndpoint));
}
