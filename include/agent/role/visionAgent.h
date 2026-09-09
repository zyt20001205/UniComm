#ifndef UNICOMM_VISIONAGENT_H
#define UNICOMM_VISIONAGENT_H

#include "agent/role/baseAgent.h"

class VisionAgent final : public BaseAgent {
public:
    explicit VisionAgent(QObject *parent = nullptr);

    [[nodiscard]] QString systemGet() const override;
};

#endif //UNICOMM_VISIONAGENT_H
