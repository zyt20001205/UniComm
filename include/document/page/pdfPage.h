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

    [[nodiscard]] QString textGet(int page) const;

private:
    QQuickWidget *m_pdfWidget{};
    QPdfDocument *m_doc{};
};

#endif //UNICOMM_PDFPAGE_H
