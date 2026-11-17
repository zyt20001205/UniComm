#include "core/globalManager.h"

#include <QQmlContext>
#include <QQmlEngine>

#include "core/themeManager.h"

// public
GlobalManager::GlobalManager(QObject *parent)
    : QObject(parent),
      m_theme(new ThemeManager(this)),
      m_engine(new QQmlEngine(this)) {
    m_engine->rootContext()->setContextProperty("global", this);
}

QJsonObject GlobalManager::themeGet() const {
    return m_theme->configGet();
}
