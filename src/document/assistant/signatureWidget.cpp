#include "document/assistant/signatureWidget.h"

#include <QJsonArray>
#include <QQmlComponent>

#include "globals.h"
#include "core/globalManager.h"

// public
SignatureWidget::SignatureWidget(QObject *parent)
    : QObject(parent) {
}

void SignatureWidget::propertySet() {
    QQmlComponent component(g_globalManager->engineGet(), QUrl("qrc:/qml/document/assistant/signatureWidget.qml"));
    if (auto *root = component.createWithInitialProperties({{"signatureWidget", QVariant::fromValue(this)}})) root->setParent(this);
    else qWarning().noquote() << component.errorString();
}

void SignatureWidget::fontSet(const QString &family, const int pointSize) {
    m_font.setFamily(family);
    m_font.setPointSize(pointSize);
    emit fontChanged();
}

void SignatureWidget::signatureShow(const QVariantHash &signatureSession, const QJsonArray &signatures) {
    if (signatures.isEmpty()) {
        signatureHide();
        return;
    }
    m_documentUrl = signatureSession["documentUrl"].toUrl();
    const auto general = g_globalManager->themeGet()["general"].toObject();
    QString helpText{};
    for (const auto &value: signatures) {
        const auto signature = value.toObject();
        helpText += "(";
        int index = 0;
        const int activeParameter = signature["activeParameter"].toInt();
        const auto label = signature["label"].toString();
        const auto parameters = signature["parameters"].toArray();
        if (parameters.isEmpty()) {
            helpText += ")<br>";
            continue;
        }
        for (const auto &_value: parameters) {
            const auto parameter = _value.toObject();
            const auto range = parameter["label"].toArray();
            const auto startIndex = range[0].toInt();
            const auto endIndex = range[1].toInt();
            auto param = label.mid(startIndex, endIndex - startIndex).toHtmlEscaped();
            if (index == activeParameter) {
                param = QString("<span style='color: %1; font-weight: 600;'>%2</span>").arg(general["warningFore3"].toString(), param);
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
    emit showSignature(signatureSession["position"].toPoint());
}

void SignatureWidget::signatureHide() {
    emit hideSignature();
    if (m_text.isEmpty()) return;
    m_text.clear();
    emit textChanged();
}

void SignatureWidget::signatureNext() {
    emit setTextSelected(m_documentUrl, ", ");
    emit addChar(m_documentUrl, ',');
}
