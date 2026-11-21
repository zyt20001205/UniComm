#ifndef UNICOMM_CONPTYWIDGET_H
#define UNICOMM_CONPTYWIDGET_H

#include <QThread>

class ConptyWidget final : public QObject {
    Q_OBJECT

public:
    explicit ConptyWidget(QObject *parent = nullptr);

    ~ConptyWidget() override;

    [[nodiscard]] bool start(const QUrl &program, const QString &arguments, const QUrl &workingDirectory, const QString &environment, int rows, int cols);

    void inputWrite(const QByteArray &bytes) const;

    void resize(int rows, int cols) const;

    void stop();

    [[nodiscard]] bool running() const;

signals:
    void quit();

    void stopped();

    void outputWrite(const QByteArray &bytes);

private:
    void outputRead();

    static QString environmentParse(const QString &environment);

    static void closeHandle(void *&handle);

    void *m_pseudoConsole{};
    void *m_conptyInputWrite{};
    void *m_conptyOutputRead{};
    void *m_processHandle{};
    void *m_stopEvent{};
    QThread *m_readerThread{};
    QThread *m_processThread{};
};

#endif //UNICOMM_CONPTYWIDGET_H
