#ifndef UNICOMM_TERMINALWIDGET_H
#define UNICOMM_TERMINALWIDGET_H

#include "globals.h"
#include "terminal/module/vtermWidget.h"

#include <QList>
#include <QQuickPaintedItem>
#include <QRect>
#include <QVariant>

class TerminalWidget final : public QQuickPaintedItem {
    Q_OBJECT

public:
    explicit TerminalWidget(QQuickItem *parent = nullptr);

    void propertySet(const QVariantHash &objects);

    Q_INVOKABLE void paste();

    void paint(QPainter *painter) override;

    void fontSet(const QFont &font);

    void screenSet(int rows, int cols, const QList<TerminalCell> &cells, bool atBottom);

    void screenDamageSet(const QRect &rect, const QList<TerminalCell> &cells);

    void selectionSet(int startRow, int startCol, int endRow, int endCol);

    void cursorPositionSet(const QPoint &position, const QPoint &oldPosition);

    void cursorVisibleSet(bool visible);

    void cursorBlinkSet(bool blink);

    void cursorShapeSet(int shape);

    void cursorModeSet(int mode);

    QVariant inputMethodQuery(Qt::InputMethodQuery query) const override;

signals:
    void resize(int rows, int cols);

    void keyPressed(int key, int modifiers, const QString &text);

    void mousePressed(int row, int col, int button, int modifiers);

    void mouseReleased(int row, int col, int button, int modifiers);

    void mouseMoved(int row, int col, int button, int modifiers);

    void mouseWheeled(int row, int col, int lines, int modifiers);

    void mouseScrolled(int lines);

    void startSelection(int row, int col);

    void updateSelection(int row, int col);

    void getLink(int uri, QUrl &url);

    void debugDamage(const QRectF &rect);

protected:
    void keyPressEvent(QKeyEvent *event) override;

    void inputMethodEvent(QInputMethodEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void hoverMoveEvent(QHoverEvent *event) override;

    void hoverLeaveEvent(QHoverEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

    void geometryChange(const QRectF &newGeometry, const QRectF &oldGeometry) override;

private:
    void metricsUpdate();

    void updateRegion();

    void updateRegion(const QRect &rect);

    [[nodiscard]] QRect selectionRect() const;

    [[nodiscard]] QRect cursorRect() const;

    [[nodiscard]] QRect cursorRect(const QPoint &position) const;

    QObject *m_contextMenu{};
    QObject *m_toolTip{};
    QFont m_font{};
    QList<TerminalCell> m_cells{};
    TerminalSelection m_selection{};
    bool m_selecting{};
    // cursor
    QPoint m_position{};
    bool m_visible{true};
    bool m_blink{true};
    QTimer *m_blinkTimer{};
    bool m_blinkPhase{true};
    int m_shape{VTERM_PROP_CURSORSHAPE_BAR_LEFT};
    int m_mode{VTERM_PROP_MOUSE_NONE};
    bool m_atBottom{true};

    int m_rows{};
    int m_cols{};
    int m_requestedRows{};
    int m_requestedCols{};
    qreal m_cellWidth{1};
    qreal m_cellHeight{1};
    qreal m_ascent{1};
};

#endif //UNICOMM_TERMINALWIDGET_H
