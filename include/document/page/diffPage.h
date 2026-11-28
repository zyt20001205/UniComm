#ifndef UNICOMM_DIFFPAGE_H
#define UNICOMM_DIFFPAGE_H

#include "documentPage.h"
#include "document/module/diffScroll.h"

class DiffPage final : public DocumentPage {
public:
    DiffPage(const QJsonObject &config, const QString &leftTitle, const QByteArray &leftContent, const QString &rightTitle, const QByteArray &rightContent);

private:
    DiffScroll::Anchors m_anchors{};
    bool m_scrolling{};
};

#endif // UNICOMM_DIFFPAGE_H
