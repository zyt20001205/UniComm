#include "agent/role/baseAgent.h"

#include <utility>

#include "agent/module/toolsModule.h"

// public
BaseAgent::BaseAgent(QString id, QObject *parent)
    : QObject(parent),
      m_role(std::move(id)),
      m_tools{
          {
              "general",
              {
                  "api_list",
                  "api_get",
                  "demo_get",
                  "database_list",
                  "database_create",
                  "database_delete",
                  "datatable_list",
                  "datatable_create",
                  "datatable_delete",
                  "plan_update",
                  "user_input_request",
                  "diagnostics_get",
                  "directory_list",
                  "directory_create",
                  "directory_delete",
                  "directory_rename",
                  "document_create",
                  "document_delete",
                  "document_rename",
                  "grep_search",
                  "line_get",
                  "line_set",
                  "memory_search",
                  "port_list",
                  "port_config_get",
                  "port_create",
                  "port_delete",
                  "subagent_dispatch",
                  "script_exec"
              }
          },
          {
              "supervisor",
              {
                  // "api_list",
                  // "api_get",
                  // "demo_get",
                  "database_list",
                  "datatable_list",
                  "directory_list",
                  "port_list",
                  "subagent_dispatch",
                  "plan_update",
                  "user_input_request",
                  // "diagnostics_get",
                  // "grep_search",
                  // "line_get",
                  // "line_set",
                  "memory_search",
                  "script_exec"
              }
          },
          {
              "data",
              {
                  "database_list",
                  "database_create",
                  "database_delete",
                  "datatable_list",
                  "datatable_create",
                  "datatable_delete",
                  "user_input_request"
              }
          },
          {
              "hardware",
              {
                  "port_list",
                  "port_config_get",
                  "port_create",
                  "port_delete",
                  "user_input_request"
              }
          },
          {
              "software",
              {
                  "api_list",
                  "api_get",
                  "demo_get",
                  "user_input_request",
                  "diagnostics_get",
                  "directory_list",
                  "directory_create",
                  "directory_delete",
                  "directory_rename",
                  "document_create",
                  "document_delete",
                  "document_rename",
                  "grep_search",
                  "line_get",
                  "line_set",
                  "script_exec"
              }
          }
      } {
}

QString BaseAgent::roleGet() const {
    return m_role;
}

QJsonArray BaseAgent::toolsGet(const ToolsModule &toolsModule) const {
    return toolsModule.toolsGet(m_tools.value(m_role), m_role, m_role == "general" || m_role == "supervisor");
}

bool BaseAgent::toolContains(const QString &name) const {
    return m_tools.value(m_role).contains(name);
}
