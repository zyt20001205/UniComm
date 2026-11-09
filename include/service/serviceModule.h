#ifndef UNICOMM_SERVICEMODULE_H
#define UNICOMM_SERVICEMODULE_H

#include <QObject>

class LspService;

class ServiceModule final : public QObject {
public:
    explicit ServiceModule(QObject *parent = nullptr);

    [[nodiscard]] LspService *lspServiceGet() const;

private:
    LspService *m_lspService{};
};

#endif //UNICOMM_SERVICEMODULE_H
