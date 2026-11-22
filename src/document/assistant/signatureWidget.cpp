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
    m_signatureSession = signatureSession;
    const auto general = g_globalManager->themeGet()["general"].toObject();
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
    emit showSignature(signatureSession["position"].toPoint(), reposition);
}

void SignatureWidget::signatureHide() {
    emit hideSignature();
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
