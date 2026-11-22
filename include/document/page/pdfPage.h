#ifndef UNICOMM_PDFPAGE_H
#define UNICOMM_PDFPAGE_H

#include "documentPage.h"

class QPdfDocument;
class QQuickWidget;

class PdfPage final: public DocumentPage {
    Q_OBJECT

public:
    explicit PdfPage(const QJsonObject &documentConfig = QJsonObject(), const QUrl &documentUrl = QUrl());

    ~PdfPage() override = default;

    void propertySet();

    Q_INVOKABLE void propertyGet(const QVariantMap &objects);

    [[nodiscard]] QString textGet(int page) const;

private:
    QQuickWidget *m_pdfWidget{};
    QObject *m_pdf{};
    QPdfDocument *m_doc{};
};

#endif //UNICOMM_PDFPAGE_H
