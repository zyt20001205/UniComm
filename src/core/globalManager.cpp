#include "core/globalManager.h"

#include <QStyleHints>

// public
GlobalManager::GlobalManager(QObject *parent)
    : QObject(parent),
      m_theme(g_mainConfig["theme"].toInt()),
      m_palette({
          {"lightFore", "#242424"},
          {"darkFore", "#ffffff"},
          {"lightForeHover", "#242424"},
          {"darkForeHover", "#ffffff"},
          {"lightForePressed", "#242424"},
          {"darkForePressed", "#ffffff"},
          {"lightForeSelected", "#242424"},
          {"darkForeSelected", "#ffffff"},
          {"lightForeDisabled", "#bdbdbd"},
          {"darkForeDisabled", "#5c5c5c"},

          {"lightBack", "#ffffff"},
          {"darkBack", "#292929"},
          {"lightBackHover", "#f5f5f5"},
          {"darkBackHover", "#3d3d3d"},
          {"lightBackPressed", "#e0e0e0"},
          {"darkBackPressed", "#1f1f1f"},
          {"lightBackSelected", "#ebebeb"},
          {"darkBackSelected", "#383838"},

          {"lightStroke", "#d1d1d1"},
          {"darkStroke", "#666666"},
          {"lightStrokePressed", "#b3b3b3"},
          {"darkStrokePressed", "#6b6b6b"},

          {"lightBrandFore", "#ffffff"},
          {"darkBrandFore", "#ffffff"},

          {"lightBrandBack", "#0f6cbd"},
          {"darkBrandBack", "#115ea3"},
          {"lightBrandBackSelected", "#0f548c"},
          {"darkBrandBackSelected", "#0f548c"},

          {"lightBrandStroke", "#0f6cbd"},
          {"darkBrandStroke", "#479ef5"},
          {"lightBrandLink", "#115ea3"},
          {"darkBrandLink", "#479ef5"},

          {"lightSuccessFore2", "#094509"},
          {"darkSuccessFore2", "#9fd89f"},
          {"lightSuccessBack2", "#9fd89f"},
          {"darkSuccessBack2", "#094509"},
          {"lightSuccessFore3", "#107c10"},
          {"darkSuccessFore3", "#9fd89f"},
          {"lightSuccessBack3", "#107c10"},
          {"darkSuccessBack3", "#107c10"},

          {"lightWarningFore2", "#8a3707"},
          {"darkWarningFore2", "#fdcfb4"},
          {"lightWarningBack2", "#fdcfb4"},
          {"darkWarningBack2", "#8a3707"},
          {"lightWarningFore3", "#bc4b09"},
          {"darkWarningFore3", "#f98845"},
          {"lightWarningBack3", "#f7630c"},
          {"darkWarningBack3", "#f7630c"},

          {"lightDangerFore2", "#6e0811"},
          {"darkDangerFore2", "#eeacb2"},
          {"lightDangerBack2", "#eeacb2"},
          {"darkDangerBack2", "#6e0811"},
          {"lightDangerFore3", "#c50f1f"},
          {"darkDangerFore3", "#eeacb2"},
          {"lightDangerBack3", "#c50f1f"},
          {"darkDangerBack3", "#c50f1f"},
      }),
      m_styleSheet({
          // Light
          "QMainWindow { background-color: " + m_palette["lightBack"] + "; }"
          + "QScrollBar:horizontal { background-color: transparent; height: 10px; margin: 0px; border: none; }"
          + "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; }"
          + "QScrollBar:vertical { background-color: transparent; width: 10px; margin: 0px; border: none; }"
          + "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; background: none; }"
          + "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { background-color: " + m_palette["lightStroke"] + "; border-radius: 3px; margin: 2px; }"
          + "QScrollBar::handle:horizontal { min-width: 20px; }"
          + "QScrollBar::handle:vertical { min-height: 20px; }"
          + "QScrollBar::handle:horizontal:pressed, QScrollBar::handle:vertical:pressed { background-color: " + m_palette["lightStrokePressed"] + "; }"
          + "QAbstractScrollArea::corner { background-color: " + m_palette["lightBack"] + "; }"

          + "QTabBar::tab { background: transparent; border: none; color: " + m_palette["lightFore"] +
          "; font-family: 'Segoe UI'; font-size: 14px; margin: 10px 10px 6px 10px; padding: 0px 0px 4px 16px;}"
          + "QTabBar::tab:hover { border-bottom: 3px solid " + m_palette["lightBackHover"] + " }"
          + "QTabBar::tab:selected { border-bottom: 3px solid " + m_palette["lightBrandBack"] + "; font-weight: 600; }"
          + "QTabBar::close-button { border-radius: 4px; margin: 2px; image: url(:/icon/dismissLight.svg); }"
          + "QTabBar::close-button:hover { background-color: " + m_palette["lightBackHover"] + "; }"
          + "QTabBar::close-button:pressed { background-color: " + m_palette["lightBackPressed"] + "; }"
          + "QTabWidget { background-color: " + m_palette["lightBack"] + "; }"
          + "QTabWidget::pane { border: 1px solid #cccccc; border-radius: 2px; border-top: none; }",

          // Dark
          "QMainWindow { background-color: " + m_palette["darkBack"] + "; }"
          + "QScrollBar:horizontal { background-color: transparent; height: 10px; margin: 0px; border: none; }"
          + "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0px; background: none; }"
          + "QScrollBar:vertical { background-color: transparent; width: 10px; margin: 0px; border: none; }"
          + "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { width: 0px; background: none; }"
          + "QScrollBar::handle:horizontal, QScrollBar::handle:vertical { background-color: " + m_palette["darkStroke"] + "; border-radius: 3px; margin: 2px; }"
          + "QScrollBar::handle:horizontal { min-width: 20px; }"
          + "QScrollBar::handle:vertical { min-height: 20px; }"
          + "QScrollBar::handle:horizontal:pressed, QScrollBar::handle:vertical:pressed { background-color: " + m_palette["darkStrokePressed"] + "; }"
          + "QAbstractScrollArea::corner { background-color: " + m_palette["darkBack"] + "; }"

          + "QTabBar::tab { background: transparent; border: none; color: " + m_palette["darkFore"] +
          "; font-family: 'Segoe UI'; font-size: 14px; margin: 10px 10px 6px 10px; padding: 0px 0px 4px 16px;}"
          + "QTabBar::tab:hover { border-bottom: 3px solid " + m_palette["darkBackHover"] + " }"
          + "QTabBar::tab:selected { border-bottom: 3px solid " + m_palette["darkBrandBack"] + "; font-weight: 600; }"
          + "QTabBar::close-button { border-radius: 4px; margin: 2px; image: url(:/icon/dismissDark.svg); }"
          + "QTabBar::close-button:hover { background-color: " + m_palette["darkBackHover"] + "; }"
          + "QTabBar::close-button:pressed { background-color: " + m_palette["darkBackPressed"] + "; }"
          + "QTabWidget { background-color: " + m_palette["darkBack"] + "; }"
          + "QTabWidget::pane { border: 1px solid #cccccc; border-radius: 2px; border-top: none; }"
      }) {
    if (m_theme == Theme::Light) QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Light);
    else QGuiApplication::styleHints()->setColorScheme(Qt::ColorScheme::Dark);
    qApp->setStyleSheet(m_styleSheet[m_theme]);
}
