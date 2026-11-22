#include "agent/module/toolsModule.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QPromise>
#include <QSharedPointer>
#include <QUrl>

#include "globals.h"
#include "core/globalManager.h"
#include "agent/agentModule.h"
#include "agent/module/mcpModule.h"
#include "agent/module/sqlModule.h"
#include "agent/runtime/runtimeModule.h"
#include "debug/breakpointModule.h"
#include "debug/debugModule.h"
#include "document/documentModule.h"
#include "port/portModule.h"
#include "runtime/threadpoolModule.h"
#include "service/grepService.h"
#include "service/serviceModule.h"

// public
ToolsModule::ToolsModule(McpModule *mcpModule, SqlModule *sqlModule, QObject *parent)
    : QObject(parent),
      m_portTypes{
          {"serial_port", PortType::SerialPort},
          {"vision", PortType::Vision},
          {"tcp_client", PortType::TcpClient},
          {"tcp_server", PortType::TcpServer},
          {"ssl_client", PortType::SslClient},
          {"ssl_server", PortType::SslServer},
          {"websocket_client", PortType::WebSocketClient},
          {"websocket_server", PortType::WebSocketServer},
          {"udp_socket", PortType::UdpSocket}
      },
      m_mcpModule(mcpModule),
      m_sqlModule(sqlModule),
      m_writeGroup{"directory_create", "directory_rename", "document_create", "document_rename", "line_set", "port_create", "debug_breakpoint_set", "debug_breakpoint_delete"},
      m_fullAccessGroup{
          "directory_delete", "document_delete", "port_delete", "run_script", "debug_script", "debug_resume", "debug_step_over", "debug_step_into", "debug_step_out"
      } {
}

void ToolsModule::initialize() {
    m_tools = QJsonArray{
        // apiList
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "api_list"},
                    {"description", "Get the list of all available API packages/modules that can be queried for detailed annotations."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {"properties", QJsonObject{}},
                            {"required", QJsonArray{}}
                        }
                    }
                }
            }
        },
        // apiGet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "api_get"},
                    {"description", "Get the detailed API annotations (function signatures, types, comments) for a specific package."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "package_name", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The name of the package/module to query. Use api_list first to get available names."}
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{"package_name"}
                            }
                        }
                    }
                }
            }
        },
        // demoGet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "demo_get"},
                    {
                        "description",
                        "Read a Lua demo or list the demos and subdirectories in a demo directory. Directory entries contain paths that can be passed back to this tool."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "package_name", QJsonObject{
                                            {"type", "string"},
                                            {
                                                "description",
                                                "Demo path relative to the demo directory, without .lua. For example: smtp, port, port/common, or port/bluetoothLe. Use api_list for package names, then query a package to read its demo or list its examples."
                                            }
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{"package_name"}
                            }
                        }
                    }
                }
            }
        },
        // subagentDispatch
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "subagent_dispatch"},
                    {"description", "Delegate one or more independent tasks to specialized agents, run them concurrently, and wait for all final results."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "tasks", QJsonObject{
                                            {"type", "array"},
                                            {"description", "Independent tasks that can run concurrently."},
                                            {
                                                "items", QJsonObject{
                                                    {"type", "object"},
                                                    {
                                                        "properties", QJsonObject{
                                                            {
                                                                "role", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"enum", QJsonArray{"hardware", "software", "vision"}},
                                                                    {"description", "The specialized agent role."}
                                                                }
                                                            },
                                                            {
                                                                "prompt", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"description", "A complete, self-contained prompt for the specialized agent."}
                                                                }
                                                            },
                                                            {
                                                                "attachments", QJsonObject{
                                                                    {"type", "array"},
                                                                    {"description", "Exact local file URLs to provide to the specialized agent. Required for vision tasks."},
                                                                    {"items", QJsonObject{{"type", "string"}}}
                                                                }
                                                            }
                                                        }
                                                    },
                                                    {"required", QJsonArray{"role", "prompt"}}
                                                }
                                            }
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"tasks"}}
                        }
                    }
                }
            }
        },
        // visionAnalyze
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "vision_analyze"},
                    {"description", "Analyze one or more attached images with the vision agent and return its observations."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "prompt", QJsonObject{
                                            {"type", "string"},
                                            {"description", "A precise, self-contained image analysis or visual verification prompt."}
                                        }
                                    },
                                    {
                                        "attachments", QJsonObject{
                                            {"type", "array"},
                                            {"description", "Exact local file URLs of the images to analyze."},
                                            {"items", QJsonObject{{"type", "string"}}}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"prompt", "attachments"}}
                        }
                    }
                }
            }
        },
        // planUpdate
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "plan_update"},
                    {
                        "description",
                        "Create or update the execution plan for a complex task. Submit the complete plan on every update, keep at most one step in progress, and mark steps completed only after the work is finished. Do not use this tool for simple one-step tasks."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "explanation", QJsonObject{
                                            {"type", "string"},
                                            {"description", "A concise user-facing summary of the current progress or why the plan changed."}
                                        }
                                    },
                                    {
                                        "plan", QJsonObject{
                                            {"type", "array"},
                                            {
                                                "items", QJsonObject{
                                                    {"type", "object"},
                                                    {
                                                        "properties", QJsonObject{
                                                            {
                                                                "step", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"description", "A concise actionable step."}
                                                                }
                                                            },
                                                            {
                                                                "status", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"enum", QJsonArray{"pending", "in_progress", "completed"}}
                                                                }
                                                            }
                                                        }
                                                    },
                                                    {"required", QJsonArray{"step", "status"}}
                                                }
                                            }
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"plan"}}
                        }
                    }
                }
            }
        },
        // userInputRequest
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "user_input_request"},
                    {
                        "description",
                        "Ask the user one concise question only when the missing information cannot be discovered with tools and choosing incorrectly would materially affect the result. Investigate first and do not ask for information that tools can provide. Offer up to three mutually exclusive suggested answers when useful."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "question", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The concise question to show the user."}
                                        }
                                    },
                                    {
                                        "options", QJsonObject{
                                            {"type", "array"},
                                            {
                                                "description",
                                                "Zero to three mutually exclusive suggested answers, with the recommended option first. Use an empty array when the user should provide free-form input."
                                            },
                                            {
                                                "items", QJsonObject{
                                                    {"type", "object"},
                                                    {
                                                        "properties", QJsonObject{
                                                            {
                                                                "label", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"description", "The concise answer returned when this option is selected."}
                                                                }
                                                            },
                                                            {
                                                                "description", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"description", "A short explanation of the option's impact or meaning."}
                                                                }
                                                            }
                                                        }
                                                    },
                                                    {"required", QJsonArray{"label", "description"}}
                                                }
                                            }
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"question", "options"}}
                        }
                    }
                }
            }
        },
        // portList
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "port_list"},
                    {"description", "Get the list of all available ports that can be queried for detailed annotations."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {"properties", QJsonObject{}},
                            {"required", QJsonArray{}}
                        }
                    }
                }
            }
        },
        // portConfigGet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "port_config_get"},
                    {"description", "Get the required fields, defaults, constraints, and available options for creating a port of the specified type."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "port_type", QJsonObject{
                                            {"type", "string"},
                                            {
                                                "enum",
                                                QJsonArray{
                                                    "serial_port", "vision", "tcp_client", "tcp_server", "ssl_client", "ssl_server", "websocket_client", "websocket_server",
                                                    "udp_socket"
                                                }
                                            },
                                            {"description", "The type of port to configure."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"port_type"}}
                        }
                    }
                }
            }
        },
        // portInsert
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "port_create"},
                    {"description", "Create a configured port. Call port_config_get first and pass a configuration matching the returned definition."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "port_type", QJsonObject{
                                            {"type", "string"},
                                            {
                                                "enum",
                                                QJsonArray{
                                                    "serial_port", "vision", "tcp_client", "tcp_server", "ssl_client", "ssl_server", "websocket_client", "websocket_server",
                                                    "udp_socket"
                                                }
                                            },
                                            {"description", "The type of port to create."}
                                        }
                                    },
                                    {
                                        "config", QJsonObject{
                                            {"type", "object"},
                                            {"description", "The type-specific configuration returned by port_config_get."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"port_type", "config"}}
                        }
                    }
                }
            }
        },
        // portRemove
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "port_delete"},
                    {"description", "Delete an existing configured port by name. Call port_list first to get the available port names."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "port_name", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The name of the port to delete."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"port_name"}}
                        }
                    }
                }
            }
        },
        // diagnosticsGet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "diagnostics_get"},
                    {
                        "description",
                        "Get the diagnostics for a specified document. Returns an array of diagnostic items. If the array is empty, there are no diagnostics (i.e., no errors or warnings). Errors related to 'PLACEHOLDER' should be ignored as they are expected placeholders."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The URL / file path of the document."}
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{
                                    "document_url"
                                }
                            }
                        }
                    }
                }
            }
        },
        // directoryList
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "directory_list"},
                    {
                        "description",
                        "List the direct children of a directory and return URLs for subsequent directory and document operations. Omit directory_url to list the current workspace root."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "directory_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the directory to list."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{}}
                        }
                    }
                }
            }
        },
        // directoryCreate
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "directory_create"},
                    {"description", "Create a directory at an absolute local file URL."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "directory_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the directory to create."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"directory_url"}}
                        }
                    }
                }
            }
        },
        // directoryDelete
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "directory_delete"},
                    {"description", "Move an existing directory and all of its contents to the system trash."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "directory_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the directory to delete."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"directory_url"}}
                        }
                    }
                }
            }
        },
        // directoryRename
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "directory_rename"},
                    {"description", "Rename or move an existing directory to an exact target URL."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "source_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the existing directory."}
                                        }
                                    },
                                    {
                                        "target_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The exact absolute local file URL for the renamed or moved directory."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"source_url", "target_url"}}
                        }
                    }
                }
            }
        },
        // documentCreate
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "document_create"},
                    {"description", "Create and open an empty document at an absolute local file URL. Use line_set afterward to add content."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the document to create."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"document_url"}}
                        }
                    }
                }
            }
        },
        // documentDelete
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "document_delete"},
                    {"description", "Move an existing document to the system trash."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the document to delete."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"document_url"}}
                        }
                    }
                }
            }
        },
        // documentRename
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "document_rename"},
                    {"description", "Rename or move an existing document to an exact target URL."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "source_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The absolute local file URL of the existing document."}
                                        }
                                    },
                                    {
                                        "target_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The exact absolute local file URL for the renamed or moved document."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"source_url", "target_url"}}
                        }
                    }
                }
            }
        },
        // grepSearch
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "grep_search"},
                    {
                        "description",
                        "Search text across files in the current workspace. Returns count (number of returned match entries), truncated (whether more matches were omitted), and result (up to 50 match entries)."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "pattern", QJsonObject{
                                            {"type", "string"},
                                            {"description", "Text or regex pattern to search for."}
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{
                                    "pattern"
                                }
                            }
                        }
                    }
                }
            }
        },
        // linesGet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "line_get"},
                    {
                        "description",
                        "Read lines from a text document. Each returned line is prefixed with its 0-based line number in the form line|content. "
                        "Use start_line = 0 and line_count = -1 to read the whole document. A line_count of -1 reads from start_line to the end of the document. "
                        "If the requested range extends past the end of the document, all remaining lines are returned."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The URL / file path of the document."}
                                        }
                                    },
                                    {
                                        "start_line", QJsonObject{
                                            {"type", "integer"},
                                            {
                                                "description",
                                                "The starting line number (0-based). "
                                                "For PDF files, this is the page index (0-based)."
                                            }
                                        }
                                    },
                                    {
                                        "line_count", QJsonObject{
                                            {"type", "integer"},
                                            {
                                                "description",
                                                "The number of lines to read. Pass -1 to read to the end of a text document. "
                                                "For PDF files, this value is ignored."
                                            }
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{
                                    "document_url",
                                    "start_line",
                                    "line_count"
                                }
                            }
                        }
                    }
                }
            }
        },
        // linesSet
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "line_set"},
                    {
                        "description",
                        "Apply one or more non-overlapping whole-line replacements to a text document and save successful edits to disk before returning. All coordinates refer to the document state before this tool call. Each replacement whose expected line no longer matches is skipped without blocking the others."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The URL / file path of the document."}
                                        }
                                    },
                                    {
                                        "edits", QJsonObject{
                                            {"type", "array"},
                                            {"description", "The non-overlapping line ranges to replace."},
                                            {"minItems", 1},
                                            {
                                                "items", QJsonObject{
                                                    {"type", "object"},
                                                    {
                                                        "properties", QJsonObject{
                                                            {
                                                                "start_line", QJsonObject{
                                                                    {"type", "integer"},
                                                                    {"description", "The starting line number (0-based)."}
                                                                }
                                                            },
                                                            {
                                                                "line_count", QJsonObject{
                                                                    {"type", "integer"},
                                                                    {
                                                                        "description",
                                                                        "The number of existing lines to replace. Pass -1 to replace from start_line to the end of the document."
                                                                    }
                                                                }
                                                            },
                                                            {
                                                                "expected", QJsonObject{
                                                                    {"type", "string"},
                                                                    {"description", "The current content of start_line without the line number and | prefix."}
                                                                }
                                                            },
                                                            {
                                                                "text", QJsonObject{
                                                                    {"type", "string"},
                                                                    {
                                                                        "description",
                                                                        "The replacement text without line-number prefixes. Pass an empty string to clear the target lines."
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    },
                                                    {"required", QJsonArray{"start_line", "line_count", "expected", "text"}}
                                                }
                                            }
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{
                                    "document_url",
                                    "edits"
                                }
                            }
                        }
                    }
                }
            }
        },
        // memorySearch
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "memory_search"},
                    {
                        "description",
                        "Search prior conversations in the current workspace when historical decisions, requirements, errors, or implementation details may help with the current task. Returns complete conversation turns around the best keyword matches."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "query", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The FTS5 keyword query used to search conversation memory."}
                                        }
                                    },
                                    {
                                        "limit", QJsonObject{
                                            {"type", "integer"},
                                            {"description", "The maximum number of matching turns to return."},
                                            {"minimum", 1},
                                            {"maximum", 5}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"query"}}
                        }
                    }
                }
            }
        },
        // runScript
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "run_script"},
                    {
                        "description",
                        "Execute the specified script and return a JSON object containing output (stdout), err (stderr), and truncated (whether either captured stream exceeded 64 KiB). Before execution, you must first call diagnostics_get to verify that there are no syntax errors or warnings. Use print or io.write to expose results; port communication logs remain in the Log panel."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "document_url", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The URL / file path of the document to execute."}
                                        }
                                    }
                                }
                            },
                            {
                                "required", QJsonArray{
                                    "document_url"
                                }
                            }
                        }
                    }
                }
            }
        },
        // debug toolset
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_breakpoint_list"},
                    {"description", "List workspace breakpoints with document_url, file name (name), zero-based line, enabled and condition. Returns [] if none exist."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {"properties", QJsonObject{}},
                            {"required", QJsonArray{}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_breakpoint_set"},
                    {
                        "description",
                        "Create or update a breakpoint and return its current settings. New breakpoints are enabled and unconditional by default. Omitted settings on existing breakpoints are preserved."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"document_url", QJsonObject{{"type", "string"}, {"description", "Local file URL of the document."}}},
                                    {"line", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"description", "Zero-based line number as returned by line_get."}}},
                                    {"enabled", QJsonObject{{"type", "boolean"}}},
                                    {"condition", QJsonObject{{"type", "string"}, {"description", "Lua condition. Pass an empty string to clear it."}}}
                                }
                            },
                            {"required", QJsonArray{"document_url", "line"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_breakpoint_delete"},
                    {"description", "Remove a breakpoint. Returns deleted=false if there was no breakpoint at that location."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"document_url", QJsonObject{{"type", "string"}, {"description", "Local file URL of the document."}}},
                                    {"line", QJsonObject{{"type", "integer"}, {"minimum", 0}, {"description", "Zero-based line number as returned by line_get."}}}
                                }
                            },
                            {"required", QJsonArray{"document_url", "line"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_script"},
                    {
                        "description",
                        "Start a Lua script in debug mode. Returns when the started thread first pauses or finishes. Set breakpoints before starting to stop at a specific line."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"document_url", QJsonObject{{"type", "string"}, {"description", "Local file URL of the Lua script."}}}
                                }
                            },
                            {"required", QJsonArray{"document_url"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_threads_get"},
                    {
                        "description",
                        "List active debug threads with thread_id and status (Running, Paused, or Terminating). Paused threads include document_url and zero-based line when their top frame has a source location. Returns [] if none exist."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {"properties", QJsonObject{}},
                            {"required", QJsonArray{}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_call_stack_get"},
                    {
                        "description",
                        "Get the saved call stack for one debug thread, outermost frame first and current frame last. Each frame has kind (Lua, C, or main), name, function_name (empty if unknown), document_url and zero-based line (-1 if unknown). Returns [] when the thread has no snapshot."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"thread_id", QJsonObject{{"type", "string"}, {"description", "Thread ID returned by debug_threads_get."}}}
                                }
                            },
                            {"required", QJsonArray{"thread_id"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_resume"},
                    {"description", "Resume the specified paused debug thread. Returns its next pause location or its final output."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"thread_id", QJsonObject{{"type", "string"}, {"description", "Paused thread ID returned by debug_threads_get."}}}
                                }
                            },
                            {"required", QJsonArray{"thread_id"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_step_over"},
                    {"description", "Step over the current line in the specified paused debug thread, skipping nested calls. Returns its next pause location or its final output."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"thread_id", QJsonObject{{"type", "string"}, {"description", "Paused thread ID returned by debug_threads_get."}}}
                                }
                            },
                            {"required", QJsonArray{"thread_id"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_step_into"},
                    {
                        "description",
                        "Advance the specified paused debug thread to its next executable Lua line, entering a call if encountered. Returns its next pause location or its final output."
                    },
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"thread_id", QJsonObject{{"type", "string"}, {"description", "Paused thread ID returned by debug_threads_get."}}}
                                }
                            },
                            {"required", QJsonArray{"thread_id"}}
                        }
                    }
                }
            }
        },
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "debug_step_out"},
                    {"description", "Run the specified paused debug thread until it returns to a shallower frame. Returns its next pause location or its final output."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {"thread_id", QJsonObject{{"type", "string"}, {"description", "Paused thread ID returned by debug_threads_get."}}}
                                }
                            },
                            {"required", QJsonArray{"thread_id"}}
                        }
                    }
                }
            }
        },
    };
}

void ToolsModule::toolsRegister(const QJsonArray &tools) {
    m_mcpTools = tools;
}

QJsonArray ToolsModule::toolsGet(const QSet<QString> &names, const bool includeMcp) const {
    QJsonArray tools{};
    for (const auto &value: m_tools) {
        const auto name = value.toObject().value("function").toObject().value("name").toString();
        if (names.contains(name)) tools.append(value);
    }
    if (includeMcp) {
        for (const auto &tool: m_mcpTools) tools.append(tool);
    }
    return tools;
}

QJsonArray ToolsModule::schemasGet(const QSet<QString> &names) const {
    QJsonArray schemas{};
    for (const auto &tool: m_tools) {
        auto function = tool.toObject().value("function").toObject();
        const auto name = function.value("name").toString();
        if (!names.contains(name)) continue;
        function["inputSchema"] = function.take("parameters");
        const auto readOnly = !m_writeGroup.contains(name) && !m_fullAccessGroup.contains(name);
        function["annotations"] = QJsonObject{{"readOnlyHint", readOnly}};
        schemas.append(function);
    }
    return schemas;
}

QPair<bool, QString> ToolsModule::toolCall(const int mode, const QString &name, const QString &arguments) const {
    return {permissionGet(mode, name), toolTextGet(name, arguments)};
}

QString ToolsModule::toolTextGet(const QString &name, const QString &arguments) const {
    const auto object = argumentsGet(arguments);
    QString chatText{};
    if (name == "api_list") {
        chatText = "List available APIs";
    } else if (name == "api_get") {
        const auto packageName = object.value("package_name").toString();
        chatText = QString("Read %1 API details").arg(packageName);
    } else if (name == "demo_get") {
        const auto packageName = object.value("package_name").toString();
        chatText = QString("Read %1 demo").arg(packageName);
    } else if (name == "subagent_dispatch") {
        const auto tasks = object.value("tasks").toArray();
        chatText = tasks.size() == 1
                       ? QString("Delegate task to %1 agent").arg(tasks.first().toObject().value("role").toString())
                       : QString("Delegate %1 tasks to subagents").arg(tasks.size());
    } else if (name == "vision_analyze") {
        const auto count = object.value("attachments").toArray().size();
        chatText = count == 1 ? "Analyze image" : QString("Analyze %1 images").arg(count);
    } else if (name == "plan_update") {
        chatText = "Update plan";
    } else if (name == "user_input_request") {
        chatText = object.value("question").toString();
    } else if (name == "port_list") {
        chatText = "List available ports";
    } else if (name == "port_config_get") {
        const auto portType = object.value("port_type").toString();
        chatText = QString("Get %1 port configuration").arg(portType);
    } else if (name == "port_create") {
        const auto portName = object.value("config").toObject().value("portName").toString();
        chatText = QString("Create port %1").arg(portName);
    } else if (name == "port_delete") {
        const auto portName = object.value("port_name").toString();
        chatText = QString("Delete port %1").arg(portName);
    } else if (name == "diagnostics_get") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        chatText = QString("Check diagnostics for %1").arg(documentName);
    } else if (name == "directory_list") {
        const auto url = object.value("directory_url").toString();
        chatText = url.isEmpty() ? "List workspace directory" : QString("List directory %1").arg(QDir(QUrl(url).toLocalFile()).dirName());
    } else if (name == "directory_create") {
        chatText = QString("Create directory %1").arg(QUrl(object.value("directory_url").toString()).fileName());
    } else if (name == "directory_delete") {
        chatText = QString("Delete directory %1").arg(QUrl(object.value("directory_url").toString()).fileName());
    } else if (name == "directory_rename") {
        const auto sourceName = QUrl(object.value("source_url").toString()).fileName();
        const auto targetName = QUrl(object.value("target_url").toString()).fileName();
        chatText = QString("Rename directory %1 to %2").arg(sourceName, targetName);
    } else if (name == "document_create") {
        chatText = QString("Create document %1").arg(QUrl(object.value("document_url").toString()).fileName());
    } else if (name == "document_delete") {
        chatText = QString("Delete document %1").arg(QUrl(object.value("document_url").toString()).fileName());
    } else if (name == "document_rename") {
        const auto sourceName = QUrl(object.value("source_url").toString()).fileName();
        const auto targetName = QUrl(object.value("target_url").toString()).fileName();
        chatText = QString("Rename document %1 to %2").arg(sourceName, targetName);
    } else if (name == "grep_search") {
        const auto pattern = object.value("pattern").toString();
        chatText = QString("Grep \"%1\"").arg(pattern);
    } else if (name == "line_get") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        const auto startLine = object.value("start_line").toInt(-1);
        const auto lineCount = object.value("line_count").toInt(-1);
        chatText = lineCount == -1
                       ? QString("Read %1 from line %2 to the end").arg(documentName, QString::number(startLine))
                       : QString("Read %1 from line %2 (%3 lines)").arg(documentName, QString::number(startLine), QString::number(lineCount));
    } else if (name == "line_set") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        chatText = QString("Apply %1 edits to %2").arg(object.value("edits").toArray().size()).arg(documentName);
    } else if (name == "memory_search") {
        chatText = QString("Search memory for \"%1\"").arg(object.value("query").toString());
    } else if (name == "run_script") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        chatText = QString("Run %1").arg(documentName);
    } else if (name == "debug_breakpoint_list") {
        chatText = "List breakpoints";
    } else if (name == "debug_breakpoint_set") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        const auto line = object.value("line").toInt(-1);
        chatText = QString("Set breakpoint in %1 (line index %2)").arg(documentName, QString::number(line));
    } else if (name == "debug_breakpoint_delete") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        const auto line = object.value("line").toInt(-1);
        chatText = QString("Delete breakpoint in %1 (line index %2)").arg(documentName, QString::number(line));
    } else if (name == "debug_script") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        chatText = QString("Debug %1").arg(documentName);
    } else if (name == "debug_threads_get") {
        chatText = "List debug threads";
    } else if (name == "debug_call_stack_get") {
        chatText = QString("Get call stack for %1").arg(object.value("thread_id").toString());
    } else if (name == "debug_resume") {
        chatText = QString("Resume thread %1").arg(object.value("thread_id").toString());
    } else if (name == "debug_step_over") {
        chatText = QString("Step over in thread %1").arg(object.value("thread_id").toString());
    } else if (name == "debug_step_into") {
        chatText = QString("Step into in thread %1").arg(object.value("thread_id").toString());
    } else if (name == "debug_step_out") {
        chatText = QString("Step out of the current frame in thread %1").arg(object.value("thread_id").toString());
    }
    return chatText.isEmpty() ? name : chatText;
}

QFuture<ToolResult> ToolsModule::toolExecute(const QString &name, const QString &arguments, const QString &runtimeId, const QString &runtimeRole) {
    if (m_mcpModule->toolContains(name)) return m_mcpModule->toolExecute(name, arguments);
    const auto object = argumentsGet(arguments);
    if (name == "vision_analyze") {
        if (runtimeRole != "general") return QtFuture::makeReadyValueFuture(ToolResult{.content = "This agent cannot use the vision agent directly.", .success = false});
        QList<QUrl> attachments{};
        for (const auto &attachment: object.value("attachments").toArray()) attachments.append(QUrl(attachment.toString()));
        if (attachments.isEmpty()) return QtFuture::makeReadyValueFuture(ToolResult{.content = "Vision agent requires at least one attachment.", .success = false});
        auto *worker = g_globalManager->agentModuleGet()->subagentDispatch("vision", object.value("prompt").toString(), attachments);
        if (worker == nullptr) return QtFuture::makeReadyValueFuture(ToolResult{.content = "Vision agent is unavailable or not configured.", .success = false});
        auto promise = QSharedPointer<QPromise<ToolResult> >::create();
        promise->start();
        const auto future = promise->future();
        connect(worker, &RuntimeModule::finishRun, this, [promise](const QString &result, const bool success) {
            promise->addResult(ToolResult{.content = result, .success = success});
            promise->finish();
        });
        return future;
    }
    if (name == "subagent_dispatch") {
        if (runtimeRole != "supervisor") return QtFuture::makeReadyValueFuture(ToolResult{.content = "This agent cannot dispatch subagents.", .success = false});
        const auto tasks = object.value("tasks").toArray();
        if (tasks.isEmpty()) return QtFuture::makeReadyValueFuture(ToolResult{.content = "No subagent tasks were provided.", .success = false});
        auto promise = QSharedPointer<QPromise<ToolResult> >::create();
        auto results = QSharedPointer<QJsonArray>::create();
        auto remaining = QSharedPointer<qsizetype>::create(tasks.size());
        auto success = QSharedPointer<bool>::create(true);
        for (qsizetype index = 0; index < tasks.size(); ++index) results->append(QJsonObject{});
        promise->start();
        const auto future = promise->future();
        const auto finish = [promise, results, remaining, success](const qsizetype index, const QString &role, const QString &result, const bool succeeded) {
            (*results)[index] = QJsonObject{{"role", role}, {"result", result}};
            *success = *success && succeeded;
            if (--*remaining > 0) return;
            promise->addResult(ToolResult{.content = QString::fromUtf8(QJsonDocument(*results).toJson(QJsonDocument::Compact)), .success = *success});
            promise->finish();
        };
        for (qsizetype index = 0; index < tasks.size(); ++index) {
            const auto task = tasks.at(index).toObject();
            const auto role = task.value("role").toString();
            QList<QUrl> attachments{};
            for (const auto &attachment: task.value("attachments").toArray()) attachments.append(QUrl(attachment.toString()));
            if (role == "vision" && attachments.isEmpty()) {
                finish(index, role, "Vision agent requires at least one attachment.", false);
                continue;
            }
            auto *worker = g_globalManager->agentModuleGet()->subagentDispatch(role, task.value("prompt").toString(), attachments);
            if (worker == nullptr) {
                finish(index, role, QString("Subagent role is unavailable or not configured: %1").arg(role), false);
                continue;
            }
            connect(worker, &RuntimeModule::finishRun, this, [finish, index, role](const QString &result, const bool success) {
                finish(index, role, result, success);
            });
        }
        return future;
    }
    if (name == "run_script") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        auto threadId = QSharedPointer<QString>::create();
        auto promise = QSharedPointer<QPromise<ToolResult> >::create();
        auto connection = QSharedPointer<QMetaObject::Connection>::create();
        promise->start();
        const auto future = promise->future();
        *connection = connect(g_globalManager->threadpoolModuleGet(), &ThreadpoolModule::finishThread, this,
                              [threadId, promise, connection](const QString &id, const QJsonObject &output) {
                                  if (id != *threadId) return;
                                  disconnect(*connection);
                                  promise->addResult(ToolResult{
                                      .content = QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact)),
                                      .success = output.value("err").toString().isEmpty()
                                  });
                                  promise->finish();
                              });
        *threadId = g_globalManager->threadpoolModuleGet()->threadStart(documentUrl, RunMode::Run);
        return future;
    }
    // debug toolset
    const auto debugResult = [](const QJsonObject &result) {
        return ToolResult{
            .content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact)),
            .success = !result.contains("error") && result.value("output").toObject().value("err").toString().isEmpty()
        };
    };
    if (name == "debug_script") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        if (!documentUrl.isLocalFile() || !QDir::isAbsolutePath(documentUrl.toLocalFile()))
            return QtFuture::makeReadyValueFuture(ToolResult{.content = "Expected a local document_url.", .success = false});
        if (g_globalManager->documentModuleGet()->lineCountGet(documentUrl) < 0)
            return QtFuture::makeReadyValueFuture(ToolResult{.content = "Script does not exist or is not readable.", .success = false});
        const auto threadId = g_globalManager->threadpoolModuleGet()->threadStart(documentUrl, RunMode::Debug);
        if (threadId.isEmpty()) return QtFuture::makeReadyValueFuture(ToolResult{.content = "Could not start debug thread.", .success = false});
        return g_globalManager->debugModuleGet()->debugWait(threadId).then(this, debugResult);
    }
    InterpreterState::Value state;
    if (name == "debug_resume") state = InterpreterState::Resume;
    else if (name == "debug_step_over") state = InterpreterState::StepOver;
    else if (name == "debug_step_into") state = InterpreterState::StepInto;
    else if (name == "debug_step_out") state = InterpreterState::StepOut;
    else return QtFuture::makeReadyValueFuture(toolExecute(name, object, runtimeId));

    const auto threadId = object.value("thread_id").toString();
    auto *pool = g_globalManager->threadpoolModuleGet();
    const auto thread = pool->threadsGet().value(threadId).toHash();
    if (thread.value("status").value<ThreadStatus::Value>() != ThreadStatus::Paused)
        return QtFuture::makeReadyValueFuture(ToolResult{.content = "Expected a paused debug thread_id.", .success = false});

    auto wait = g_globalManager->debugModuleGet()->debugWait(threadId);
    if (!wait.isFinished()) pool->interpreterStateSet(threadId, state);
    return wait.then(this, debugResult);
}

ToolResult ToolsModule::toolExecute(const QString &name, const QJsonObject &object, const QString &runtimeId) const {
    const QDir uniCommDir(QDir(QCoreApplication::applicationDirPath()).filePath("lua-language-server/meta/3rd/UniComm"));
    const QDir apiDir(uniCommDir.filePath("library"));
    const QDir demoDir(uniCommDir.filePath("demo"));
    // UniComm tools
    if (name == "api_list") {
        QJsonArray array{};
        const auto entries = apiDir.entryInfoList({"*.d.lua"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &entry: entries) {
            const auto packageName = entry.fileName().chopped(QStringLiteral(".d.lua").size());
            if (packageName != "types") {
                array.append(packageName);
            }
        }
        return {.content = QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))};
    }
    if (name == "api_get") {
        const auto packageName = object.value("package_name").toString();
        if (packageName.isEmpty() || packageName.contains('/') || packageName.contains('\\') || packageName.contains("..")) {
            return {.content = "Invalid package name.", .success = false};
        }
        auto file = QFile(apiDir.filePath(packageName + ".d.lua"));
        if (!file.open(QIODevice::ReadOnly)) {
            return {.content = QString("Package '%1' not found.").arg(packageName), .success = false};
        }
        QTextStream stream(&file);
        return {.content = stream.readAll()};
    }
    if (name == "demo_get") {
        const auto packageName = object.value("package_name").toString();
        const auto parts = packageName.split('/');
        if (packageName.isEmpty() || QDir::isAbsolutePath(packageName) || packageName.contains('\\') || packageName.contains(':')
            || parts.contains(QString{}) || parts.contains(".") || parts.contains("..")) {
            return {.content = "Invalid demo path.", .success = false};
        }
        const QFileInfo directoryInfo(demoDir.filePath(packageName));
        const QFileInfo targetInfo(directoryInfo.isDir() ? directoryInfo : QFileInfo(demoDir.filePath(packageName + ".lua")));
        const auto rootPath = demoDir.canonicalPath() + '/';
        const auto targetPath = targetInfo.canonicalFilePath();
        if (targetPath.isEmpty()) return {.content = QString("Demo '%1' not found.").arg(packageName), .success = false};
        if (!targetPath.startsWith(rootPath, Qt::CaseInsensitive)) return {.content = "Invalid demo path.", .success = false};

        if (targetInfo.isDir()) {
            QJsonArray entries{};
            const QDir directory(targetPath);
            for (const auto &entry: directory.entryInfoList({"*.lua"}, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot, QDir::DirsFirst | QDir::Name)) {
                if (!entry.canonicalFilePath().startsWith(rootPath, Qt::CaseInsensitive)) continue;
                entries.append(QJsonObject{
                    {"path", packageName + '/' + (entry.isDir() ? entry.fileName() : entry.fileName().chopped(4))},
                    {"type", entry.isDir() ? "directory" : "file"}
                });
            }
            return {.content = QString::fromUtf8(QJsonDocument(QJsonObject{{"path", packageName}, {"entries", entries}}).toJson(QJsonDocument::Compact))};
        }

        auto file = QFile(targetPath);
        if (!file.open(QIODevice::ReadOnly)) {
            return {.content = QString("Demo '%1' not found.").arg(packageName), .success = false};
        }
        QTextStream stream(&file);
        return {.content = stream.readAll()};
    }
    if (name == "plan_update") {
        if (runtimeId.isEmpty()) return {.content = "Plan update requires an agent runtime.", .success = false};
        if (object.contains("explanation") && !object.value("explanation").isString()) return {.content = "Plan update failed: explanation must be a string.", .success = false};
        if (!object.value("plan").isArray()) return {.content = "Plan update failed: plan must be an array.", .success = false};

        QJsonArray normalizedSteps{};
        auto inProgressCount = 0;
        auto completedCount = 0;
        for (const auto &value: object.value("plan").toArray()) {
            if (!value.isObject()) return {.content = "Plan update failed: every plan item must be an object.", .success = false};

            const auto stepObject = value.toObject();
            const auto description = stepObject.value("step").toString().trimmed();
            const auto status = stepObject.value("status").toString();
            if (description.isEmpty()) return {.content = "Plan update failed: step must be a non-empty string.", .success = false};
            if (status == "in_progress") ++inProgressCount;
            else if (status == "completed") ++completedCount;
            else if (status != "pending") return {.content = "Plan update failed: status must be pending, in_progress, or completed.", .success = false};

            normalizedSteps.append(QJsonObject{
                {"step", description},
                {"status", status}
            });
        }
        if (normalizedSteps.isEmpty()) return {.content = "Plan update failed: plan must contain at least one step.", .success = false};
        if (inProgressCount > 1) return {.content = "Plan update failed: at most one step can be in progress.", .success = false};

        const QJsonObject plan{
            {"explanation", object.value("explanation").toString()},
            {"plan", normalizedSteps}
        };
        g_globalManager->agentModuleGet()->planUpdate(runtimeId, plan);
        return {.content = QString("Plan updated: %1/%2 steps completed.").arg(completedCount).arg(normalizedSteps.size())};
    }
    if (name == "port_list") {
        QJsonArray ports{};
        for (const auto &portName: g_globalManager->portModuleGet()->portList()) ports.append(portName);
        return {.content = QString::fromUtf8(QJsonDocument(ports).toJson(QJsonDocument::Compact))};
    }
    if (name == "port_config_get") {
        const auto portType = m_portTypes.value(object.value("port_type").toString(), static_cast<PortType::Value>(-1));
        const auto config = PortModule::portConfigGet(portType);
        if (config.isEmpty()) return {.content = "Unsupported port type.", .success = false};
        return {.content = QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact))};
    }
    if (name == "port_create") {
        const auto portType = m_portTypes.value(object.value("port_type").toString(), static_cast<PortType::Value>(-1));
        auto config = object.value("config").toObject();
        config["portType"] = portType;
        const auto portName = config.value("portName").toString();
        const auto undoGroupId = runtimeId.isEmpty() ? QString{} : g_globalManager->agentModuleGet()->undoGroupIdGet();
        const auto error = g_globalManager->portModuleGet()->portInsert(-1, config, undoGroupId);
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Port '%1' created.").arg(portName)};
    }
    if (name == "port_delete") {
        const auto portName = object.value("port_name").toString();
        const auto undoGroupId = runtimeId.isEmpty() ? QString{} : g_globalManager->agentModuleGet()->undoGroupIdGet();
        const auto error = g_globalManager->portModuleGet()->portRemove(portName, undoGroupId);
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Port '%1' deleted.").arg(portName)};
    }
    if (name == "diagnostics_get") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto diagnostics = g_globalManager->documentModuleGet()->diagnosticsGet(documentUrl);
        return {.content = QString::fromUtf8(QJsonDocument(diagnostics).toJson(QJsonDocument::Compact))};
    }
    if (name == "directory_list") {
        const auto url = object.value("directory_url").toString();
        const auto directoryUrl = url.isEmpty() ? g_workspaceUrl : QUrl(url);
        if (!QFileInfo(directoryUrl.toLocalFile()).isDir()) return {.content = "Directory list failed: directory does not exist.", .success = false};
        return {.content = QString::fromUtf8(QJsonDocument(g_globalManager->documentModuleGet()->directoryList(directoryUrl)).toJson(QJsonDocument::Compact))};
    }
    if (name == "directory_create") {
        const auto directoryUrl = QUrl(object.value("directory_url").toString());
        const auto error = g_globalManager->documentModuleGet()->directoryCreate(directoryUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Directory created: %1").arg(directoryUrl.toString())};
    }
    if (name == "directory_delete") {
        const auto directoryUrl = QUrl(object.value("directory_url").toString());
        const auto error = g_globalManager->documentModuleGet()->directoryDelete(directoryUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Directory deleted: %1").arg(directoryUrl.toString())};
    }
    if (name == "directory_rename") {
        const auto sourceUrl = QUrl(object.value("source_url").toString());
        const auto targetUrl = QUrl(object.value("target_url").toString());
        const auto error = g_globalManager->documentModuleGet()->directoryRename(sourceUrl, targetUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Directory renamed: %1").arg(targetUrl.toString())};
    }
    if (name == "document_create") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto error = g_globalManager->documentModuleGet()->documentCreate(documentUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Document created: %1").arg(documentUrl.toString())};
    }
    if (name == "document_delete") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto error = g_globalManager->documentModuleGet()->documentDelete(documentUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Document deleted: %1").arg(documentUrl.toString())};
    }
    if (name == "document_rename") {
        const auto sourceUrl = QUrl(object.value("source_url").toString());
        const auto targetUrl = QUrl(object.value("target_url").toString());
        const auto error = g_globalManager->documentModuleGet()->documentRename(sourceUrl, targetUrl, g_globalManager->agentModuleGet()->undoGroupIdGet());
        if (!error.isEmpty()) return {.content = error, .success = false};
        return {.content = QString("Document renamed: %1").arg(targetUrl.toString())};
    }
    if (name == "grep_search") {
        const auto pattern = object.value("pattern").toString();
        const auto result = g_globalManager->serviceModuleGet()->grepServiceGet()->grep(pattern);
        return {.content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "line_get") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto documentInfo = QFileInfo(documentUrl.toLocalFile());
        if (!documentInfo.isFile()) return {.content = "Line get failed: document does not exist.", .success = false};
        const auto startLine = object.value("start_line").toInt();
        const auto lineCount = object.value("line_count").toInt();
        if (startLine < 0) return {.content = "Line get failed: start_line is out of range.", .success = false};
        if (lineCount == 0 || lineCount < -1) return {.content = "Line get failed: line_count is out of range.", .success = false};
        if (documentInfo.size() == 0 && startLine == 0) return {.content = QString("")};

        const auto text = g_globalManager->documentModuleGet()->linesGet(documentUrl, startLine, lineCount);
        if (text.isNull()) return {.content = "Line get failed: start_line is out of range.", .success = false};
        auto lines = text.split('\n', Qt::KeepEmptyParts);
        for (qsizetype i = 0; i < lines.size(); ++i) lines[i].prepend(QString::number(startLine + i) + "|");
        return {.content = lines.join('\n')};
    }
    if (name == "line_set") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto documentInfo = QFileInfo(documentUrl.toLocalFile());
        if (!documentInfo.isFile()) return {.content = "Line set failed: document does not exist.", .success = false};
        QList<qsizetype> accepted{};
        QJsonArray applied{};
        QJsonArray rejected{};
        const auto edits = object.value("edits").toArray();
        for (qsizetype index = 0; index < edits.size(); ++index) {
            const auto edit = edits.at(index).toObject();
            const auto startLine = edit.value("start_line").toInt();
            const auto lineCount = edit.value("line_count").toInt();
            QString reason{};
            if (startLine < 0) reason = "start_line_out_of_range";
            else if (lineCount == 0 || lineCount < -1) reason = "line_count_out_of_range";
            else if (g_globalManager->documentModuleGet()->linesGet(documentUrl, startLine, 1).trimmed() != edit.value("expected").toString().section('\n', 0, 0).trimmed())
                reason = "document_changed";

            if (!reason.isEmpty()) {
                rejected.append(QJsonObject{{"index", index}, {"start_line", startLine}, {"reason", reason}});
                continue;
            }
            accepted.append(index);
            applied.append(index);
        }
        std::ranges::sort(accepted, [&edits](const qsizetype left, const qsizetype right) {
            return edits.at(left).toObject().value("start_line").toInt() > edits.at(right).toObject().value("start_line").toInt();
        });
        QStringList texts{};
        QList<int> startLines{};
        QList<int> lineCounts{};
        for (const auto index: accepted) {
            const auto edit = edits.at(index).toObject();
            texts.append(edit.value("text").toString());
            startLines.append(edit.value("start_line").toInt());
            lineCounts.append(edit.value("line_count").toInt());
        }
        if (!texts.isEmpty()) {
            const auto error = g_globalManager->documentModuleGet()->linesSet(documentUrl, texts, startLines, lineCounts, g_globalManager->agentModuleGet()->undoGroupIdGet());
            if (!error.isEmpty()) return {.content = error, .success = false};
        }
        return {
            .content = QString::fromUtf8(QJsonDocument(QJsonObject{{"applied", applied}, {"rejected", rejected}}).toJson(QJsonDocument::Compact)),
            .success = rejected.isEmpty()
        };
    }
    if (name == "memory_search") {
        const auto limit = qBound(1, object.value("limit").toInt(3), 5);
        const auto results = m_sqlModule->conversationsSearch(object.value("query").toString(), limit);
        QJsonArray array{};
        for (const auto &result: results) {
            QJsonArray messages{};
            for (const auto &message: result.messages) {
                if (message.content.isEmpty()) continue;
                messages.append(QJsonObject{
                    {"role", message.role},
                    {"content", message.content}
                });
            }
            array.append(QJsonObject{
                {"conversation_id", result.conversationId},
                {"conversation_title", result.conversationTitle},
                {"turn_id", result.turnId},
                {"created_at", result.createdAt},
                {"rank", result.rank},
                {"messages", messages}
            });
        }
        const auto json = QJsonDocument(array);
        return {.content = QString::fromUtf8(json.toJson(QJsonDocument::Compact))};
    }
    // debug toolset
    if (name == "debug_threads_get") {
        const auto result = g_globalManager->debugModuleGet()->threadsGet();
        return {.content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "debug_call_stack_get") {
        const auto threadId = object.value("thread_id").toString();
        if (threadId.isEmpty()) return {.content = "Expected a non-empty thread_id.", .success = false};
        const auto frames = g_globalManager->debugModuleGet()->callStackGet(threadId);
        QJsonArray result{};
        for (const auto &frame: frames) {
            auto row = QJsonObject::fromVariantHash(frame.toHash());
            row["document_url"] = row.take("documentUrl");
            row["function_name"] = row.take("functionName");
            result.append(row);
        }
        return {.content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "debug_breakpoint_list") {
        QJsonArray result{};
        const auto rows = g_globalManager->breakpointModuleGet()->breakpointsGet();
        for (const auto &row: rows) {
            auto breakpoint = QJsonObject::fromVariantHash(row.toHash());
            breakpoint["document_url"] = breakpoint.take("documentUrl");
            result.append(breakpoint);
        }
        return {.content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "debug_breakpoint_set") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto line = object.value("line").toInt(-1);
        if (!documentUrl.isValid() || !documentUrl.isLocalFile() || !QDir::isAbsolutePath(documentUrl.toLocalFile())) return {
            .content = "Expected a local document_url.", .success = false
        };
        if (line < 0) return {.content = "Expected a zero-based integer line number.", .success = false};
        const auto lineCount = g_globalManager->documentModuleGet()->lineCountGet(documentUrl);
        if (lineCount < 0) return {.content = "Breakpoint set failed: document does not exist or is not readable.", .success = false};
        if (line >= lineCount) return {.content = QString("Breakpoint set failed: line %1 is out of range (0..%2).").arg(line).arg(lineCount - 1), .success = false};

        QVariantHash settings{};
        if (object.contains("enabled")) {
            if (!object.value("enabled").isBool()) return {.content = "Expected a boolean enabled.", .success = false};
            settings.insert("enabled", object.value("enabled").toBool());
        }
        if (object.contains("condition")) {
            if (!object.value("condition").isString()) return {.content = "Expected a string condition.", .success = false};
            settings.insert("condition", object.value("condition").toString());
        }
        auto *module = g_globalManager->breakpointModuleGet();
        if (module->breakpointGet(documentUrl, line).isEmpty()) module->breakpointAdd(documentUrl, line);
        if (!settings.isEmpty()) module->breakpointConfigure(documentUrl, line, settings);
        auto result = QJsonObject::fromVariantHash(module->breakpointGet(documentUrl, line));
        result["document_url"] = documentUrl.toString();
        result["line"] = line;
        return {.content = QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "debug_breakpoint_delete") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto line = object.value("line").toInt(-1);
        if (!documentUrl.isValid() || !documentUrl.isLocalFile() || !QDir::isAbsolutePath(documentUrl.toLocalFile())) return {
            .content = "Expected a local document_url.", .success = false
        };
        if (line < 0) return {.content = "Expected a zero-based integer line number.", .success = false};

        auto *module = g_globalManager->breakpointModuleGet();
        const auto exists = !module->breakpointGet(documentUrl, line).isEmpty();
        if (exists) module->breakpointDelete(documentUrl, line);
        return {.content = QString::fromUtf8(QJsonDocument(QJsonObject{{"deleted", exists}}).toJson(QJsonDocument::Compact))};
    }
    return {.content = "Unknown tool.", .success = false};
}

// private
QJsonObject ToolsModule::argumentsGet(const QString &arguments) {
    auto object = QJsonDocument::fromJson(arguments.toUtf8()).object();
    if (object.size() == 1 && object.value("arguments").isObject()) return object.value("arguments").toObject();
    return object;
}

bool ToolsModule::permissionGet(const int mode, const QString &name) const {
    if (m_mcpModule->toolContains(name)) {
        return mode != RuntimeModule::AgentMode::Chat && (m_mcpModule->toolReadOnly(name) || mode == RuntimeModule::AgentMode::FullAccess);
    }
    switch (mode) {
        case RuntimeModule::AgentMode::Chat: return false;
        case RuntimeModule::AgentMode::Read: return !m_writeGroup.contains(name) && !m_fullAccessGroup.contains(name);
        case RuntimeModule::AgentMode::Write: return !m_fullAccessGroup.contains(name);
        case RuntimeModule::AgentMode::FullAccess: return true;
        default: return false;
    }
}
