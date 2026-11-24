#include "mainWindow/toastModule.h"

#include <QQuickItem>
#include <QScreen>
#include <QTimer>
#include <QVariant>
#include <utility>

#include "globals.h"

ToastModule::ToastModule(QQmlEngine *engine, QWindow &owner)
    : QQuickView(engine, nullptr),
      m_owner(owner) {
    setResizeMode(SizeViewToRootObject);
    setColor(Qt::transparent);
    setFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus);
    setTransientParent(&m_owner);

    connect(this, &QWindow::widthChanged, this, &ToastModule::geometryUpdate);
    connect(this, &QWindow::heightChanged, this, &ToastModule::geometryUpdate);
    connect(&m_owner, &QWindow::screenChanged, this, &ToastModule::geometryUpdate);
    connect(&m_owner, &QWindow::visibilityChanged, this, &ToastModule::geometryUpdate);

    setInitialProperties({{"toastModule", QVariant::fromValue(this)}});
    setSource(QUrl("qrc:/qml/mainWindow/Toast.qml"));
    m_root = rootObject();
    hide();

    geometryUpdate();
    QTimer::singleShot(0, this, &ToastModule::geometryUpdate);
}

void ToastModule::show(const int level, const QString &title, const QString &text, QList<ToastAction> actions, const int duration) {
    if (!m_root) return;

    QVariantList actionModel;
    QList<std::function<void()> > callbacks;
    for (auto &action: actions) {
        if (action.text.isEmpty() || !action.callback) continue;
        actionModel.append(QVariantMap{
            {"actionText", action.text},
            {"actionIndex", callbacks.size()}
        });
        callbacks.append(std::move(action.callback));
    }

    int actionGroupId = -1;
    if (!callbacks.isEmpty()) {
        actionGroupId = m_actionGroupId++;
        m_callbackGroups.insert(actionGroupId, std::move(callbacks));
    }
    const int toastDuration = duration == 0 && level != ToastLevel::Error ? (actionGroupId == -1 ? 3000 : 5000) : duration;
    const QVariant actionModelValue = actionModel;
    const bool invoked = QMetaObject::invokeMethod(
        m_root,
        "show",
        Q_ARG(int, level),
        Q_ARG(QString, title),
        Q_ARG(QString, text),
        Q_ARG(QVariant, actionModelValue),
        Q_ARG(int, actionGroupId),
        Q_ARG(int, toastDuration)
    );
    if (!invoked) m_callbackGroups.remove(actionGroupId);
}

void ToastModule::actionTrigger(const int actionGroupId, const int actionIndex) {
    auto callbacks = m_callbackGroups.take(actionGroupId);
    if (actionIndex < 0 || actionIndex >= callbacks.size()) return;

    auto callback = std::move(callbacks[actionIndex]);
    if (callback) callback();
}

void ToastModule::actionRemove(const int actionGroupId) {
    m_callbackGroups.remove(actionGroupId);
}

void ToastModule::geometryUpdate() {
    auto *screen = m_owner.screen();
    if (!screen) {
        hide();
        return;
    }

    setScreen(screen);
    const auto geometry = screen->geometry();
    setPosition(
        geometry.x() + geometry.width() - width() - 20,
        geometry.y() + geometry.height() - height() - 20
    );

    const auto visibility = m_owner.visibility();
    const bool shouldShow = height() > 0
                            && visibility != Minimized
                            && visibility != Hidden;
    if (shouldShow) QQuickView::show();
    else hide();
}
