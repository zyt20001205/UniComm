#ifndef UNICOMM_FILEMODULE_H
#define UNICOMM_FILEMODULE_H

#include <QUrl>
#include <QVariantHash>

class FileModule final : public QObject {
    Q_OBJECT

public:
    explicit FileModule(QObject *parent = nullptr);

    ~FileModule() override = default;

    void propertySet();

    Q_INVOKABLE void fileOpenInExplorer(const QUrl &fileUrl);

    Q_INVOKABLE void fileOpenInApplication(const QUrl &fileUrl);

    Q_INVOKABLE static QVariantHash fileInfo(const QUrl &fileUrl);

    Q_INVOKABLE void fileCopy(const QList<QUrl> &sourceUrls, const QUrl &directoryUrl);

    Q_INVOKABLE bool fileRename(const QUrl &sourceUrl, const QUrl &targetUrl);

    Q_INVOKABLE void fileWritable(const QUrl &fileUrl, bool status);

    Q_INVOKABLE void copyToClipboard(const QString &text) const;

    static int lineCountGet(const QUrl &documentUrl);

    static QString linesGet(const QUrl &documentUrl, int startLine, int lineCount);

    static QString textGet(const QUrl &documentUrl, int startLine = -1, int startCharacter = -1, int endLine = -1, int endCharacter = -1);

    // void textSet();

signals:
    void showPropertyDialog(const QUrl &documentUrl);

    void showRenameDialog(const QUrl &documentUrl);

    void showPermissionDialog(const QUrl &documentUrl);

    void setPermission(const QUrl &documentUrl);

    int appendBackground(const std::function<void()> &abort, const std::function<void()> &info);

    void removeBackground(int taskId);

    void refreshBackground(int taskId, const QString &message);
};

#endif //UNICOMM_FILEMODULE_H
