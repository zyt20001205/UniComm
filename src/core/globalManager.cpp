#include "core/globalManager.h"

// public
GlobalManager::GlobalManager(QObject *parent)
    : QObject(parent),
      m_theme(new ThemeManager(this)) {
}
