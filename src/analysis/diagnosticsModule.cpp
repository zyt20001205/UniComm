#include "analysis/diagnosticsModule.h"

#include <QJsonArray>
#include <QQmlContext>
#include <QQuickWidget>

#include "globals.h"
#include "core/globalManager.h"
#include "util/uniCast.h"

// public
DiagnosticsModule::DiagnosticsModule()
    : DockWidget("Diagnostics"),
      m_widget(new QQuickWidget()) {
    setWidget(m_widget);
}

DiagnosticsModule::~DiagnosticsModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void DiagnosticsModule::propertySet(const QVariantHash &objects) {
    m_widget->rootContext()->setContextProperty("diagnosticsModule", this);
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("diagnosticMenu", qvariant_cast<QObject *>(objects["diagnosticsModuleDiagnosticMenu"]));

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/analysis/diagnosticsModule.qml"));
}

QVariantList DiagnosticsModule::diagnosticsGet() const {
    QVariantList diagnostics{};
    diagnostics.reserve(m_diagnosticsModelHash.size());
    for (auto it = m_diagnosticsModelHash.cbegin(); it != m_diagnosticsModelHash.cend(); ++it) {
        const auto &documentUrl = it.key();
        diagnostics.append(QVariantHash{
            {"documentUrl", documentUrl},
            {"name", documentUrl.fileName()},
            {"model", QVariant::fromValue<QAbstractItemModel *>(it.value())}
        });
    }
    return diagnostics;
}

void DiagnosticsModule::diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics) {
    QJsonArray filtered{};
    for (const auto &value: diagnostics) {
        if (!value.toObject()["message"].toString().contains("__PLACEHOLDER__")) filtered.append(value);
    }

    auto *diagnosticsModel = m_diagnosticsModelHash.value(documentUrl);
    if (filtered.isEmpty()) {
        if (!diagnosticsModel) return;
        m_diagnosticsModelHash.remove(documentUrl);
        emit changeDiagnostics();
        diagnosticsModel->deleteLater();
        return;
    }

    if (diagnosticsModel) {
        diagnosticsModel->diagnosticsSet(documentUrl, filtered);
        return;
    }

    diagnosticsModel = new DiagnosticsModel(this);
    diagnosticsModel->diagnosticsSet(documentUrl, filtered);
    m_diagnosticsModelHash.insert(documentUrl, diagnosticsModel);
    emit changeDiagnostics();
}

void DiagnosticsModule::indicatorFill(const QVariantHash &position) {
    emit setIndex(
        position["documentUrl"].toUrl(),
        position["startLine"].toInt(),
        position["startCharacter"].toInt());
    emit fillIndicator(
        position["documentUrl"].toUrl(),
        ScintillaIndicator::Highlight,
        position["startLine"].toInt(),
        position["startCharacter"].toInt(),
        position["endLine"].toInt(),
        position["endCharacter"].toInt(),
        1000);
}

// public
DiagnosticsModel::DiagnosticsModel(QObject *parent)
    : QStandardItemModel(parent) {
    setHorizontalHeaderLabels({"", tr("Source"), tr("Code"), tr("Data"), tr("Message")});
}

void DiagnosticsModel::diagnosticsSet(const QUrl &documentUrl, const QJsonArray &diagnostics) {
    setRowCount(0);
    for (const auto &value: diagnostics) {
        const auto diagnostic = value.toObject();
        // range
        const auto range = diagnostic["range"].toObject();
        const auto start = range["start"].toObject();
        const auto end = range["end"].toObject();
        const QVariantHash position = {
            {"documentUrl", documentUrl},
            {"startLine", start["line"].toInt()},
            {"startCharacter", start["character"].toInt()},
            {"endLine", end["line"].toInt()},
            {"endCharacter", end["character"].toInt()}
        };
        // information
        const auto severity = diagnostic["severity"].toInt();
        const auto source = diagnostic["source"].toString();
        const auto message = diagnostic["message"].toString();
        auto *severityItem = new QStandardItem(); // NOLINT
        switch (severity) {
            case LEVEL_ERROR: {
                severityItem->setData(QUrl("qrc:/icon/error.svg"), Qt::DecorationRole);
            }
            break;
            case LEVEL_WARNING: {
                severityItem->setData(QUrl("qrc:/icon/warning.svg"), Qt::DecorationRole);
            }
            break;
            case LEVEL_INFO: {
                severityItem->setData(QUrl("qrc:/icon/info.svg"), Qt::DecorationRole);
            }
            break;
            case LEVEL_HINT: {
                severityItem->setData(QUrl("qrc:/icon/hint.svg"), Qt::DecorationRole);
            }
            break;
            default: break;
        }
        severityItem->setData(severity, SeverityRole);
        severityItem->setData(position, PositionRole);
        severityItem->setData(message, MessageRole);
        auto *sourceItem = new QStandardItem(source); // NOLINT
        auto *codeItem = new QStandardItem(); // NOLINT
        codeItem->setData(diagnostic["code"].toVariant(), Qt::DisplayRole);
        auto *dataItem = new QStandardItem(); // NOLINT
        dataItem->setData(diagnostic["data"].toVariant(), Qt::DisplayRole);
        auto *messageItem = new QStandardItem(uni_cast<QHtmlString>(message)); // NOLINT
        appendRow({severityItem, sourceItem, codeItem, dataItem, messageItem});
    }
}

QHash<int, QByteArray> DiagnosticsModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[SeverityRole] = "severity";
    roles[PositionRole] = "position";
    roles[MessageRole] = "message";
    return roles;
}

QVariant DiagnosticsModel::data(const QModelIndex &index, const int role) const {
    if (!index.isValid()) return {};
    if (role == PositionRole || role == SeverityRole || role == MessageRole) {
        return QStandardItemModel::data(this->index(index.row(), 0), role);
    }
    return QStandardItemModel::data(index, role);
}
