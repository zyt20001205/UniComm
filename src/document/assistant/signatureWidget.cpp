#include "document/assistant/signatureWidget.h"

#include <QJsonArray>
#include <QFont>

#include "globals.h"
#include "core/globalManager.h"
#include "core/themeManager.h"

// public
SignatureWidget::SignatureWidget(QObject *parent)
    : QObject(parent) {
}

void SignatureWidget::propertySet(const QVariantHash &objects) {
    m_tooltip = qvariant_cast<QObject *>(objects["documentModuleSignatureToolTip"]);
    m_tooltip->setProperty("signatureWidget", QVariant::fromValue(this));
}

void SignatureWidget::fontSet(const QString &family, const int pointSize) const {
    if (!m_tooltip) return;
    auto font = m_tooltip->property("font").value<QFont>();
    font.setFamily(family);
    font.setPointSize(pointSize);
    m_tooltip->setProperty("font", font);
}

bool SignatureWidget::isVisible() const {
    if (!m_tooltip) return false;
    return m_tooltip->property("visible").toBool();
}

void SignatureWidget::signatureShow(const QVariantHash &signatureSession, const QJsonArray &signatures) {
    m_signatureSession = signatureSession;
    const auto *theme = g_globalManager->themeGet();
    QString helpText{};
    bool reposition = false;
    for (const auto &value: signatures) {
        const auto signature = value.toObject();
        helpText += "(";
        int index = 0;
        const int activeParameter = signature["activeParameter"].toInt();
        if (activeParameter == 0) reposition = true;
        const QString label = signature["label"].toString();
        const QJsonArray parameters = signature["parameters"].toArray();
        if (parameters.isEmpty()) {
            helpText += ")<br>";
            continue;
        }
        for (const QJsonValue &_value: parameters) {
            const QJsonObject parameter = _value.toObject();
            const QJsonArray range = parameter["label"].toArray();
            const int startIndex = range[0].toInt();
            const int endIndex = range[1].toInt();
            QString param = label.mid(startIndex, endIndex - startIndex).toHtmlEscaped();
            if (index == activeParameter) {
                param = QString("<span style='color: %1; font-weight: 600;'>%2</span>").arg(theme->warningFore3Get(), param);
            }
            helpText += param;
            helpText += ", ";
            index++;
        }
        helpText.chop(2);
        helpText += ")<br>";
    }
    helpText.chop(4);
    if (m_text != helpText) {
        m_text = helpText;
        emit textChanged();
    }
    if (reposition) {
        const auto position = signatureSession["position"].toPoint();
        m_tooltip->setProperty("position", position);
    }
    QMetaObject::invokeMethod(m_tooltip, "open");
}

void SignatureWidget::signatureHide() {
    QMetaObject::invokeMethod(m_tooltip, "close");
    if (m_text.isEmpty()) return;
    m_text.clear();
    emit textChanged();
}

void SignatureWidget::signatureNext() {
    emit setTextSelected(
        m_signatureSession["documentUrl"].toUrl(),
        ", ");
    emit addChar(
        m_signatureSession["documentUrl"].toUrl(),
        ',');
}
