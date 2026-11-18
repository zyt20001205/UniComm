#include "debug/breakpointModule.h"

#include <QQuickWidget>
#include <algorithm>

#include "globals.h"
#include "core/globalManager.h"

// public
BreakpointModule::BreakpointModule()
    : DockWidget("Breakpoint"),
      m_widget(new QQuickWidget(g_globalManager->engineGet(), nullptr)) {
    setWidget(m_widget);

    const auto config = g_workspaceConfig["breakpointConfig"].toObject();
    for (auto file = config.begin(); file != config.end(); ++file) {
        const QUrl url(file.key());
        const auto lines = file.value().toObject();
        for (auto it = lines.begin(); it != lines.end(); ++it) {
            m_breakpoints[url].insert(it.key().toInt(), it.value().toObject().toVariantHash());
        }
    }
}

BreakpointModule::~BreakpointModule() {
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 module destructed").arg(timestamp, uniqueName());
}

void BreakpointModule::propertySet() {
    m_widget->setInitialProperties({{"breakpointModule", QVariant::fromValue(this)}});
    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/debug/breakpointModule.qml"));
}

void BreakpointModule::breakpointConfigSave() const {
    QJsonObject config{};
    QReadLocker locker(&m_lock);
    for (auto file = m_breakpoints.cbegin(); file != m_breakpoints.cend(); ++file) {
        QJsonObject lines{};
        for (auto it = file->cbegin(); it != file->cend(); ++it) {
            lines.insert(QString::number(it.key()), QJsonObject::fromVariantHash(it.value()));
        }
        config.insert(file.key().toString(), lines);
    }
    g_workspaceConfig["breakpointConfig"] = config;
}

QVariantList BreakpointModule::breakpointsGet() const {
    QReadLocker locker(&m_lock);
    QVariantList breakpoints{};
    for (auto it = m_breakpoints.cbegin(); it != m_breakpoints.cend(); ++it) {
        const auto &url = it.key();
        const auto name = url.fileName();
        const auto &file = it.value();
        auto lines = file.keys();
        std::ranges::sort(lines);
        for (const int line: lines) {
            const auto &session = file.constFind(line).value();
            breakpoints.append(QVariantHash{
                {"documentUrl", url},
                {"name", name},
                {"line", line},
                {"enabled", session.value("enabled").toBool()},
                {"condition", session.value("condition").toString()}
            });
        }
    }
    return breakpoints;
}

QVariantHash BreakpointModule::breakpointGet(const QUrl &documentUrl, const int line) const {
    QReadLocker locker(&m_lock);
    const auto file = m_breakpoints.constFind(documentUrl);
    return file == m_breakpoints.cend() ? QVariantHash{} : file->value(line);
}

void BreakpointModule::breakpointAdd(const QUrl &documentUrl, const int line) {
    {
        QWriteLocker locker(&m_lock);
        m_breakpoints[documentUrl].insert(line, {{"condition", ""}, {"enabled", true}});
    }

    emit addMarker(documentUrl, ScintillaMarker::BreakpointEnabled, line, -1);
    emit breakpointsChanged();
    breakpointShow(documentUrl, line);
}

void BreakpointModule::breakpointDelete(const QUrl &documentUrl, const int line) {
    bool enabled;
    {
        QWriteLocker locker(&m_lock);
        auto file = m_breakpoints.find(documentUrl);
        if (file == m_breakpoints.end()) return;
        auto entry = file->find(line);
        if (entry == file->end()) return;
        enabled = entry->value("enabled").toBool();
        file->erase(entry);
        if (file->isEmpty()) m_breakpoints.erase(file);
    }

    emit deleteMarker(documentUrl, enabled ? ScintillaMarker::BreakpointEnabled : ScintillaMarker::BreakpointDisabled, line);
    emit breakpointsChanged();
}

void BreakpointModule::breakpointConfigure(const QUrl &documentUrl, const int line, const QVariantHash &session) {
    bool wasEnabled, enabled;
    {
        QWriteLocker locker(&m_lock);
        auto file = m_breakpoints.find(documentUrl);
        if (file == m_breakpoints.end()) return;
        auto entry = file->find(line);
        if (entry == file->end()) return;
        wasEnabled = entry->value("enabled").toBool();
        entry->insert(session);
        enabled = entry->value("enabled").toBool();
    }

    if (wasEnabled != enabled) {
        emit deleteMarker(documentUrl, wasEnabled ? ScintillaMarker::BreakpointEnabled : ScintillaMarker::BreakpointDisabled, line);
        emit addMarker(documentUrl, enabled ? ScintillaMarker::BreakpointEnabled : ScintillaMarker::BreakpointDisabled, line, -1);
    }
    emit breakpointsChanged();
}

void BreakpointModule::breakpointToggle(const QUrl &documentUrl, const int line) {
    if (breakpointGet(documentUrl, line).isEmpty()) breakpointAdd(documentUrl, line);
    else breakpointDelete(documentUrl, line);
}

// private
void BreakpointModule::breakpointShow(const QUrl &documentUrl, const int line) {
    if (!isOpen()) open();
    raise();
    emit showBreakpoint(documentUrl, line);
}
