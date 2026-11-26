#include "document/module/diffOverlay.h"

#include <QEvent>
#include <QScrollBar>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/scintillaWidget.h"

DiffOverlay::DiffOverlay(ScintillaWidget *ours, ScintillaWidget *base, ScintillaWidget *theirs, QWidget *parent)
    : QQuickWidget(g_globalManager->engineGet(), parent), m_editors{ours, base, theirs} {
    setClearColor(Qt::transparent);
    setAttribute(Qt::WA_AlwaysStackOnTop);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    m_refresh.setSingleShot(true);
    connect(&m_refresh, &QTimer::timeout, this, &DiffOverlay::regionsChanged);
    for (auto *editor : m_editors) {
        editor->parentWidget()->installEventFilter(this);
        editor->installEventFilter(this);
        editor->viewport()->installEventFilter(this);
        connect(editor->verticalScrollBar(), &QScrollBar::valueChanged, this, [this] { m_refresh.start(); });
        connect(editor, &ScintillaEditBase::zoom, this, [this] { m_refresh.start(); });
        connect(editor, &ScintillaEdit::updateUi, this, [this](Scintilla::Update flags) {
            if (static_cast<int>(flags) & SC_UPDATE_CONTENT) m_refresh.start();
        });
    }
    setResizeMode(SizeRootObjectToView);
    setInitialProperties({{"diffOverlay", QVariant::fromValue(this)}});
    setSource(QUrl("qrc:/qml/document/module/diffOverlay.qml"));
}

void DiffOverlay::blocksSet(QList<Block> blocks) {
    m_blocks = std::move(blocks);
    m_refresh.start();
}

QVariantList DiffOverlay::regionsGet() const {
    if (m_blocks.isEmpty()) return {};
    const auto origin = [this](const QWidget *widget) { return widget->mapTo(parentWidget(), QPoint{}) - pos(); };
    std::array<QRect, 3> textAreas{};
    std::array<int, 3> first{}, height{};
    for (int i = 0; i < 3; ++i) {
        const auto *editor = m_editors[i];
        const QRect view(origin(editor->viewport()), editor->viewport()->size());
        textAreas[i] = view.adjusted(editor->marginWidthGet() + editor->send(SCI_GETMARGINLEFT), 0, -editor->send(SCI_GETMARGINRIGHT), 0);
        first[i] = editor->firstVisibleLine();
        height[i] = editor->heightGet();
    }

    // Alternate text areas and gaps. Each is clipped to its own visible editor height.
    QVariantList regions{};
    for (int i = 0; i < 5; ++i) {
        const int left = i / 2, right = (i + 1) / 2;
        QRect area = textAreas[left];
        int curveStart = 0, curveEnd = 0;
        if (left != right) {
            area.setLeft(textAreas[left].right() + 1);
            area.setRight(textAreas[right].left() - 1);
            area.setTop(qMax(textAreas[left].top(), textAreas[right].top()));
            area.setBottom(qMin(textAreas[left].bottom(), textAreas[right].bottom()));
            curveStart = origin(m_editors[left]).x() + m_editors[left]->width() - area.x();
            curveEnd = origin(m_editors[right]).x() - area.x();
        }
        const auto y = [&](int pane, int line) {
            const auto visual = m_editors[pane]->visibleFromDocLine(line);
            return textAreas[pane].top() - area.top() + (visual - first[pane]) * height[pane];
        };
        QVariantList bands{};
        for (const auto &block : m_blocks) {
            const auto &from = block.ranges[left], &to = block.ranges[right];
            if (!from || !to) continue;
            const auto topLeft = y(left, from->start), topRight = y(right, to->start);
            const auto bottomLeft = y(left, from->start + from->count), bottomRight = y(right, to->start + to->count);
            if (qMax(bottomLeft, bottomRight) < 0 || qMin(topLeft, topRight) > area.height()) continue;
            bands.append(QVariantHash{{"topLeft", topLeft}, {"topRight", topRight}, {"bottomLeft", bottomLeft},
                                      {"bottomRight", bottomRight}, {"color", block.color}});
        }
        regions.append(QVariantHash{{"area", area}, {"curveStart", curveStart}, {"curveEnd", curveEnd}, {"bands", bands}});
    }
    return regions;
}

bool DiffOverlay::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::Resize || event->type() == QEvent::Move || event->type() == QEvent::Show) m_refresh.start();
    return QQuickWidget::eventFilter(watched, event);
}
