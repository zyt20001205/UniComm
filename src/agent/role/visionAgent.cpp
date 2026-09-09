#include "agent/role/visionAgent.h"

VisionAgent::VisionAgent(QObject *parent)
    : BaseAgent("vision", parent) {
}

QString VisionAgent::systemGet() const {
    return "You are a vision agent responsible for analyzing attached images for another agent. "
           "Follow the delegated prompt and inspect only the supplied attachments. Report direct observations, the verification result, and any uncertainty. "
           "Do not infer that an expected condition succeeded when it is not visibly confirmed. Return only a concise analysis for the primary agent.";
}
