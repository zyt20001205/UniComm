// #define UNICOMM_TERMINAL_DAMAGE_DEBUG

#include "terminal/module/terminalWidget.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QInputMethodEvent>
#include <QPainter>
#include <QTimer>

#include "globals.h"

TerminalWidget::TerminalWidget(QQuickItem *parent)
    : QQuickPaintedItem(parent),
      m_blinkTimer(new QTimer(this)) {
    setAntialiasing(false);
    setOpaquePainting(false);
    setFlag(ItemHasContents, true);
    setFlag(ItemAcceptsInputMethod, true);
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::AllButtons);
    metricsUpdate();

    m_blinkTimer->setInterval(500);
    connect(m_blinkTimer, &QTimer::timeout, this, [this] {
        m_blinkPhase = !m_blinkPhase;
        updateRegion(cursorRect());
    });
    m_blinkTimer->start();
}

void TerminalWidget::propertySet(const QVariantHash &objects) {
    m_contextMenu = qvariant_cast<QObject *>(objects["terminalModuleContextMenu"]);
    m_toolTip = qvariant_cast<QObject *>(objects["mainWindowToolTip"]);
}

void TerminalWidget::paste() {
    const QString text = QGuiApplication::clipboard()->text();
    if (!text.isEmpty()) emit keyPressed(0, Qt::NoModifier, text);
}

void TerminalWidget::paint(QPainter *painter) {
    if (m_rows < 1 || m_cols < 1 || m_cells.size() < m_rows * m_cols) return;

    QRectF paintRect = painter->hasClipping() ? painter->clipBoundingRect() : boundingRect();
    paintRect = paintRect.intersected(QRectF(0, 0, m_cols * m_cellWidth, m_rows * m_cellHeight));
    if (paintRect.isEmpty()) return;

    const int firstRow = qMax(0, qFloor(paintRect.top() / m_cellHeight));
    const int endRow = qMin(m_rows, qCeil(paintRect.bottom() / m_cellHeight));
    const int firstCol = qMax(0, qFloor(paintRect.left() / m_cellWidth) - 1);
    const int endCol = qMin(m_cols, qCeil(paintRect.right() / m_cellWidth) + 1);

    painter->setFont(m_font);
    QFont currentFont = m_font;

    for (int row = firstRow; row < endRow; ++row) {
        const qreal y = row * m_cellHeight + m_ascent;
        int col = firstCol;
        while (col < endCol) {
            // get cell
            const auto &cell = m_cells[row * m_cols + col];
            if (cell.width == 0) {
                ++col;
                continue;
            }
            // draw background
            const QRectF rect(col * m_cellWidth, row * m_cellHeight, cell.width * m_cellWidth, m_cellHeight);
            painter->fillRect(rect, cell.background);
            // draw cell
            if (!cell.text.isEmpty()) {
                QFont cellFont = m_font;
                cellFont.setBold(cell.bold);
                cellFont.setUnderline(cell.underline || cell.uri > 0);
                cellFont.setItalic(cell.italic);
                cellFont.setStrikeOut(cell.strike);
                if (cellFont != currentFont) {
                    currentFont = cellFont;
                    painter->setFont(currentFont);
                }
                painter->setPen(cell.foreground);
                painter->drawText(QPointF(rect.left(), y), cell.text);
                if (cell.overline) {
                    const qreal thickness = qMax<qreal>(1, m_cellHeight / 18);
                    painter->fillRect(QRectF(rect.left(), rect.top(), rect.width(), thickness), cell.foreground);
                }
            }
            // draw cursor
            if (m_atBottom && m_visible && m_blinkPhase && row == m_position.x() && col == m_position.y()) {
                switch (m_shape) {
                    case VTERM_PROP_CURSORSHAPE_BLOCK: {
                        painter->fillRect(rect, cell.foreground);
                        painter->setPen(cell.background);
                        painter->drawText(QPointF(rect.left(), y), cell.text);
                    }
                    break;
                    case VTERM_PROP_CURSORSHAPE_UNDERLINE: {
                        const qreal thickness = qMax<qreal>(1, m_cellHeight / 10);
                        painter->fillRect(QRectF(rect.left(), rect.bottom() - thickness, rect.width(), thickness), cell.foreground);
                    }
                    break;
                    case VTERM_PROP_CURSORSHAPE_BAR_LEFT: {
                        const qreal thickness = qMax<qreal>(1, m_cellWidth / 8);
                        painter->fillRect(QRectF(rect.left(), rect.top(), thickness, rect.height()), cell.foreground);
                    }
                    break;
                    default: break;
                }
            }
            col += cell.width;
        }
    }
}

void TerminalWidget::fontSet(const QFont &font) {
    m_font = font;
    metricsUpdate();
    updateRegion();
}

void TerminalWidget::screenSet(const int rows, const int cols, const QList<TerminalCell> &cells, const bool atBottom) {
    m_rows = rows;
    m_cols = cols;
    m_cells = cells;
    m_atBottom = atBottom;
    updateRegion();
}

void TerminalWidget::screenDamageSet(const QRect &rect, const QList<TerminalCell> &cells) {
    const int expectedCells = rect.width() * rect.height();
    if (rect.isEmpty() || cells.size() != expectedCells || m_cells.size() != m_rows * m_cols || rect.left() < 0 || rect.top() < 0 || rect.right() >= m_cols || rect.bottom() >= m_rows) return;

    int source = 0;
    for (int row = rect.top(); row <= rect.bottom(); ++row) {
        for (int col = rect.left(); col <= rect.right(); ++col) {
            m_cells[row * m_cols + col] = cells[source++];
        }
    }

    const QRect pixelRect = QRectF(
        rect.left() * m_cellWidth,
        rect.top() * m_cellHeight,
        rect.width() * m_cellWidth,
        rect.height() * m_cellHeight
    ).toAlignedRect().adjusted(-1, 0, 1, 0).intersected(boundingRect().toAlignedRect());

    updateRegion(pixelRect);
}

void TerminalWidget::cursorPositionSet(const QPoint &position, const QPoint &oldPosition) {
    if (m_position == position) return;
    m_position = position;
    if (!(m_atBottom && m_visible && m_blinkPhase)) return;
    updateRegion(cursorRect(oldPosition));
    updateRegion(cursorRect(position));
}

void TerminalWidget::cursorVisibleSet(const bool visible) {
    if (m_visible == visible) return;
    m_visible = visible;
    if (visible) {
        if (m_blink) {
            m_blinkPhase = false;
            m_blinkTimer->start();
        } else {
            m_blinkPhase = true;
        }
    } else {
        m_blinkTimer->stop();
        m_blinkPhase = true;
    }
    updateRegion(cursorRect());
}

void TerminalWidget::cursorBlinkSet(const bool blink) {
    if (m_blink == blink) return;
    m_blink = blink;
    if (!m_visible) return;
    if (blink) {
        m_blinkPhase = false;
        m_blinkTimer->start();
    } else {
        m_blinkTimer->stop();
        m_blinkPhase = true;
    }
    updateRegion(cursorRect());
}

void TerminalWidget::cursorShapeSet(const int shape) {
    if (m_shape == shape) return;
    m_shape = shape;
    updateRegion(cursorRect());
}

void TerminalWidget::cursorModeSet(const int mode) {
    if (m_mode == mode) return;
    m_mode = mode;
    m_selecting = false;
}

QVariant TerminalWidget::inputMethodQuery(const Qt::InputMethodQuery query) const {
    if (query == Qt::ImCursorRectangle) return cursorRect();
    return QQuickPaintedItem::inputMethodQuery(query);
}

// protected
void TerminalWidget::keyPressEvent(QKeyEvent *event) {
    emit keyPressed(event->key(), event->modifiers(), event->text());
    event->accept();
}

void TerminalWidget::inputMethodEvent(QInputMethodEvent *event) {
    const QString commit = event->commitString();
    if (!commit.isEmpty()) emit keyPressed(0, Qt::NoModifier, commit);
    event->accept();
}

void TerminalWidget::mousePressEvent(QMouseEvent *event) {
    forceActiveFocus();
    m_selecting = false;
    if (m_mode >= VTERM_PROP_MOUSE_CLICK) {
        emit mousePressed(
            event->position().y() / m_cellHeight,
            event->position().x() / m_cellWidth,
            event->button(),
            event->modifiers()
        );
    } else {
        if (event->button() == Qt::LeftButton && !(event->modifiers() & Qt::ControlModifier)) {
            const int row = qFloor(event->position().y() / m_cellHeight);
            const int col = qRound(event->position().x() / m_cellWidth);
            if (m_rows > 0 && m_cols > 0 && row >= 0 && row < m_rows) {
                m_selecting = true;
                emit startSelection(row, qBound(0, col, m_cols));
            }
        }
        else if (event->button() == Qt::MiddleButton) paste();
        else if (event->button() == Qt::RightButton) {
            const int row = qFloor(event->position().y() / m_cellHeight);
            const int col = qFloor(event->position().x() / m_cellWidth);
            const int index = row * m_cols + col;
            QUrl url;
            if (row >= 0 && row < m_rows && col >= 0 && col < m_cols && index >= 0 && index < m_cells.size() && m_cells[index].uri > 0) {
                emit getLink(m_cells[index].uri, url);
            }
            m_contextMenu->setProperty("menuSession", QVariantHash({
                {"terminalWidget", QVariant::fromValue(this)},
                {"copiable", false},
                {"pastable", !QGuiApplication::clipboard()->text().isEmpty()},
                {"url", url}
            }));
            m_toolTip->setProperty("text", QString());
            QMetaObject::invokeMethod(m_contextMenu, "popup");
        }
    }
    event->accept();
}

void TerminalWidget::mouseReleaseEvent(QMouseEvent *event) {
    if (m_mode == VTERM_PROP_MOUSE_NONE) {
        const int row = qFloor(event->position().y() / m_cellHeight);
        const int col = qFloor(event->position().x() / m_cellWidth);
        const int index = row * m_cols + col;
        const bool link = row >= 0 && row < m_rows && col >= 0 && col < m_cols && index >= 0 && index < m_cells.size() && m_cells[index].uri > 0;
        if (event->button() == Qt::LeftButton && m_selecting) {
            emit updateSelection(row, qRound(event->position().x() / m_cellWidth));
        }
        else if (link && event->button() == Qt::LeftButton && event->modifiers() & Qt::ControlModifier) {
            QUrl url;
            emit getLink(m_cells[index].uri, url);
            if (!url.isEmpty()) QDesktopServices::openUrl(url);
        }
    }
    else {
        emit mouseReleased(
            event->position().y() / m_cellHeight,
            event->position().x() / m_cellWidth,
            event->button(),
            event->modifiers()
        );
    }
    if (event->button() == Qt::LeftButton) m_selecting = false;
    event->accept();
}

void TerminalWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_mode == VTERM_PROP_MOUSE_MOVE || (m_mode == VTERM_PROP_MOUSE_DRAG && event->buttons() != Qt::NoButton)) {
        setCursor(Qt::IBeamCursor);
        emit mouseMoved(
            event->position().y() / m_cellHeight,
            event->position().x() / m_cellWidth,
            event->button(),
            event->modifiers()
        );
    }
    else if (m_mode == VTERM_PROP_MOUSE_NONE && m_selecting && (event->buttons() & Qt::LeftButton)) {
        emit updateSelection(qFloor(event->position().y() / m_cellHeight), qRound(event->position().x() / m_cellWidth));
    }
    event->accept();
}

void TerminalWidget::hoverMoveEvent(QHoverEvent *event) {
    const int row = qFloor(event->position().y() / m_cellHeight);
    const int col = qFloor(event->position().x() / m_cellWidth);
    const int index = row * m_cols + col;
    const bool link = m_mode == VTERM_PROP_MOUSE_NONE && row >= 0 && row < m_rows && col >= 0 && col < m_cols && index >= 0 && index < m_cells.size() && m_cells[index].uri > 0;
    const bool control = event->modifiers() & Qt::ControlModifier;
    setCursor(link && control ? Qt::PointingHandCursor : Qt::IBeamCursor);
    if (link) {
        m_toolTip->setProperty("position", QCursor::pos());
        if (control) {
            QUrl url;
            emit getLink(m_cells[index].uri, url);
            m_toolTip->setProperty("text", url.toString());
        } else {
            m_toolTip->setProperty("text", tr("Ctrl + Click"));
        }
    } else {
        m_toolTip->setProperty("text", QString());
    }
    QQuickPaintedItem::hoverMoveEvent(event);
}

void TerminalWidget::hoverLeaveEvent(QHoverEvent *event) {
    setCursor(Qt::IBeamCursor);
    m_toolTip->setProperty("text", QString());
    QQuickPaintedItem::hoverLeaveEvent(event);
}

void TerminalWidget::wheelEvent(QWheelEvent *event) {
    const int degrees = event->angleDelta().y() / 120;
    const int pixels = event->pixelDelta().y() == 0 ? 0 : event->pixelDelta().y() / static_cast<int>(m_cellHeight);
    const int lines = pixels != 0 ? pixels : degrees * 3;
    if (lines == 0) {
        event->ignore();
        return;
    }

    if (m_mode >= VTERM_PROP_MOUSE_CLICK) {
        emit mouseWheeled(
            event->position().y() / m_cellHeight,
            event->position().x() / m_cellWidth,
            lines,
            event->modifiers()
        );
    } else {
        emit mouseScrolled(lines);
    }
    event->accept();
}

void TerminalWidget::geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) {
    QQuickPaintedItem::geometryChange(newGeometry, oldGeometry);
    metricsUpdate();
}

// private
void TerminalWidget::metricsUpdate() {
    QFontMetricsF metrics(m_font);
    m_cellWidth = qMax<qreal>(1, metrics.horizontalAdvance('M'));
    m_cellHeight = qMax<qreal>(1, metrics.lineSpacing());
    m_ascent = qMax<qreal>(1, metrics.ascent());

    const int rows = static_cast<int>(height() / m_cellHeight);
    const int cols = static_cast<int>(width() / m_cellWidth);
    if (rows < 1 || cols < 1) return;
    if (rows == m_requestedRows && cols == m_requestedCols) return;

    m_requestedRows = rows;
    m_requestedCols = cols;
    emit resize(rows, cols);
}

void TerminalWidget::updateRegion() {
#ifdef UNICOMM_TERMINAL_DAMAGE_DEBUG
    emit debugDamage(boundingRect());
#endif
    update();
}

void TerminalWidget::updateRegion(const QRect &rect) {
#ifdef UNICOMM_TERMINAL_DAMAGE_DEBUG
    if (!rect.isEmpty()) emit debugDamage(rect);
#endif
    update(rect);
}

QRect TerminalWidget::cursorRect() const {
    return cursorRect(m_position);
}

QRect TerminalWidget::cursorRect(const QPoint &position) const {
    if (m_rows < 1 || m_cols < 1) return {};
    const int row = qBound(0, position.x(), m_rows - 1);
    const int col = qBound(0, position.y(), m_cols - 1);
    return QRect(
        qFloor(col * m_cellWidth),
        qFloor(row * m_cellHeight),
        qCeil(m_cellWidth),
        qCeil(m_cellHeight)
    );
}
