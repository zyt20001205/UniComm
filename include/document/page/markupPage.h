#ifndef UNICOMM_MARKUPPAGE_H
#define UNICOMM_MARKUPPAGE_H

#include "documentPage.h"
#include "document/module/editorWidget.h"

class WebviewWidget;

class MarkupPage final : public DocumentPage {
    Q_OBJECT

public:
    explicit MarkupPage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~MarkupPage() override = default;

    void propertySet() const;

    [[nodiscard]] ScintillaWidget *handler() const { return m_editorWidget->handler(); }

    [[nodiscard]] QString documentSave() override;

signals:
    void changeSelection(const QHash<QString, int> &selection);

protected:
    [[nodiscard]] bool documentModified() const override;

private:
    void previewUpdate() const;

    void savepointChange(bool status);

    EditorWidget *m_editorWidget{};
    WebviewWidget *m_webviewWidget{};
};

#endif //UNICOMM_MARKUPPAGE_H
