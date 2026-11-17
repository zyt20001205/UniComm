#include "document/assistant/searchWindow.h"

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QQmlContext>
#include <QProcess>
#include <QQuickView>
#include <QRegularExpression>
#include <QUrl>

#include "globals.h"
#include "core/globalManager.h"
#include "core/themeManager.h"
#include "mainWindow/mainWindow.h"
#include "types/scintillaTypes.h"

// public
SearchWindow::SearchWindow(QObject *parent)
    : QObject(parent),
      m_searchWindow(new QQuickView()),
      m_searchModel(new SearchModel(this)) {
    m_searchWindow->setTitle("Search");
}

SearchWindow::~SearchWindow() {
    delete m_searchWindow;
}

void SearchWindow::propertySet(const QVariantHash &objects) {
    m_searchWindow->setTransientParent(g_globalManager->mainWindowGet()->windowHandle());
    m_searchWindow->rootContext()->setContextProperty("searchWindow", this);
    m_searchWindow->rootContext()->setContextProperty("global", g_globalManager);
    m_searchWindow->rootContext()->setContextProperty("mainToolTip", qvariant_cast<QObject *>(objects["mainWindowToolTip"]));
    m_searchWindow->rootContext()->setContextProperty("searchModel", m_searchModel);

    m_searchWindow->setResizeMode(QQuickView::SizeRootObjectToView);
    m_searchWindow->setSource(QUrl("qrc:/qml/document/assistant/searchWindow.qml"));
}

void SearchWindow::propertyGet(const QVariantMap &objects) {
    m_searchBar = qvariant_cast<QObject *>(objects["searchBar"]);
    m_searchTextField = qvariant_cast<QObject *>(objects["searchTextField"]);
    m_searchStatLabel = qvariant_cast<QObject *>(objects["searchStatLabel"]);
}

void SearchWindow::open() const {
    m_searchWindow->resize(600, 600);
    m_searchWindow->show();
    m_searchWindow->raise();
    m_searchWindow->requestActivate();
}

void SearchWindow::searchRequest() const {
    m_searchModel->clear();
    const auto text = m_searchTextField->property("text").toString();
    if (text.isEmpty()) return;
    // fill pattern
    QString pattern = text;
    QStringList args{};
    args << "--json";
    if (!m_searchFlags.matchCase) args << "--ignore-case";
    if (m_searchFlags.wholeWord) args << "--word-regexp";
    if (m_searchFlags.wordStart) {
        if (!m_searchFlags.regExp) pattern = QRegularExpression::escape(pattern);
        pattern = "\\b" + pattern;
    } else if (!m_searchFlags.regExp) {
        args << "--fixed-strings";
    }
    args << "--" << pattern << g_workspaceUrl.toLocalFile();
    // start search
    QProcess rg{};
    rg.start(QCoreApplication::applicationDirPath() + "/ripgrep/rg.exe", args);
    if (!rg.waitForFinished() || (rg.exitCode() != 0 && rg.exitCode() != 1)) return;
    // parse result
    int matchCount = 0;
    const auto out = rg.readAllStandardOutput();
    for (const auto &line: out.split('\n')) {
        if (line.isEmpty()) continue;
        const auto json = QJsonDocument::fromJson(line).object();
        if (json.value("type").toString() != "match") continue;

        const auto data = json.value("data").toObject();
        const auto path = data.value("path").toObject().value("text").toString();
        const auto relativePath = QDir(g_workspaceUrl.toLocalFile()).relativeFilePath(path);
        const auto _line = data.value("line_number").toInt();
        auto _text = data.value("lines").toObject().value("text").toString();
        _text.remove('\r');
        _text.remove('\n');
        const auto submatches = data.value("submatches").toArray();
        const auto bytes = _text.toUtf8();
        QString richText{};
        int offset = 0;
        for (const auto &value: submatches) {
            const auto submatch = value.toObject();
            const int startByte = submatch.value("start").toInt(0);
            const int endByte = submatch.value("end").toInt(-1);
            if (startByte < offset || endByte <= startByte || endByte > bytes.size()) continue;
            richText += QString::fromUtf8(bytes.mid(offset, startByte - offset)).toHtmlEscaped();
            richText += QString("<span style=\"background-color:%1;\">%2</span>")
                .arg(g_globalManager->themeGet()->warningBack2Get(), QString::fromUtf8(bytes.mid(startByte, endByte - startByte)).toHtmlEscaped());
            offset = endByte;
        }
        richText += QString::fromUtf8(bytes.mid(offset)).toHtmlEscaped();

        auto *pathItem = new QStandardItem(relativePath); // NOLINT
        auto *lineItem = new QStandardItem(QString::number(_line)); // NOLINT
        auto *textItem = new QStandardItem(richText); // NOLINT
        const auto documentUrl = QUrl::fromLocalFile(path).toString();
        pathItem->setData(documentUrl, SearchModel::DocumentUrlRole);
        lineItem->setData(documentUrl, SearchModel::DocumentUrlRole);
        textItem->setData(documentUrl, SearchModel::DocumentUrlRole);
        pathItem->setData(_line - 1, SearchModel::LineRole);
        lineItem->setData(_line - 1, SearchModel::LineRole);
        textItem->setData(_line - 1, SearchModel::LineRole);

        m_searchModel->appendRow({pathItem, lineItem, textItem});
        ++matchCount;
    }
    m_searchStatLabel->setProperty("matchCount", matchCount);
}

void SearchWindow::searchFlagsSet(const bool matchCase, const bool wholeWord, const bool wordStart, const bool regExp) {
    m_searchFlags.matchCase = matchCase;
    m_searchFlags.wholeWord = wholeWord;
    m_searchFlags.wordStart = wordStart;
    m_searchFlags.regExp = regExp;
}

void SearchWindow::searchNavigate(const QUrl &documentUrl, const int line) {
    emit setIndex(documentUrl, line, 0);
    emit addMarker(documentUrl, ScintillaMarker::Hint, line, 1000);
}

// public
SearchModel::SearchModel(QObject *parent)
    : QStandardItemModel(parent) {
    connect(this, &QAbstractItemModel::rowsInserted, this, &SearchModel::emptyChanged);
    connect(this, &QAbstractItemModel::rowsRemoved, this, &SearchModel::emptyChanged);
    connect(this, &QAbstractItemModel::modelReset, this, &SearchModel::emptyChanged);
}

QHash<int, QByteArray> SearchModel::roleNames() const {
    auto roles = QStandardItemModel::roleNames();
    roles[DocumentUrlRole] = "documentUrl";
    roles[LineRole] = "line";
    return roles;
}
