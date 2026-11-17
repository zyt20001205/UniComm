#include "analysis/diagnosticsModule.h"

#include <QJsonArray>
#include <QQuickWidget>
#include <QStandardItemModel>

#include "globals.h"
#include "core/globalManager.h"
#include "util/uniCast.h"

// public
DiagnosticsModule::DiagnosticsModule()
    : DockWidget("Diagnostics"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);
}

DiagnosticsModule::~DiagnosticsModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void DiagnosticsModule::propertySet() {
    m_widget->setInitialProperties({{"diagnosticsModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/analysis/diagnosticsModule.qml"));
}

QVariantList DiagnosticsModule::diagnosticsGet() const {
    QVariantList diagnostics{};
    diagnostics.reserve(m_diagnosticsModelHash.size());
    for (auto it = m_diagnosticsModelHash.cbegin(); it != m_diagnosticsModelHash.cend(); ++it) {
        const auto &documentUrl = it.key();
        const auto *model = it.value();
        diagnostics.append(QVariantHash{
            {"documentUrl", documentUrl},
            {"name", documentUrl.fileName()},
            {"model", QVariant::fromValue<QAbstractItemModel *>(it.value())},
            {"errors", model->property("errors")},
            {"warnings", model->property("warnings")},
            {"infos", model->property("infos")},
            {"hints", model->property("hints")}
        });
    }
    return diagnostics;
}

void DiagnosticsModule::diagnosticsNotification(const QUrl &documentUrl, const QJsonArray &diagnostics) {
    auto *diagnosticsModel = m_diagnosticsModelHash.value(documentUrl);
    if (diagnosticsModel) {
        diagnosticsSet(diagnosticsModel, documentUrl, diagnostics);
        if (diagnosticsModel->rowCount() == 0) {
            m_diagnosticsModelHash.remove(documentUrl);
            diagnosticsModel->deleteLater();
        }
        emit changeDiagnostics();
        return;
    }
    if (diagnostics.isEmpty()) return;

    diagnosticsModel = new QStandardItemModel(this);
    diagnosticsModel->setItemRoleNames({
        {Qt::DisplayRole, "display"},
        {SeverityRole, "severity"},
        {DocumentUrlRole, "documentUrl"},
        {StartLineRole, "startLine"},
        {StartCharacterRole, "startCharacter"},
        {EndLineRole, "endLine"},
        {EndCharacterRole, "endCharacter"},
        {MessageRole, "message"}
    });
    diagnosticsModel->setColumnCount(Count);
    diagnosticsModel->setHeaderData(Severity, Qt::Horizontal, "");
    diagnosticsModel->setHeaderData(Source, Qt::Horizontal, tr("Source"));
    diagnosticsModel->setHeaderData(Code, Qt::Horizontal, tr("Code"));
    diagnosticsModel->setHeaderData(Data, Qt::Horizontal, tr("Data"));
    diagnosticsModel->setHeaderData(Message, Qt::Horizontal, tr("Message"));
    diagnosticsSet(diagnosticsModel, documentUrl, diagnostics);
    if (diagnosticsModel->rowCount() == 0) {
        delete diagnosticsModel;
        return;
    }
    m_diagnosticsModelHash.insert(documentUrl, diagnosticsModel);
    emit changeDiagnostics();
}

// private
void DiagnosticsModule::diagnosticsSet(QStandardItemModel *model, const QUrl &documentUrl, const QJsonArray &diagnostics) {
    int errors = 0;
    int warnings = 0;
    int infos = 0;
    int hints = 0;
    model->setRowCount(0);
    for (const auto &value: diagnostics) {
        const auto diagnostic = value.toObject();
        const auto message = diagnostic["message"].toString();
        if (message.contains("__PLACEHOLDER__")) continue;
        // range
        const auto range = diagnostic["range"].toObject();
        const auto start = range["start"].toObject();
        const auto end = range["end"].toObject();
        const auto startLine = start["line"].toInt();
        const auto startCharacter = start["character"].toInt();
        const auto endLine = end["line"].toInt();
        const auto endCharacter = end["character"].toInt();
        // information
        const auto severity = diagnostic["severity"].toInt();
        switch (severity) {
            case Error: ++errors;
                break;
            case Warning: ++warnings;
                break;
            case Information: ++infos;
                break;
            case Hint: ++hints;
                break;
            default: ;
        }
        const auto source = diagnostic["source"].toString();
        QList<QStandardItem *> row(Count);
        row[Severity] = new QStandardItem(); // NOLINT
        row[Source] = new QStandardItem(source); // NOLINT
        row[Code] = new QStandardItem(); // NOLINT
        row[Code]->setData(diagnostic["code"].toVariant(), Qt::DisplayRole);
        row[Data] = new QStandardItem(); // NOLINT
        row[Data]->setData(diagnostic["data"].toVariant(), Qt::DisplayRole);
        row[Message] = new QStandardItem(uni_cast<QHtmlString>(message)); // NOLINT
        for (auto *item: row) {
            item->setData(severity, SeverityRole);
            item->setData(documentUrl, DocumentUrlRole);
            item->setData(startLine, StartLineRole);
            item->setData(startCharacter, StartCharacterRole);
            item->setData(endLine, EndLineRole);
            item->setData(endCharacter, EndCharacterRole);
            item->setData(message, MessageRole);
        }
        model->appendRow(row);
    }
    model->setProperty("errors", errors);
    model->setProperty("warnings", warnings);
    model->setProperty("infos", infos);
    model->setProperty("hints", hints);
}
