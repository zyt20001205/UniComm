#include "document/page/diffPage.h"

#include <QGridLayout>
#include <QLabel>
#include <QScopedValueRollback>
#include <QScrollBar>
#include <QTextStream>
#include <QUuid>

#include "globals.h"
#include "core/globalManager.h"
#include "document/module/diffOverlay.h"
#include "document/module/editorWidget.h"
#include "mainWindow/toastModule.h"
#include "service/git/gitService.h"
#include "service/serviceModule.h"

DiffPage::DiffPage(const QJsonObject &config, const QString &leftTitle, const QByteArray &leftContent, const QString &rightTitle, const QByteArray &rightContent)
    : DocumentPage(QUrl("diff:" + QUuid::createUuid().toString(QUuid::WithoutBraces))) {
    setTitle(leftTitle + " ↔ " + rightTitle);
    setIcon(QIcon(g_globalManager->themeGet()["mode"].toInt() == Theme::Light ? ":/icon/documentChevronDoubleLight.svg" : ":/icon/documentChevronDoubleDark.svg"));
    auto *widget = new QWidget(this);
    auto *layout = new QGridLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setHorizontalSpacing(32);

    QList<ScintillaWidget *> editors{};
    QList<QByteArray> contents{};
    for (int column = 0; column < 2; ++column) {
        auto *label = new QLabel(column == 0 ? leftTitle : rightTitle, widget);
        label->setTextFormat(Qt::PlainText);
        layout->addWidget(label, 0, column);
        auto *editor = new EditorWidget(config, {}, widget);
        layout->addWidget(editor, 1, column);
        layout->setColumnStretch(column, 1);
        editor->propertySet();
        auto *handler = editor->handler();
        auto text = QTextStream(column == 0 ? leftContent : rightContent).readAll();
        text.replace("\r\n", "\n").replace('\r', '\n');
        handler->textAppend(text);
        handler->savepointSet();
        handler->readonlySet(true);
        editors.append(handler);
        contents.append(text.toUtf8());
    }

    auto *overlay = new DiffOverlay(editors, widget);
    layout->addWidget(overlay, 1, 0, 1, 2);
    overlay->raise();
    setWidget(widget);
    for (int i = 0; i < 2; ++i) {
        connect(editors[i]->verticalScrollBar(), &QScrollBar::valueChanged, this, [this, editors, i] {
            if (m_scrolling) return;
            const QScopedValueRollback guard(m_scrolling, true);
            DiffScroll::sync(editors[i], editors[1 - i], m_anchors, i == 1);
        });
    }
    auto *git = g_globalManager->serviceModuleGet()->gitServiceGet();
    git->diffGet(contents[0], contents[1]).then(this, [this, overlay](const GitDiffResult &diff) {
        if (!diff.success) {
            g_globalManager->toastModuleGet()->show(ToastLevel::Error, tr("Compare failed"), diff.text());
            return;
        }
        m_anchors.append({0, 0});
        QList<DiffOverlay::Block> blocks{};
        for (const auto &hunk : diff.hunks) {
            m_anchors.append({hunk.leftStart, hunk.rightStart});
            m_anchors.append({hunk.leftStart + hunk.leftCount, hunk.rightStart + hunk.rightCount});
            blocks.append({{DiffOverlay::Range{hunk.leftStart, hunk.leftCount}, DiffOverlay::Range{hunk.rightStart, hunk.rightCount}},
                           !hunk.leftCount ? DiffKind::Added : !hunk.rightCount ? DiffKind::Deleted : DiffKind::Modified});
        }
        overlay->blocksSet(std::move(blocks));
    });
}
