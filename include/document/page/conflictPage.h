#ifndef UNICOMM_CONFLICTPAGE_H
#define UNICOMM_CONFLICTPAGE_H

#include "documentPage.h"
#include "document/module/conflictWidget.h"

class QWidget;

class ResolveWidget;

class ConflictPage final : public DocumentPage {
    Q_OBJECT

public:
    explicit ConflictPage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~ConflictPage() override = default;

    void propertySet() const;

    [[nodiscard]] ScintillaWidget *handler() const { return m_conflictWidget->handler(); }

    [[nodiscard]] QString documentSave() override;

signals:
    void changeSelection(const QHash<QString, int> &selection);

    void reloadDocument(const QString &documentPath);

protected:
    [[nodiscard]] bool documentModified() const override;

private:
    void savepointChange(bool status);

    void resolveFinish();

    QWidget *m_widget{};
    ConflictWidget *m_conflictWidget{};
    ResolveWidget *m_resolveWidget{};
};

#endif //UNICOMM_CONFLICTPAGE_H
