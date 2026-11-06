#ifndef UNICOMM_VTERMWIDGET_H
#define UNICOMM_VTERMWIDGET_H

#include "globals.h"

#include <QObject>
#include <QRect>
#include <QUrl>
#include <vterm.h>

extern "C" {
typedef VTerm VTerm;
typedef VTermState VTermState;
typedef VTermScreen VTermScreen;
}

class VtermWidget final : public QObject {
    Q_OBJECT
    Q_PROPERTY(qreal scrollPosition READ scrollPositionGet WRITE scrollPositionSet NOTIFY scrollChanged)
    Q_PROPERTY(qreal scrollSize READ scrollSizeGet NOTIFY scrollChanged)

public:
    explicit VtermWidget(int rows = 24, int cols = 80, QObject *parent = nullptr);

    ~VtermWidget() override;

    void resize(int rows, int cols);

    void reset(bool hard = true);

    void inputWrite(const QByteArray &bytes);

    void keyPressed(int key, int modifiers, const QString &text);

    void mousePressed(int row, int col, int button, int modifiers);

    void mouseReleased(int row, int col, int button, int modifiers);

    void mouseMoved(int row, int col, int button, int modifiers);

    void mouseWheeled(int row, int col, int lines, int modifiers);

    void mouseScrolled(int lines);

    void selectionStart(int row, int col);

    void selectionUpdate(int row, int col);

    void linkGet(int uri, QUrl &url) const;

    [[nodiscard]] qreal scrollPositionGet() const;

    void scrollPositionSet(qreal position);

    [[nodiscard]] qreal scrollSizeGet() const;

signals:
    void outputWrite(const QByteArray &bytes);

    void setScreen(int rows, int cols, const QList<TerminalCell> &cells, bool atBottom);

    void setScreenDamage(const QRect &rect, const QList<TerminalCell> &cells);

    void setSelection(int startRow, int startCol, int endRow, int endCol);

    void setCursorPosition(const QPoint &position, const QPoint &oldPosition);

    void setCursorVisible(bool visible);

    void setCursorBlink(bool blink);

    void setTitle(const QString &title);

    void setCursorShape(int shape);

    void setCursorMode(int mode);

    void scrollChanged();

private:
    void renderScreen();

    void renderDamage(const QRect &rect);

    void renderSelection();

    int screenDamage(VTermRect rect);

    int cursorMove(VTermPos pos, VTermPos oldPos, int visible);

    int termPropSet(VTermProp prop, const VTermValue *value);

    int bell();

    int linePush(int cols, const VTermScreenCell *cells);

    int clear();

    int selectionSet(VTermSelectionMask mask, VTermStringFragment frag);

    int selectionQuery(VTermSelectionMask mask);

    int m_rows{};
    int m_cols{};
    VTerm *m_vterm{};
    VTermState *m_state{};
    VTermScreen *m_screen{};
    VTermScreenCallbacks m_callbacks{};
    VTermSelectionCallbacks m_selectionCallbacks{};
    QPoint m_cursorPosition{};
    bool m_cursorVisible{true};
    bool m_altScreen{false};
    QList<QList<TerminalCell> > m_scrollback{};
    qint64 m_scrollbackFirstRow{};
    int m_scrollOffset{};
    TerminalSelection m_selection{};
    QRect m_pendingDamage{};
    // osc52
    QByteArray m_selectionBuffer{};
    QString m_pendingSelection{};
};

#endif //UNICOMM_VTERMWIDGET_H
