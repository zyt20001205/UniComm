#ifndef UNICOMM_TEXTPAGE_H
#define UNICOMM_TEXTPAGE_H

#include "documentPage.h"
#include "document/module/editorWidget.h"

class TextPage final : public DocumentPage {
    Q_OBJECT

public:
    explicit TextPage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~TextPage() override = default;

    void propertySet() const;

    [[nodiscard]] ScintillaWidget *handler() const { return m_editorWidget->handler(); }

    [[nodiscard]] QString documentSave() override;

signals:
    void changeSelection(const QHash<QString, int> &selection);

protected:
    [[nodiscard]] bool documentModified() const override;

private:
    void savepointChange(bool status);

    EditorWidget *m_editorWidget{};
};

#endif //UNICOMM_TEXTPAGE_H
