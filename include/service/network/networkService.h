#ifndef UNICOMM_NETWORKSERVICE_H
#define UNICOMM_NETWORKSERVICE_H

#include "service/baseService.h"
#include "service/network/networkManager.h"

class NetworkService final : public BaseService {
    Q_OBJECT

public:
    explicit NetworkService(QObject *parent = nullptr);

    [[nodiscard]] QNetworkAccessManager *managerGet() const {
        return m_manager;
    }

private:
    NetworkManager *m_manager{};
    int m_activeRequests{};
};

#endif //UNICOMM_NETWORKSERVICE_H
