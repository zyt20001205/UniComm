#include "core/themeManager.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QStyleHints>

#include "globals.h"

// public
ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent),
      m_mode(g_mainConfig["theme"].toInt()) {
    if (m_mode == Theme::Light) QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    else QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);

    const QDir themeDir(QCoreApplication::applicationDirPath());
    QFile themeFile(themeDir.filePath(QString("theme/%1.json").arg(m_mode)));
    if (!themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning().noquote() << "Cannot load theme:" << themeFile.fileName() << themeFile.errorString();
        return;
    }
    QJsonParseError error{};
    const auto document = QJsonDocument::fromJson(themeFile.readAll(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        qWarning().noquote() << "Invalid theme JSON:" << themeFile.fileName() << (error.error != QJsonParseError::NoError ? error.errorString() : QString("expected an object"));
        return;
    }
    m_config = document.object();
    if (!m_config["general"].isObject()) {
        qWarning().noquote() << "Missing or invalid theme general object:" << themeFile.fileName();
        return;
    }
    m_palette = m_config["general"].toObject();
    bool valid = true;
    for (const auto *key: {
             "fore", "foreHover", "forePressed", "foreSelected", "foreDisabled",
             "back", "backHover", "backPressed", "backSelected",
             "stroke", "strokePressed",
             "brandFore", "brandBack", "brandBackSelected", "brandStroke", "brandLink",
             "successFore2", "successBack2", "successFore3", "successBack3",
             "warningFore2", "warningBack2", "warningFore3", "warningBack3",
             "dangerFore2", "dangerBack2", "dangerFore3", "dangerBack3"
         }) {
        const auto value = m_palette[key];
        if (!value.isString() || !QColor(value.toString()).isValid()) {
            qWarning().noquote() << "Missing or invalid theme color:" << themeFile.fileName() << QString("general.%1").arg(key);
            valid = false;
        }
    }
    if (!valid) return;

    const QString closeIcon = m_mode == Theme::Light ? ":/icon/dismissLight.svg" : ":/icon/dismissDark.svg";
    qApp->setStyleSheet(
        "QMainWindow { background-color: " + backGet() + "; }"
        + "QScrollBar:horizontal { background-color: transparent; height: 10px; margin: 0px; border: none; }"
        + "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; }"
        + "QScrollBar:vertical { background-color: transparent; width: 10px; margin: 0px; border: none; }"
        + "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; background: none; }"
        + "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { background-color: " + strokeGet() + "; border-radius: 3px; margin: 2px; }"
        + "QScrollBar::handle:horizontal { min-width: 20px; }"
        + "QScrollBar::handle:vertical { min-height: 20px; }"
        + "QScrollBar::handle:horizontal:pressed, QScrollBar::handle:vertical:pressed { background-color: " + strokePressedGet() + "; }"
        + "QAbstractScrollArea::corner { background-color: " + backGet() + "; }"

        + "QTabBar::tab { background: transparent; border: none; color: " + foreGet() +
        "; font-family: 'Segoe UI'; font-size: 14px; margin: 10px 10px 6px 10px; padding: 0px 0px 4px 16px;}"
        + "QTabBar::tab:hover { border-bottom: 3px solid " + backHoverGet() + " }"
        + "QTabBar::tab:selected { border-bottom: 3px solid " + brandBackGet() + "; font-weight: 600; }"
        + "QTabBar::close-button { border-radius: 4px; margin: 2px; image: url(" + closeIcon + "); }"
        + "QTabBar::close-button:hover { background-color: " + backHoverGet() + "; }"
        + "QTabBar::close-button:pressed { background-color: " + backPressedGet() + "; }"
        + "QTabWidget { background-color: " + backGet() + "; }"
        + "QTabWidget::pane { border: 1px solid " + strokeGet() + "; border-radius: 2px; border-top: none; }");
}
