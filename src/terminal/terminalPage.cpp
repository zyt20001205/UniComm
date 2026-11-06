#include "terminal/terminalPage.h"

#include <QCloseEvent>
#include <QDir>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QQmlContext>
#include <QQuickWidget>
#include <QTimer>

#include "globals.h"
#include "core/globalManager.h"
#include "terminal/module/conptyWidget.h"
#include "terminal/module/terminalWidget.h"
#include "terminal/module/vtermWidget.h"

// public
TerminalPage::TerminalPage(const QString &uniqueName, const QVariantHash &session, const QJsonObject &config, const int backend)
    : DockWidget(uniqueName),
      m_backend(backend),
      m_config(config),
      m_session(session),
      m_widget(new QQuickWidget()),
      m_conptyWidget(m_backend == Backend::Conpty ? new ConptyWidget(this) : nullptr),
      m_vtermWidget(new VtermWidget(24, 80, this)) {
    setWidget(m_widget);
    m_widget->installEventFilter(this);

    // set icon
    const auto program = m_session["program"].toUrl();
    if (program.isLocalFile()) {
        const QFileIconProvider provider;
        setIcon(provider.icon(QFileInfo(program.toLocalFile())));
    }
}

TerminalPage::~TerminalPage() {
    stop();
    const auto timestamp = QDateTime::currentDateTime().toString("HH:mm:ss.zzz");
    qDebug() << QString("[%1] %2 destructed").arg(timestamp, uniqueName());
}

void TerminalPage::propertySet(const QVariantHash &objects) {
    m_widget->rootContext()->setContextProperty("global", g_globalManager);
    m_widget->rootContext()->setContextProperty("terminalPage", this);
    m_widget->rootContext()->setContextProperty("vtermWidget", m_vtermWidget);

    m_widget->setResizeMode(QQuickWidget::SizeRootObjectToView);
    m_widget->setSource(QUrl("qrc:/qml/terminal/terminalPage.qml"));
    m_root = m_widget->rootObject();
    m_terminalWidget->propertySet(objects);
}

void TerminalPage::write(const QByteArray &data) const {
    m_vtermWidget->inputWrite(data);
}

void TerminalPage::propertyGet(const QVariantMap &objects) {
    m_terminalItem = qvariant_cast<QObject *>(objects["terminalItem"]);
    auto *terminalItem = qobject_cast<QQuickItem *>(m_terminalItem);
    auto *damageOverlay = qobject_cast<QQuickItem *>(qvariant_cast<QObject *>(objects["damageOverlay"]));

    m_terminalWidget = new TerminalWidget(terminalItem);
    m_terminalWidget->setParentItem(terminalItem);

    connect(m_terminalWidget, &TerminalWidget::keyPressed, m_vtermWidget, &VtermWidget::keyPressed);
    connect(m_terminalWidget, &TerminalWidget::mousePressed, m_vtermWidget, &VtermWidget::mousePressed);
    connect(m_terminalWidget, &TerminalWidget::mouseReleased, m_vtermWidget, &VtermWidget::mouseReleased);
    connect(m_terminalWidget, &TerminalWidget::mouseMoved, m_vtermWidget, &VtermWidget::mouseMoved);
    connect(m_terminalWidget, &TerminalWidget::mouseWheeled, m_vtermWidget, &VtermWidget::mouseWheeled);
    connect(m_terminalWidget, &TerminalWidget::mouseScrolled, m_vtermWidget, &VtermWidget::mouseScrolled);
    connect(m_terminalWidget, &TerminalWidget::startSelection, m_vtermWidget, &VtermWidget::selectionStart);
    connect(m_terminalWidget, &TerminalWidget::updateSelection, m_vtermWidget, &VtermWidget::selectionUpdate);
    connect(m_terminalWidget, &TerminalWidget::getLink, m_vtermWidget, &VtermWidget::linkGet);
    connect(m_terminalWidget, &TerminalWidget::debugDamage, damageOverlay, [damageOverlay](const QRectF &rect) {
        damageOverlay->setPosition(rect.topLeft());
        damageOverlay->setSize(rect.size());
        damageOverlay->setVisible(true);
    });
    connect(m_vtermWidget, &VtermWidget::setScreen, m_terminalWidget, &TerminalWidget::screenSet);
    connect(m_vtermWidget, &VtermWidget::setScreenDamage, m_terminalWidget, &TerminalWidget::screenDamageSet);
    connect(m_vtermWidget, &VtermWidget::setCursorPosition, m_terminalWidget, &TerminalWidget::cursorPositionSet);
    connect(m_vtermWidget, &VtermWidget::setCursorVisible, m_terminalWidget, &TerminalWidget::cursorVisibleSet);
    connect(m_vtermWidget, &VtermWidget::setCursorBlink, m_terminalWidget, &TerminalWidget::cursorBlinkSet);
    connect(m_vtermWidget, &VtermWidget::setTitle, this, &TerminalPage::titleSet);
    connect(m_vtermWidget, &VtermWidget::setCursorShape, m_terminalWidget, &TerminalWidget::cursorShapeSet);
    connect(m_vtermWidget, &VtermWidget::setCursorMode, m_terminalWidget, &TerminalWidget::cursorModeSet);

    connect(m_terminalWidget, &TerminalWidget::resize, this, &TerminalPage::_resize);
    if (m_conptyWidget) {
        connect(m_vtermWidget, &VtermWidget::outputWrite, m_conptyWidget, &ConptyWidget::inputWrite);
        connect(m_conptyWidget, &ConptyWidget::outputWrite, m_vtermWidget, &VtermWidget::inputWrite);
        connect(m_conptyWidget, &ConptyWidget::quit, this, &TerminalPage::close);
        start();
    } else {
        m_vtermWidget->inputWrite("\x1b[20h");
        connect(m_vtermWidget, &VtermWidget::outputWrite, this, &TerminalPage::readTerminal);
    }
}

bool TerminalPage::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_widget) {
        if (event->type() == QEvent::Show) {
            QTimer::singleShot(0, this, [this] {
                auto *terminalItem = qobject_cast<QQuickItem *>(m_terminalItem);
                if (m_terminalWidget->width() <= 0) {
                    auto font = QFont(m_config["fontFamily"].toString(), m_config["fontSize"].toInt());
                    font.setFixedPitch(true);
                    font.setStyleHint(QFont::Monospace, QFont::ForceOutline);
                    m_terminalWidget->fontSet(font);
                    m_terminalWidget->setWidth(terminalItem->width());
                    m_terminalWidget->setHeight(terminalItem->height());
                    connect(terminalItem, &QQuickItem::widthChanged, m_terminalWidget, [this, terminalItem] { m_terminalWidget->setWidth(terminalItem->width()); });
                    connect(terminalItem, &QQuickItem::heightChanged, m_terminalWidget, [this, terminalItem] { m_terminalWidget->setHeight(terminalItem->height()); });
                }
                m_widget->setFocus(Qt::OtherFocusReason);
                m_terminalWidget->forceActiveFocus(Qt::OtherFocusReason);
            });
        } else if (event->type() == QEvent::KeyPress) {
            const auto *keyEvent = static_cast<QKeyEvent *>(event);
            if (keyEvent->key() == Qt::Key_Tab || keyEvent->key() == Qt::Key_Backtab) {
                if (m_vtermWidget) m_vtermWidget->keyPressed(keyEvent->key(), keyEvent->modifiers(), "\t");
                return true;
            }
        }
    }
    return DockWidget::eventFilter(watched, event);
}

// protected
void TerminalPage::closeEvent(QCloseEvent *event) {
    stop();
    deleteLater();
    event->accept();
}

// private
void TerminalPage::start() {
    if (!m_conptyWidget || !m_vtermWidget) return;
    const auto workingDirectory = m_session["workingDirectory"].toUrl();

    const bool started = m_conptyWidget->start(
        m_session["program"].toUrl(),
        m_session["arguments"].toString(),
        workingDirectory.isEmpty() ? g_workspaceUrl : workingDirectory,
        m_session["environment"].toString(),
        m_rows,
        m_cols
    );
    if (!started) close();
}

void TerminalPage::_resize(const int rows, const int cols) {
    if (m_rows == rows && m_cols == cols) return;
    m_rows = rows;
    m_cols = cols;
    if (m_vtermWidget) m_vtermWidget->resize(rows, cols);
    if (m_conptyWidget) m_conptyWidget->resize(rows, cols);
}

void TerminalPage::stop() const {
    if (m_conptyWidget) m_conptyWidget->stop();
}

void TerminalPage::titleSet(const QString &title) {
    auto displayTitle = title.trimmed();
    const auto fileinfo = QFileInfo(QDir::fromNativeSeparators(displayTitle));
    if (fileinfo.isAbsolute() && !fileinfo.fileName().isEmpty()) displayTitle = fileinfo.fileName();
    setTitle(displayTitle);
}
