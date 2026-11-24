#ifndef UNICOMM_DOCUMENTPAGE_H
#define UNICOMM_DOCUMENTPAGE_H

#include <QJsonObject>
#include <kddockwidgets/qtwidgets/views/DockWidget.h>

#include "types/logTypes.h"

class DocumentPage : public KDDockWidgets::QtWidgets::DockWidget {
    Q_OBJECT

public:
    explicit DocumentPage(const QUrl &documentUrl = QUrl());

    ~DocumentPage() override = default;

    // public: file
    void pathDisambiguation();

    [[nodiscard]] virtual QString documentSave() {
        return {};
    }

    [[nodiscard]] virtual bool documentModified() const {
        return false;
    }

    void closeApprove();

    [[nodiscard]] QUrl documentUrl() {
        return m_documentUrl;
    }

    virtual void permissionGet();

signals:
    void appendLog(LogType::Value type, const QString &prefix, const QString &message);

    void closeRequest(const QUrl &documentUrl);

    void closeDocument(const QUrl &documentUrl);

    void gotoDocument();

    void showPermissionDialog(const QUrl &documentUrl);

protected:
    void closeEvent(QCloseEvent *event) override;

    QUrl m_documentUrl{};

private:
    bool m_closeApproved{};
};

#endif //UNICOMM_DOCUMENTPAGE_H
