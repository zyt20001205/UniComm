#include "agent/provider/providerModule.h"

#include <QFile>
#include <QJsonDocument>
#include <QStandardItem>

#include "agent/provider/baseProvider.h"
#include "agent/provider/openAIProvider.h"

ProviderModule::ProviderModule(const QJsonObject &providers, const QJsonObject &defaults, QObject *parent)
    : QObject(parent),
      m_providerConfigs(providers),
      m_default(defaults),
      m_providerModel(new ProviderModel(this)) {
}

void ProviderModule::propertySet(const QVariantHash &objects) {
    m_modelMenu = qvariant_cast<QObject *>(objects["agentModuleModelMenu"]);
    m_modelMenu->setProperty("providerModel", QVariant::fromValue(m_providerModel));
}

void ProviderModule::initialize() {
    QFile file(":/config/api.json");
    if (!file.open(QIODevice::ReadOnly)) return;
    m_catalog = QJsonDocument::fromJson(file.readAll()).object();
    const auto providerConfigs = m_providerConfigs;
    for (auto iterator = providerConfigs.constBegin(); iterator != providerConfigs.constEnd(); ++iterator) {
        providerInsert(iterator.key(), iterator.value().toObject());
    }
}

void ProviderModule::apikeySet(const QString &provider, const QString &apikey) const {
    m_providers.value(provider)->apikeySet(apikey);
}

void ProviderModule::providerInsert(const QString &id, const QJsonObject &overrides) {
    if (m_providers.contains(id)) return;

    const auto custom = !m_catalog.contains(id);
    auto config = m_catalog.value(id).toObject();
    for (auto iterator = overrides.constBegin(); iterator != overrides.constEnd(); ++iterator) config[iterator.key()] = iterator.value();
    auto *provider = new OpenAIProvider(id, config, this); // NOLINT
    m_providerConfigs[id] = overrides;
    m_providers[id] = provider;

    auto *item = new QStandardItem(provider->nameGet()); // NOLINT
    const auto icon = QFile::exists(":/icon/" + id + ".svg") ? "qrc:/icon/" + id + ".svg" : "qrc:/icon/model.svg";
    item->setData(QUrl(icon), Qt::DecorationRole);
    item->setData(id, ProviderModel::IdRole);
    item->setData("", ProviderModel::ApikeyRole);
    item->setData(provider->baseUrlGet(), ProviderModel::BaseUrlRole);
    item->setData(custom, ProviderModel::CustomRole);
    item->setData(provider->chatEndpointGet(), ProviderModel::ChatEndpointRole);
    item->setData(provider->modelEndpointGet(), ProviderModel::ModelEndpointRole);
    item->setData(config, ProviderModel::ConfigRole);
    item->setData(QVariant::fromValue(provider->modelListGet()), ProviderModel::ModelsRole);
    m_providerModel->appendRow(item);

    connect(provider, &BaseProvider::apikeyChanged, this, [item](const QString &apikey) {
        item->setData(apikey, ProviderModel::ApikeyRole);
    });
    connect(provider, &BaseProvider::modelsChanged, this, [this] {
        _defaultSet();
        emit modelsChanged();
    });
    provider->apikeyGet();
}

void ProviderModule::providerEdit(const QString &id, const QJsonObject &config) {
    m_providerConfigs[id] = config;

    auto *provider = static_cast<OpenAIProvider *>(m_providers.value(id));
    provider->configSet(config);
    const auto indexes = m_providerModel->match(m_providerModel->index(0, 0), ProviderModel::IdRole, id, 1, Qt::MatchExactly);
    auto *item = m_providerModel->itemFromIndex(indexes.constFirst());
    item->setText(provider->nameGet());
    item->setData(provider->baseUrlGet(), ProviderModel::BaseUrlRole);
    item->setData(provider->chatEndpointGet(), ProviderModel::ChatEndpointRole);
    item->setData(provider->modelEndpointGet(), ProviderModel::ModelEndpointRole);
    item->setData(config, ProviderModel::ConfigRole);
    provider->modelsGet();
}

void ProviderModule::providerModelsGet(const QString &id) const {
    m_providers.value(id)->modelsGet();
}

void ProviderModule::providerRemove(const QString &id) {
    const auto indexes = m_providerModel->match(m_providerModel->index(0, 0), ProviderModel::IdRole, id, 1, Qt::MatchExactly);
    if (indexes.isEmpty()) return;
    const auto row = indexes.constFirst().row();
    auto *provider = m_providers.take(id);
    provider->disconnect(this);
    provider->apikeyRemove();
    provider->deleteLater();
    m_providerConfigs.remove(id);
    m_providerModel->removeRow(row);
}

void ProviderModule::defaultSet(const QJsonObject &defaults) {
    m_default = defaults;
    _defaultSet();
}

BaseProvider *ProviderModule::providerGet(const QString &id) const {
    return m_providers.value(id);
}

// private
void ProviderModule::_defaultSet() const {
    const auto primary = m_default.value("primary").toObject();
    const auto subagent = m_default.value("subagent").toObject();
    const auto vision = m_default.value("vision").toObject();
    for (auto iterator = m_providers.constBegin(); iterator != m_providers.constEnd(); ++iterator) {
        auto *models = static_cast<OpenAIProvider *>(iterator.value())->modelListGet();
        for (auto row = 0; row < models->rowCount(); ++row) {
            auto *item = models->item(row);
            const auto model = item->data(ProviderModelModel::ModelIdRole).toString();
            models->item(row, 4)->setData(primary.value("provider").toString() == iterator.key() && primary.value("model").toString() == model, ProviderModelModel::PrimaryRole);
            models->item(row, 5)->setData(subagent.value("provider").toString() == iterator.key() && subagent.value("model").toString() == model, ProviderModelModel::SubagentRole);
            models->item(row, 6)->setData(vision.value("provider").toString() == iterator.key() && vision.value("model").toString() == model, ProviderModelModel::VisionRole);
        }
    }
}

// public
QHash<int, QByteArray> ProviderModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[IdRole] = "id";
    roles[ApikeyRole] = "apikey";
    roles[BaseUrlRole] = "baseUrl";
    roles[CustomRole] = "custom";
    roles[ChatEndpointRole] = "chatEndpoint";
    roles[ModelEndpointRole] = "modelEndpoint";
    roles[ConfigRole] = "config";
    roles[ModelsRole] = "models";
    return roles;
}

ProviderModelModel::ProviderModelModel(QObject *parent)
    : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({tr("Model"), tr("Context"), tr("Output"), tr("Input"), tr("Main"), tr("Sub"), tr("Vision"), {}});
    connect(this, &QAbstractItemModel::modelReset, this, &ProviderModelModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsInserted, this, &ProviderModelModel::rowsChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &ProviderModelModel::rowsChanged);
}

void ProviderModelModel::clear() {
    QStandardItemModel::clear();
    setHorizontalHeaderLabels({tr("Model"), tr("Context"), tr("Output"), tr("Input"), tr("Main"), tr("Sub"), tr("Vision"), {}});
}

int ProviderModelModel::rowsGet() const {
    return rowCount();
}

QHash<int, QByteArray> ProviderModelModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[IdRole] = "id";
    roles[ModelIdRole] = "modelId";
    roles[InputRole] = "input";
    roles[PrimaryRole] = "primary";
    roles[SubagentRole] = "subagent";
    roles[VisionRole] = "vision";
    return roles;
}
