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
                  "vision_analyze",
                  "run_script",
                  // debug toolset
                  "debug_breakpoint_list",
                  "debug_breakpoint_set",
                  "debug_breakpoint_delete",
                  "debug_script",
                  "debug_threads_get",
                  "debug_call_stack_get",
                  "debug_resume",
                  "debug_step_over",
                  "debug_step_into",
                  "debug_step_out"
              }
          },
          {
              "supervisor",
              {
                  // "api_list",
                  // "api_get",
                  // "demo_get",
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
                  "run_script"
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
                  "run_script",
                  // debug toolset
                  "debug_breakpoint_list",
                  "debug_breakpoint_set",
                  "debug_breakpoint_delete",
                  "debug_script",
                  "debug_threads_get",
                  "debug_call_stack_get",
                  "debug_resume",
                  "debug_step_over",
                  "debug_step_into",
                  "debug_step_out"
              }
          }
      } {
}

QString BaseAgent::roleGet() const {
    return m_role;
}

QJsonArray BaseAgent::toolsGet(const ToolsModule &toolsModule) const {
    return toolsModule.toolsGet(m_tools.value(m_role), m_role == "general" || m_role == "supervisor");
}

bool BaseAgent::toolContains(const QString &name) const {
    return m_tools.value(m_role).contains(name);
}
