#include "document/page/conflictPage.h"

#include <algorithm>
#include <iterator>
#include <QGridLayout>
#include <QLabel>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QTextStream>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/diffOverlay.h"
#include "document/module/editorWidget.h"
#include "document/module/resolveWidget.h"
#include "document/module/scintillaWidget.h"
#include "service/git/gitModule.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"

// public
ConflictPage::ConflictPage(const QJsonObject &documentConfig, const QUrl &documentUrl)
    : DocumentPage(documentUrl),
      m_widget(new QWidget(this)),
      m_baseEditor(new EditorWidget(documentConfig, {}, m_widget)),
      m_resolveWidget(new ResolveWidget(m_widget)) {
    setIcon(QIcon(g_globalManager->themeGet()["mode"].toInt() == Theme::Light ? ":/icon/documentErrorLight.svg" : ":/icon/documentErrorDark.svg"));
    auto *layout = new QGridLayout(m_widget); // NOLINT
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(32);
    auto *ours = new EditorWidget(documentConfig, {}, m_widget);
    auto *theirs = new EditorWidget(documentConfig, {}, m_widget);
    m_editors = {ours->handler(), handler(), theirs->handler()};
    for (int i = 0; i < 3; ++i)
        connect(m_editors[i]->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, i] { scrollSync(i); });
    for (auto *editor : {ours, theirs}) {
        editor->propertySet();
        editor->handler()->readonlySet(true);
    }
    handler()->readonlySet(true);
    auto *oursLabel = new QLabel(tr("Ours"), m_widget);
    auto *baseLabel = new QLabel(tr("Base"), m_widget);
    auto *theirsLabel = new QLabel(tr("Theirs"), m_widget);
    layout->addWidget(oursLabel, 0, 0);
    layout->addWidget(baseLabel, 0, 1);
    layout->addWidget(theirsLabel, 0, 2);
    layout->addWidget(ours, 1, 0);
    layout->addWidget(m_baseEditor, 1, 1);
    layout->addWidget(m_resolveWidget, 1, 1, Qt::AlignBottom | Qt::AlignHCenter);
    m_resolveWidget->hide();
    layout->addWidget(theirs, 1, 2);
    m_diffOverlay = new DiffOverlay({ours->handler(), handler(), theirs->handler()}, m_widget);
    layout->addWidget(m_diffOverlay, 1, 0, 1, 3);
    m_diffOverlay->raise();
    for (int column = 0; column < 3; ++column) layout->setColumnStretch(column, 1);
    setWidget(m_widget);
    connect(m_baseEditor, &EditorWidget::appendLog, this, &ConflictPage::appendLog);
    connect(m_baseEditor, &EditorWidget::changeSavepoint, this, &ConflictPage::savepointChange);
    connect(m_baseEditor, &EditorWidget::changeSelection, this, &ConflictPage::changeSelection);
    connect(m_baseEditor, &EditorWidget::gotoDocument, this, &DocumentPage::gotoDocument);
    connect(m_baseEditor, &EditorWidget::showPermissionDialog, this, &DocumentPage::showPermissionDialog);
    connect(m_resolveWidget, &ResolveWidget::finishResolve, this, &ConflictPage::resolveFinish);

    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    git->conflictGet(documentUrl).then(this, [this, git, ours, theirs, oursLabel, baseLabel, theirsLabel](const GitConflictResult &result) {
        if (!result.success) {
            emit appendLog(LogType::Error, tr("Git command failed"), result.text());
            return;
        }
        if (!result.base && !result.ours && !result.theirs) return;
        const auto display = [](const ScintillaWidget *editor, const std::optional<QByteArray> &blob) -> QByteArray {
            if (!blob) return {};
            auto text = QTextStream(*blob).readAll();
            text.replace("\r\n", "\n").replace('\r', '\n');
            editor->readonlySet(false);
            editor->textAppend(text);
            editor->savepointSet();
            editor->readonlySet(true);
            return text.toUtf8();
        };
        const auto base = display(handler(), result.base);
        if (!result.base) baseLabel->setText(tr("Base (absent)"));
        if (!result.ours) oursLabel->setText(tr("Ours (deleted)"));
        if (!result.theirs) theirsLabel->setText(tr("Theirs (deleted)"));
        std::array diffs{git->diffGet(base, display(ours->handler(), result.ours)),
                         git->diffGet(base, display(theirs->handler(), result.theirs))};
        QtFuture::whenAll(diffs.begin(), diffs.end()).then(this, [this, ours, theirs](const QList<QFuture<GitDiffResult>> &finished) {
            diffSet(ours->handler(), theirs->handler(), finished[0].result(), finished[1].result());
        });
    });
}

void ConflictPage::propertySet() const {
    m_baseEditor->propertySet();
    m_resolveWidget->propertySet();
}

QString ConflictPage::documentSave() {
    return m_baseEditor->documentSave();
}

// protected
bool ConflictPage::documentModified() const {
    return handler()->modifyGet();
}

// private
void ConflictPage::diffSet(ScintillaWidget *ours, ScintillaWidget *theirs, const GitDiffResult &oursDiff, const GitDiffResult &theirsDiff) {
    struct Change {
        GitDiffResult::Hunk hunk;
        int side;
    };
    QList<Change> changes{};
    std::array<DiffScroll::Anchors, 2> anchors{};
    const std::array diffs{&oursDiff, &theirsDiff};
    for (int side = 0; side < 2; ++side) {
        if (!diffs[side]->success) {
            emit appendLog(LogType::Error, tr("Git command failed"), diffs[side]->text());
            return;
        }
        anchors[side].append({0, 0});
        for (const auto &hunk : diffs[side]->hunks) {
            changes.append({hunk, side});
            anchors[side].append({hunk.leftStart, hunk.rightStart});
            anchors[side].append({hunk.leftStart + hunk.leftCount, hunk.rightStart + hunk.rightCount});
        }
    }
    // At a shared boundary, process insertions before changes to the following line.
    std::ranges::sort(changes, {}, [](const Change &change) { return std::pair{change.hunk.leftStart, change.hunk.leftCount}; });

    const auto text = [](const ScintillaWidget *view, int start, int count) {
        if (!count) return QString{};
        const int end = start + count;
        if (end < view->lineCountGet()) return view->textGet(start, 0, end, 0);
        return view->textGet(start, 0, end - 1, -1);
    };
    QList<DiffOverlay::Block> blocks{};
    std::array<int, 2> offset{};
    for (qsizetype i = 0; i < changes.size();) {
        const int start = changes[i].hunk.leftStart;
        int end = start;
        std::array<int, 2> delta{};
        std::array<bool, 2> changed{};
        // Join overlapping base ranges, including insertions at the same position.
        do {
            const auto &[hunk, side] = changes[i++];
            end = qMax(end, hunk.leftStart + hunk.leftCount);
            delta[side] += hunk.rightCount - hunk.leftCount;
            changed[side] = true;
        } while (i < changes.size() && (changes[i].hunk.leftStart < end ||
                 (end == start && changes[i].hunk.leftStart == start && !changes[i].hunk.leftCount)));

        const int count = end - start;
        const int oursStart = start + offset[0], oursCount = count + delta[0];
        const int theirsStart = start + offset[1], theirsCount = count + delta[1];
        DiffOverlay::Block block{};
        if (changed[0] && changed[1] && (oursCount != theirsCount || text(ours, oursStart, oursCount) != text(theirs, theirsStart, theirsCount))) {
            block.kind = DiffKind::Conflict;
        } else if (!count) {
            block.kind = DiffKind::Added;
        } else if ((changed[0] && !oursCount) || (changed[1] && !theirsCount)) {
            block.kind = DiffKind::Deleted;
        }
        block.ranges.resize(3);
        block.ranges[1] = {start, count};
        if (changed[0]) block.ranges[0] = {oursStart, oursCount};
        if (changed[1]) block.ranges[2] = {theirsStart, theirsCount};
        blocks.append(block);
        offset[0] += delta[0];
        offset[1] += delta[1];
    }
    m_diffOverlay->blocksSet(std::move(blocks));
    m_scrollAnchors = std::move(anchors);
}

void ConflictPage::scrollSync(const int source) {
    if (m_scrolling || m_scrollAnchors[0].isEmpty()) return;
    const QScopedValueRollback guard(m_scrolling, true);
    constexpr int ours = 0, base = 1, theirs = 2;
    if (source != base) DiffScroll::sync(m_editors[source], m_editors[base], m_scrollAnchors[source == ours ? 0 : 1], true);
    if (source != ours) DiffScroll::sync(m_editors[base], m_editors[ours], m_scrollAnchors[0], false);
    if (source != theirs) DiffScroll::sync(m_editors[base], m_editors[theirs], m_scrollAnchors[1], false);
}

void ConflictPage::savepointChange(const bool status) {
    const QString pageName = title();
    if (status) {
        setTitle(pageName + "*");
    } else {
        setTitle(pageName.chopped(1));
    }
}

void ConflictPage::resolveFinish() {
    if (!documentSave().isEmpty()) return;
    g_globalManager->gitModuleGet()->add(m_documentUrl).then(this, [this](bool success) {
        if (success) emit reloadDocument(m_documentUrl.toLocalFile());
    });
}
