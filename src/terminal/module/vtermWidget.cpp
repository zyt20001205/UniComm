#include "terminal/module/vtermWidget.h"

#include <QClipboard>

#include "globals.h"
#include "core/globalManager.h"
#include "util/uniCast.h"

VtermWidget::VtermWidget(const int rows, const int cols, QObject *parent)
    : QObject(parent),
      m_rows(rows),
      m_cols(cols),
      m_vterm(vterm_new(m_rows, m_cols)),
      m_state(vterm_obtain_state(m_vterm)),
      m_screen(vterm_obtain_screen(m_vterm)),
      m_selectionBuffer(1024 * 1024, '\0') {
    vterm_set_utf8(m_vterm, 1);
    vterm_output_set_callback(m_vterm, [](const char *data, const size_t size, void *user) {
        emit static_cast<VtermWidget *>(user)->outputWrite(QByteArray(data, static_cast<qsizetype>(size)));
    }, this);
    vterm_state_set_bold_highbright(m_state, 1);

    m_callbacks.damage = [](const VTermRect rect, void *user) -> int {
        return static_cast<VtermWidget *>(user)->screenDamage(rect);
    };
    m_callbacks.movecursor = [](const VTermPos pos, const VTermPos oldPos, const int visible, void *user) -> int {
        return static_cast<VtermWidget *>(user)->cursorMove(pos, oldPos, visible);
    };
    m_callbacks.settermprop = [](const VTermProp prop, VTermValue *value, void *user) -> int {
        return static_cast<VtermWidget *>(user)->termPropSet(prop, value);
    };
    m_callbacks.bell = [](void *user) -> int {
        return static_cast<VtermWidget *>(user)->bell();
    };
    m_callbacks.sb_pushline = [](const int cols, const VTermScreenCell *cells, void *user) -> int {
        return static_cast<VtermWidget *>(user)->linePush(cols, cells);
    };
    m_callbacks.sb_clear = [](void *user) -> int {
        return static_cast<VtermWidget *>(user)->clear();
    };
    m_selectionCallbacks.set = [](const VTermSelectionMask mask, const VTermStringFragment frag, void *user) -> int {
        return static_cast<VtermWidget *>(user)->selectionSet(mask, frag);
    };
    m_selectionCallbacks.query = [](const VTermSelectionMask mask, void *user) -> int {
        return static_cast<VtermWidget *>(user)->selectionQuery(mask);
    };

    vterm_screen_set_callbacks(m_screen, &m_callbacks, this);
    vterm_state_set_selection_callbacks(m_state, &m_selectionCallbacks, this, m_selectionBuffer.data(), static_cast<size_t>(m_selectionBuffer.size()));
    vterm_screen_enable_altscreen(m_screen, 1);

    const auto defaultForeground = uni_cast<VTermColor>(QColor(g_globalManager->foreGet()));
    const auto defaultBackground = uni_cast<VTermColor>(QColor(g_globalManager->backGet()));
    vterm_screen_set_default_colors(m_screen, &defaultForeground, &defaultBackground);

    vterm_screen_set_damage_merge(m_screen, VTERM_DAMAGE_SCROLL);
    vterm_screen_reset(m_screen, 1);
}

VtermWidget::~VtermWidget() {
    if (m_vterm) vterm_free(m_vterm);
}

void VtermWidget::resize(const int rows, const int cols) {
    m_selection = {};
    m_rows = rows;
    m_cols = cols;
    m_pendingDamage = {};
    vterm_set_size(m_vterm, m_rows, m_cols);
    vterm_screen_flush_damage(m_screen);
    renderScreen();
    emit scrollChanged();
}

void VtermWidget::reset(const bool hard) {
    m_selection = {};
    m_pendingDamage = {};
    vterm_screen_reset(m_screen, hard ? 1 : 0);
    vterm_screen_flush_damage(m_screen);
    m_scrollOffset = 0;
    renderScreen();
    emit scrollChanged();
}

void VtermWidget::inputWrite(const QByteArray &bytes) {
    if (bytes.isEmpty()) return;
    const int scrollbackSize = m_scrollback.size();
    const int scrollOffset = m_scrollOffset;
    const bool altScreen = m_altScreen;
    m_pendingDamage = {};
    vterm_input_write(m_vterm, bytes.constData(), static_cast<size_t>(bytes.size()));
    vterm_screen_flush_damage(m_screen);
    m_scrollOffset = qBound(0, m_scrollOffset, m_scrollback.size());

    const QRect damage = m_pendingDamage;
    m_pendingDamage = {};
    if (!damage.isEmpty()) renderDamage(damage);
    if (scrollbackSize != m_scrollback.size() || scrollOffset != m_scrollOffset || altScreen != m_altScreen) emit scrollChanged();
}

void VtermWidget::keyPressed(const int key, const int modifiers, const QString &text) {
    const auto &vtermModifier = uni_cast<VTermModifier>(modifiers);
    const auto &vtermKey = uni_cast<VTermKey>(key);
    if (vtermKey != VTERM_KEY_NONE) vterm_keyboard_key(m_vterm, vtermKey, vtermModifier);
    else {
        for (const auto ch: text.toUcs4()) {
            auto characterModifier = vtermModifier;
            if ((ch <= 0x1f || ch == 0x7f) && characterModifier & VTERM_MOD_CTRL) characterModifier = static_cast<VTermModifier>(characterModifier & ~VTERM_MOD_CTRL);
            vterm_keyboard_unichar(m_vterm, ch, characterModifier);
        }
    }
}

void VtermWidget::mousePressed(const int row, const int col, const int button, const int modifiers) {
    const auto &vtermModifier = uni_cast<VTermModifier>(modifiers);
    vterm_mouse_move(m_vterm, row, col, vtermModifier);
    const auto &vtermButton = uni_cast<VTermButton>(button);
    if (vtermButton > 0) vterm_mouse_button(m_vterm, vtermButton, true, vtermModifier);
}

void VtermWidget::mouseReleased(const int row, const int col, const int button, const int modifiers) {
    const auto &vtermModifier = uni_cast<VTermModifier>(modifiers);
    vterm_mouse_move(m_vterm, row, col, vtermModifier);
    const auto &vtermButton = uni_cast<VTermButton>(button);
    if (vtermButton > 0) vterm_mouse_button(m_vterm, vtermButton, false, vtermModifier);
}

void VtermWidget::mouseMoved(const int row, const int col, const int button, const int modifiers) {
    const auto &vtermModifier = uni_cast<VTermModifier>(modifiers);
    vterm_mouse_move(m_vterm, row, col, vtermModifier);
}

void VtermWidget::mouseWheeled(const int row, const int col, const int lines, const int modifiers) {
    const auto &vtermModifier = uni_cast<VTermModifier>(modifiers);
    const int button = lines > 0 ? 4 : 5;
    const int steps = qMax(1, qAbs(lines) / 3);
    vterm_mouse_move(m_vterm, row, col, vtermModifier);
    for (int step = 0; step < steps; ++step) {
        vterm_mouse_button(m_vterm, button, true, vtermModifier);
    }
}

void VtermWidget::mouseScrolled(const int lines) {
    if (lines == 0) return;
    if (m_altScreen) {
        const int key = lines > 0 ? Qt::Key_Up : Qt::Key_Down;
        const int steps = qMax(1, qAbs(lines) / 3);
        for (int step = 0; step < steps; ++step) {
            keyPressed(key, Qt::NoModifier, QString{});
        }
        return;
    }
    const int scrollOffset = qBound(0, m_scrollOffset + lines, m_scrollback.size());
    if (m_scrollOffset == scrollOffset) return;
    m_scrollOffset = scrollOffset;
    renderScreen();
    emit scrollChanged();
}

void VtermWidget::selectionStart(const int row, const int col) {
    if (row < 0 || row >= m_rows || col < 0 || col > m_cols) return;

    const qint64 firstRow = m_altScreen ? 0 : m_scrollbackFirstRow + m_scrollback.size() - m_scrollOffset;
    m_selection.anchorRow = firstRow + row;
    m_selection.anchorCol = col;
    m_selection.extentRow = m_selection.anchorRow;
    m_selection.extentCol = col;
}

void VtermWidget::selectionUpdate(const int row, const int col) {
    if (m_selection.anchorRow < 0) return;

    const qint64 firstRow = m_altScreen ? 0 : m_scrollbackFirstRow + m_scrollback.size() - m_scrollOffset;
    m_selection.extentRow = firstRow + qBound(0, row, m_rows - 1);
    m_selection.extentCol = qBound(0, col, m_cols);
}

void VtermWidget::linkGet(const int uri, QUrl &url) const {
    const char *value = vterm_screen_get_uri(m_screen, uri);
    url = value ? QUrl(QString::fromUtf8(value)) : QUrl{};
}

qreal VtermWidget::scrollPositionGet() const {
    if (m_altScreen) return 0;
    const int totalRows = m_scrollback.size() + m_rows;
    if (totalRows < 1) return 0;
    return static_cast<qreal>(m_scrollback.size() - m_scrollOffset) / totalRows;
}

void VtermWidget::scrollPositionSet(const qreal position) {
    if (m_altScreen) return;
    const int totalRows = m_scrollback.size() + m_rows;
    if (totalRows < 1) return;
    const int scrollOffset = qBound(
        0,
        m_scrollback.size() - qRound(qBound<qreal>(0, position, 1) * totalRows),
        m_scrollback.size()
    );
    if (m_scrollOffset == scrollOffset) return;
    m_scrollOffset = scrollOffset;
    renderScreen();
    emit scrollChanged();
}

qreal VtermWidget::scrollSizeGet() const {
    if (m_altScreen) return 1;
    const int totalRows = m_scrollback.size() + m_rows;
    if (totalRows < 1) return 1;
    return qMin<qreal>(1, static_cast<qreal>(m_rows) / totalRows);
}

// private
void VtermWidget::renderScreen() {
    QList<TerminalCell> screen{};
    screen.reserve(m_rows * m_cols);
    for (int row = 0; row < m_rows; ++row) {
        for (int col = 0; col < m_cols; ++col) {
            VTermScreenCell _cell{};
            vterm_screen_get_cell(m_screen, VTermPos{row, col}, &_cell);
            screen.append(uni_cast<TerminalCell>(m_screen, _cell));
        }
    }

    m_scrollOffset = qBound(0, m_scrollOffset, m_scrollback.size());
    const int totalRows = m_scrollback.size() + m_rows;
    const int firstRow = qMax(0, totalRows - m_rows - m_scrollOffset);

    QList<TerminalCell> visible{};
    visible.reserve(m_rows * m_cols);
    for (int row = 0; row < m_rows; ++row) {
        const int sourceRow = firstRow + row;
        if (sourceRow < m_scrollback.size()) {
            const auto &line = m_scrollback[sourceRow];
            for (int col = 0; col < m_cols; ++col) {
                if (col < line.size()) visible.append(line[col]);
                else visible.append(TerminalCell{});
            }
        } else {
            const int screenRow = sourceRow - m_scrollback.size();
            for (int col = 0; col < m_cols; ++col) {
                visible.append(screen[screenRow * m_cols + col]);
            }
        }
    }

    m_pendingDamage = {};
    emit setScreen(m_rows, m_cols, visible, m_scrollOffset == 0);
}

void VtermWidget::renderDamage(const QRect &rect) {
    if (m_scrollOffset != 0) {
        renderScreen();
        return;
    }

    const QRect damage = rect.intersected(QRect(0, 0, m_cols, m_rows));
    if (damage.isEmpty()) return;

    QList<TerminalCell> cells{};
    cells.reserve(damage.width() * damage.height());
    for (int row = damage.top(); row <= damage.bottom(); ++row) {
        for (int col = damage.left(); col <= damage.right(); ++col) {
            VTermScreenCell cell{};
            vterm_screen_get_cell(m_screen, VTermPos{row, col}, &cell);
            cells.append(uni_cast<TerminalCell>(m_screen, cell));
        }
    }

    emit setScreenDamage(damage, cells);
}

int VtermWidget::screenDamage(const VTermRect rect) {
    const QRect damage(
        rect.start_col,
        rect.start_row,
        rect.end_col - rect.start_col,
        rect.end_row - rect.start_row
    );
    if (damage.isEmpty()) return 1;

    m_pendingDamage = m_pendingDamage.isEmpty() ? damage : m_pendingDamage.united(damage);
    return 1;
}

int VtermWidget::cursorMove(const VTermPos pos, const VTermPos oldPos, const int visible) {
    Q_UNUSED(visible);
    const QPoint oldPosition{oldPos.row, oldPos.col};
    m_cursorPosition = {pos.row, pos.col};
    emit setCursorPosition(m_cursorPosition, oldPosition);
    return 1;
}

int VtermWidget::termPropSet(const VTermProp prop, const VTermValue *value) {
    switch (static_cast<int>(prop)) {
        case VTERM_PROP_CURSORVISIBLE:
            m_cursorVisible = value->boolean;
            emit setCursorVisible(value->boolean);
            break;
        case VTERM_PROP_CURSORBLINK: emit setCursorBlink(value->boolean);
            break;
        case VTERM_PROP_ALTSCREEN:
            m_selection = {};
            m_altScreen = value->boolean;
            m_scrollOffset = 0;
            break;
        case VTERM_PROP_TITLE: {
            const auto frag = value->string;
            emit setTitle(QString::fromUtf8(frag.str, static_cast<qsizetype>(frag.len)));
        }
        break;
        case VTERM_PROP_ICONNAME: {
            // ignored
            // const auto frag = value->string;
            // const auto &iconname = QString::fromUtf8(frag.str, static_cast<qsizetype>(frag.len));
            // qDebug() << iconname;
        }
        break;
        case VTERM_PROP_CURSORSHAPE: emit setCursorShape(value->number);
            break;
        case VTERM_PROP_MOUSE: emit setCursorMode(value->number);
            break;
        default: break;
    }
    return 1;
}

int VtermWidget::bell() {
    QApplication::beep();
    return 1;
}

int VtermWidget::linePush(const int cols, const VTermScreenCell *cells) {
    QList<TerminalCell> row{};
    row.reserve(cols);
    for (int col = 0; col < cols; ++col) {
        const auto &cell = uni_cast<TerminalCell>(m_screen, cells[col]);
        row.append(cell);
    }
    if (m_scrollOffset > 0) ++m_scrollOffset;
    m_scrollback.append(row);

    constexpr int maxScrollbackLines = 10000;
    if (m_scrollback.size() > maxScrollbackLines) {
        m_scrollback.removeFirst();
        ++m_scrollbackFirstRow;
        if (m_selection.anchorRow < m_scrollbackFirstRow || m_selection.extentRow < m_scrollbackFirstRow) m_selection = {};
    }
    m_scrollOffset = qMin(m_scrollOffset, m_scrollback.size());

    return 1;
}

int VtermWidget::clear() {
    m_selection = {};
    m_scrollbackFirstRow += m_scrollback.size();
    m_scrollback.clear();
    if (m_scrollOffset == 0) return 1;

    m_scrollOffset = 0;
    const QRect screenRect(0, 0, m_cols, m_rows);
    m_pendingDamage = m_pendingDamage.isEmpty() ? screenRect : m_pendingDamage.united(screenRect);
    return 1;
}

int VtermWidget::selectionSet(const VTermSelectionMask mask, const VTermStringFragment frag) {
    if (frag.initial) m_pendingSelection.clear();
    m_pendingSelection += QString::fromUtf8(frag.str, static_cast<qsizetype>(frag.len));

    if (!frag.final) return 1;

    if (!m_pendingSelection.isEmpty()) {
        auto *clipboard = QApplication::clipboard();
        if (static_cast<int>(mask) & VTERM_SELECTION_CLIPBOARD) {
            clipboard->setText(m_pendingSelection, QClipboard::Clipboard);
        }
        if (static_cast<int>(mask) & VTERM_SELECTION_PRIMARY && clipboard->supportsSelection()) {
            clipboard->setText(m_pendingSelection, QClipboard::Selection);
        }
    }
    m_pendingSelection.clear();
    return 1;
}

int VtermWidget::selectionQuery(const VTermSelectionMask mask) {
    qDebug() << "OSC 52 selection query:" << mask;
    return 1;
}
