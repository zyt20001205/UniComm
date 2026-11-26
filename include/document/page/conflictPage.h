#ifndef UNICOMM_CONFLICTPAGE_H
#define UNICOMM_CONFLICTPAGE_H

#include <array>

#include "documentPage.h"
#include "document/module/editorWidget.h"

class QWidget;

class DiffOverlay;
class ResolveWidget;
struct GitDiffResult;

class ConflictPage final : public DocumentPage {
    Q_OBJECT

public:
    explicit ConflictPage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~ConflictPage() override = default;

    void propertySet() const;

    [[nodiscard]] ScintillaWidget *handler() const { return m_baseEditor->handler(); }

    [[nodiscard]] QString documentSave() override;

signals:
    void changeSelection(const QHash<QString, int> &selection);

    void reloadDocument(const QString &documentPath);

protected:
    [[nodiscard]] bool documentModified() const override;

private:
    using ScrollAnchors = QList<std::array<int, 2>>; // Base line, side line

    void diffSet(ScintillaWidget *ours, ScintillaWidget *theirs, const GitDiffResult &oursDiff, const GitDiffResult &theirsDiff);

    void scrollSync(int source);

    void savepointChange(bool status);

    void resolveFinish();

    QWidget *m_widget{};
    EditorWidget *m_baseEditor{};
    ResolveWidget *m_resolveWidget{};
    DiffOverlay *m_diffOverlay{};
    std::array<ScintillaWidget *, 3> m_editors{};
    std::array<ScrollAnchors, 2> m_scrollAnchors{}; // Ours, Theirs
    bool m_scrolling{};
};

#endif //UNICOMM_CONFLICTPAGE_H
