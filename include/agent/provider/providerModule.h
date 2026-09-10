#ifndef UNICOMM_PROVIDERMODULE_H
#define UNICOMM_PROVIDERMODULE_H

#include <QJsonObject>
#include <QObject>
#include <QStandardItemModel>

class BaseProvider;
class ProviderModel;

class ProviderModule final : public QObject {
    Q_OBJECT

public:
    explicit ProviderModule(const QJsonObject &providers, const QJsonObject &defaults, QObject *parent = nullptr);

    ~ProviderModule() override = default;

    void propertySet(const QVariantHash &objects);

    void initialize();

    void apikeySet(const QString &provider, const QString &apikey) const;

    void providerInsert(const QString &id, const QJsonObject &overrides);

    void providerEdit(const QString &id, const QJsonObject &config);

    void providerModelsGet(const QString &id) const;

    void providerRemove(const QString &id);

    void defaultSet(const QJsonObject &defaults);

    [[nodiscard]] BaseProvider *providerGet(const QString &id) const;

    [[nodiscard]] bool providerExists(const QString &id) const {
        return m_providers.contains(id);
    }

    [[nodiscard]] ProviderModel *providerModelGet() const {
        return m_providerModel;
    }

signals:
    void modelsChanged();

private:
    void _defaultSet() const;

    QObject *m_modelMenu{};
    QJsonObject m_catalog{};
    QJsonObject m_providerConfigs{};
    QJsonObject m_default{};
    ProviderModel *m_providerModel{};
    QHash<QString, BaseProvider *> m_providers{};
};

class ProviderModel final : public QStandardItemModel {
    Q_OBJECT

public:
    using QStandardItemModel::QStandardItemModel;

    enum Role {
        IdRole = Qt::UserRole + 1,
        ApikeyRole,
        BaseUrlRole,
        CustomRole,
        ChatEndpointRole,
        ModelEndpointRole,
        ConfigRole,
        ModelsRole
    };

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
};

class ProviderModelModel final : public QStandardItemModel {
    Q_OBJECT
    Q_PROPERTY(int rows READ rowsGet NOTIFY rowsChanged)

public:
    explicit ProviderModelModel(QObject *parent = nullptr);

    enum Role {
        IdRole = Qt::UserRole + 1,
        ModelIdRole,
        InputRole,
        PrimaryRole,
        SubagentRole,
        VisionRole
    };

    void clear();

    [[nodiscard]] int rowsGet() const;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

signals:
    void rowsChanged();
};

#endif //UNICOMM_PROVIDERMODULE_H
