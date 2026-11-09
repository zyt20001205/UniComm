#include "service/serviceModule.h"

#include "service/lsp/lspService.h"
#include "service/mcp/mcpService.h"

ServiceModule::ServiceModule(QObject *parent)
    : QObject(parent),
      m_lspService(new LspService(this)) {
    new McpService(this);
}

LspService *ServiceModule::lspServiceGet() const {
    return m_lspService;
}
