#include "agent/module/hookModule.h"

#include <QFileInfo>
#include <QJsonArray>

#include "globals.h"
#include "core/globalManager.h"
#include "mainWindow/toastModule.h"
#include "runtime/threadpoolModule.h"
#include "util/uniCast.h"

// public
HookModule::HookModule(const QJsonArray &config, QObject *parent)
    : QObject(parent),
      m_config(config),
      m_hookModel(new HookModel(this)) {
    auto *turnStartItem = new QStandardItem(tr("Turn started")); // NOLINT
    turnStartItem->setData(tr("Run scripts after an agent turn starts."), HookModel::DescriptionRole);
    m_hookModel->appendRow(turnStartItem);
    auto *permissionItem = new QStandardItem(tr("Permission requested")); // NOLINT
    permissionItem->setData(tr("Run scripts when an agent requests permission."), HookModel::DescriptionRole);
    m_hookModel->appendRow(permissionItem);
    auto *userInputItem = new QStandardItem(tr("User input requested")); // NOLINT
    userInputItem->setData(tr("Run scripts when an agent asks the user for input."), HookModel::DescriptionRole);
    m_hookModel->appendRow(userInputItem);
    auto *turnFinishItem = new QStandardItem(tr("Turn finished")); // NOLINT
    turnFinishItem->setData(tr("Run scripts after an agent turn finishes."), HookModel::DescriptionRole);
    m_hookModel->appendRow(turnFinishItem);
    hookUpdate(Event::TurnStart);
    hookUpdate(Event::PermissionRequest);
    hookUpdate(Event::UserInputRequest);
    hookUpdate(Event::TurnFinish);
}

HookModel *HookModule::hookModelGet() const {
    return m_hookModel;
}

const QJsonArray &HookModule::configGet() const {
    return m_config;
}

void HookModule::propertySet(const QVariantHash &objects) {
    m_toast = qvariant_cast<ToastModule *>(objects["mainWindowToast"]);
}

void HookModule::hookEnabledSet(const int event, const bool enabled) {
    auto hook = m_config[event].toObject();
    hook["enabled"] = enabled;
    m_config[event] = hook;
    hookUpdate(event);
}

void HookModule::hookScriptInsert(const int event, const QUrl &documentUrl) {
    auto hook = m_config[event].toObject();
    auto scripts = hook["scripts"].toArray();
    if (scripts.contains(documentUrl.toString())) return;
    scripts.append(documentUrl.toString());
    hook["enabled"] = true;
    hook["scripts"] = scripts;
    m_config[event] = hook;
    hookUpdate(event);
}

void HookModule::hookScriptRemove(const int event, const QUrl &documentUrl) {
    auto hook = m_config[event].toObject();
    auto scripts = hook["scripts"].toArray();
    for (qsizetype index = 0; index < scripts.size(); ++index) {
        if (scripts.at(index).toString() != documentUrl.toString()) continue;
        scripts.removeAt(index);
        break;
    }
    hook["scripts"] = scripts;
    m_config[event] = hook;
    hookUpdate(event);
}

void HookModule::hookRun(const int event) const {
    const auto hook = m_config[event].toObject();
    if (!hook["enabled"].toBool()) return;
    for (const auto &value: hook["scripts"].toArray()) {
        const QUrl documentUrl(value.toString());
        if (!QFileInfo(documentUrl.toLocalFile()).isFile()) {
            m_toast->show(ToastLevel::Warning, tr("Hook"), tr("Hook script does not exist: %1").arg(documentUrl.toLocalFile()));
            continue;
        }
        g_globalManager->threadpoolModuleGet()->threadStart(documentUrl, RunMode::Run);
    }
}

// private
QVariantList HookModule::scriptsGet(const int event) const {
    QVariantList scripts{};
    for (const auto &value: m_config[event].toObject()["scripts"].toArray()) {
        const QUrl documentUrl(value.toString());
        scripts.append(QVariantMap{
            {"documentUrl", documentUrl},
            {"fileName", documentUrl.fileName()},
            {"decoration", uni_cast<QFileIcon>(documentUrl).value}
        });
    }
    return scripts;
}

void HookModule::hookUpdate(const int event) const {
    auto *item = m_hookModel->item(event);
    const auto hook = m_config[event].toObject();
    item->setData(hook["enabled"].toBool(), HookModel::EnabledRole);
    item->setData(scriptsGet(event), HookModel::ScriptsRole);
}

// public
HookModel::HookModel(QObject *parent)
    : QStandardItemModel(parent) {
}

QHash<int, QByteArray> HookModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[DescriptionRole] = "description";
    roles[EnabledRole] = "enabled";
    roles[ScriptsRole] = "scripts";
    return roles;
}
