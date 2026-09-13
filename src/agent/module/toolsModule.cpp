#include "agent/module/toolsModule.h"

#include <QDir>
#include <QJsonDocument>
#include <QPromise>
#include <QSharedPointer>
#include <QUrl>

#include "globals.h"
#include "agent/agentModule.h"
#include "agent/module/mcpModule.h"
#include "agent/module/sqlModule.h"
#include "agent/runtime/runtimeModule.h"
#include "data/databaseModule.h"
#include "data/datatableModule.h"
#include "document/documentModule.h"
#include "port/portModule.h"
#include "runtime/threadpoolModule.h"
#include "service/ripgrep.h"

// public
ToolsModule::ToolsModule(McpModule *mcpModule, SqlModule *sqlModule, QObject *parent)
    : QObject(parent),
      m_portTypes{
          {"serial_port", PortType::SerialPort},
          {"vision", PortType::Vision},
          {"tcp_client", PortType::TcpClient},
          {"tcp_server", PortType::TcpServer},
          {"ssl_client", PortType::SslClient},
          {"ssl_server", PortType::SslServer}
      },
      m_mcpModule(mcpModule),
      m_sqlModule(sqlModule),
      m_writeGroup{"database_create", "datatable_create", "directory_create", "directory_rename", "document_create", "document_rename", "line_set", "port_create"},
      m_fullAccessGroup{"database_delete", "datatable_delete", "directory_delete", "document_delete", "port_delete", "script_exec"} {
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
                    {"description", "Get the detailed API demo for a specific package."},
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
        // databaseList
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "database_list"},
                    {"description", "Get the list of all available database keys that can be queried for detailed annotations."},
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
        // databaseCreate
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "database_create"},
                    {"description", "Create a database entry with a unique key."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "key", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The unique key for the new database entry."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"key"}}
                        }
                    }
                }
            }
        },
        // databaseDelete
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "database_delete"},
                    {"description", "Delete an existing database entry by key. Call database_list first to get the available keys."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "key", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The database key to delete."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"key"}}
                        }
                    }
                }
            }
        },
        // datatableList
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "datatable_list"},
                    {"description", "Get the list of all available data table keys that can be queried for detailed annotations."},
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
        // datatableCreate
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "datatable_create"},
                    {"description", "Create a data table column with a unique key."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "key", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The unique key for the new data table column."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"key"}}
                        }
                    }
                }
            }
        },
        // datatableDelete
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "datatable_delete"},
                    {"description", "Delete an existing data table column by key. Call datatable_list first to get the available keys."},
                    {
                        "parameters", QJsonObject{
                            {"type", "object"},
                            {
                                "properties", QJsonObject{
                                    {
                                        "key", QJsonObject{
                                            {"type", "string"},
                                            {"description", "The data table key to delete."}
                                        }
                                    }
                                }
                            },
                            {"required", QJsonArray{"key"}}
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
                                                                    {"enum", QJsonArray{"data", "hardware", "software", "vision"}},
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
                                            {"enum", QJsonArray{"serial_port", "vision", "tcp_client", "tcp_server", "ssl_client", "ssl_server"}},
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
                                            {"enum", QJsonArray{"serial_port", "vision", "tcp_client", "tcp_server", "ssl_client", "ssl_server"}},
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
                    {"description", "Search text across files in the current workspace."},
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
        // scriptExec
        QJsonObject{
            {"type", "function"},
            {
                "function", QJsonObject{
                    {"name", "script_exec"},
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
    } else if (name == "grep_search") {
        const auto pattern = object.value("pattern").toString();
        chatText = QString("Grep \"%1\"").arg(pattern);
    } else if (name == "database_list") {
        chatText = "List available databases";
    } else if (name == "database_create") {
        chatText = QString("Create database %1").arg(object.value("key").toString());
    } else if (name == "database_delete") {
        chatText = QString("Delete database %1").arg(object.value("key").toString());
    } else if (name == "datatable_list") {
        chatText = "List available datatables";
    } else if (name == "datatable_create") {
        chatText = QString("Create data table %1").arg(object.value("key").toString());
    } else if (name == "datatable_delete") {
        chatText = QString("Delete data table %1").arg(object.value("key").toString());
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
    } else if (name == "log_get") {
        const auto blockCount = object.value("block_count").toInt(-1);
        chatText = QString("Get last %1 log blocks").arg(QString::number(blockCount));
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
    } else if (name == "script_exec") {
        const auto documentName = QUrl(object.value("document_url").toString()).fileName();
        chatText = QString("Run %1").arg(documentName);
    }
    return chatText.isEmpty() ? name : chatText;
}

QFuture<ToolResult> ToolsModule::toolExecute(const QString &runtimeId, const QString &runtimeRole, const QString &name, const QString &arguments) {
    if (m_mcpModule->toolContains(name)) return m_mcpModule->toolExecute(name, arguments);
    const auto object = argumentsGet(arguments);
    if (name == "vision_analyze") {
        if (runtimeRole != "general") return QtFuture::makeReadyValueFuture(ToolResult{"This agent cannot use the vision agent directly.", false});
        QList<QUrl> attachments{};
        for (const auto &attachment: object.value("attachments").toArray()) attachments.append(QUrl(attachment.toString()));
        if (attachments.isEmpty()) return QtFuture::makeReadyValueFuture(ToolResult{"Vision agent requires at least one attachment.", false});
        auto *worker = g_agent->subagentDispatch("vision", object.value("prompt").toString(), attachments);
        if (worker == nullptr) return QtFuture::makeReadyValueFuture(ToolResult{"Vision agent is unavailable or not configured.", false});
        auto promise = QSharedPointer<QPromise<ToolResult> >::create();
        promise->start();
        const auto future = promise->future();
        connect(worker, &RuntimeModule::finishRun, this, [promise](const QString &result, const bool success) {
            promise->addResult(ToolResult{result, success});
            promise->finish();
        });
        return future;
    }
    if (name == "subagent_dispatch") {
        if (runtimeRole != "supervisor") return QtFuture::makeReadyValueFuture(ToolResult{"This agent cannot dispatch subagents.", false});
        const auto tasks = object.value("tasks").toArray();
        if (tasks.isEmpty()) return QtFuture::makeReadyValueFuture(ToolResult{"No subagent tasks were provided.", false});
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
            promise->addResult(ToolResult{QString::fromUtf8(QJsonDocument(*results).toJson(QJsonDocument::Compact)), *success});
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
            auto *worker = g_agent->subagentDispatch(role, task.value("prompt").toString(), attachments);
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
    if (name == "script_exec") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        auto threadId = QSharedPointer<QString>::create();
        auto promise = QSharedPointer<QPromise<ToolResult> >::create();
        auto connection = QSharedPointer<QMetaObject::Connection>::create();
        promise->start();
        const auto future = promise->future();
        *connection = connect(g_threadpool, &ThreadpoolModule::finishThread, this, [threadId, promise, connection](const QString &id, const QJsonObject &output) {
            if (id != *threadId) return;
            disconnect(*connection);
            promise->addResult(ToolResult{
                QString::fromUtf8(QJsonDocument(output).toJson(QJsonDocument::Compact)),
                output.value("err").toString().isEmpty()
            });
            promise->finish();
        });
        g_threadpool->threadStart(documentUrl, InterpreterMode::Agent, *threadId);
        return future;
    }
    return QtFuture::makeReadyValueFuture(_toolExecute(runtimeId, name, object));
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

ToolResult ToolsModule::_toolExecute(const QString &runtimeId, const QString &name, const QJsonObject &object) const {
    const QDir uniCommDir(QDir(QCoreApplication::applicationDirPath()).filePath("lua-language-server/meta/3rd/UniComm"));
    const QDir apiDir(uniCommDir.filePath("library"));
    const QDir demoDir(uniCommDir.filePath("demo"));
    // UniComm tools
    if (name == "api_list") {
        QJsonArray array{};
        const auto entries = apiDir.entryInfoList({"*.d.lua"}, QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
        for (const auto &entry: entries) {
            const auto packageName = entry.fileName().chopped(QStringLiteral(".d.lua").size());
            if (!QStringList({"mqtt", "types"}).contains(packageName)) {
                array.append(packageName);
            }
        }
        return {QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))};
    }
    if (name == "api_get") {
        const auto packageName = object.value("package_name").toString();
        if (packageName.isEmpty() || packageName.contains('/') || packageName.contains('\\') || packageName.contains("..")) {
            return {"Invalid package name.", false};
        }
        auto file = QFile(apiDir.filePath(packageName + ".d.lua"));
        if (!file.open(QIODevice::ReadOnly)) {
            return {QString("Package '%1' not found.").arg(packageName), false};
        }
        QTextStream stream(&file);
        return {stream.readAll()};
    }
    if (name == "demo_get") {
        const auto packageName = object.value("package_name").toString();
        if (packageName.isEmpty() || packageName.contains('/') || packageName.contains('\\') || packageName.contains("..")) {
            return {"Invalid package name.", false};
        }
        auto file = QFile(demoDir.filePath(packageName + ".lua"));
        if (!file.open(QIODevice::ReadOnly)) {
            return {QString("Demo '%1' not found.").arg(packageName), false};
        }
        QTextStream stream(&file);
        return {stream.readAll()};
    }
    if (name == "database_list") {
        const auto keys = g_database->databaseList();
        QJsonArray array{};
        for (const auto &key: keys) {
            array.append(key);
        }
        return {QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))};
    }
    if (name == "database_create") {
        const auto key = object.value("key").toString().trimmed();
        if (key.isEmpty()) return {"Database create failed: key cannot be empty.", false};
        const auto error = g_database->databaseInsert(key, {}, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Database '%1' created.").arg(key)};
    }
    if (name == "database_delete") {
        const auto key = object.value("key").toString().trimmed();
        const auto error = g_database->databaseRemove(key, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Database '%1' deleted.").arg(key)};
    }
    if (name == "datatable_list") {
        const auto keys = g_datatable->datatableList();
        QJsonArray array{};
        for (const auto &key: keys) {
            array.append(key);
        }
        return {QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))};
    }
    if (name == "datatable_create") {
        const auto key = object.value("key").toString().trimmed();
        if (key.isEmpty()) return {"Data table create failed: key cannot be empty.", false};
        const auto error = g_datatable->datatableInsert(key, {}, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Data table '%1' created.").arg(key)};
    }
    if (name == "datatable_delete") {
        const auto key = object.value("key").toString().trimmed();
        const auto error = g_datatable->datatableRemove(key, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Data table '%1' deleted.").arg(key)};
    }
    if (name == "plan_update") {
        if (object.contains("explanation") && !object.value("explanation").isString()) return {"Plan update failed: explanation must be a string.", false};
        if (!object.value("plan").isArray()) return {"Plan update failed: plan must be an array.", false};

        QJsonArray normalizedSteps{};
        auto inProgressCount = 0;
        auto completedCount = 0;
        for (const auto &value: object.value("plan").toArray()) {
            if (!value.isObject()) return {"Plan update failed: every plan item must be an object.", false};

            const auto stepObject = value.toObject();
            const auto description = stepObject.value("step").toString().trimmed();
            const auto status = stepObject.value("status").toString();
            if (description.isEmpty()) return {"Plan update failed: step must be a non-empty string.", false};
            if (status == "in_progress") ++inProgressCount;
            else if (status == "completed") ++completedCount;
            else if (status != "pending") return {"Plan update failed: status must be pending, in_progress, or completed.", false};

            normalizedSteps.append(QJsonObject{
                {"step", description},
                {"status", status}
            });
        }
        if (normalizedSteps.isEmpty()) return {"Plan update failed: plan must contain at least one step.", false};
        if (inProgressCount > 1) return {"Plan update failed: at most one step can be in progress.", false};

        const QJsonObject plan{
            {"explanation", object.value("explanation").toString()},
            {"plan", normalizedSteps}
        };
        g_agent->planUpdate(runtimeId, plan);
        return {QString("Plan updated: %1/%2 steps completed.").arg(completedCount).arg(normalizedSteps.size())};
    }
    if (name == "port_list") {
        const auto keys = g_port->portList();
        QJsonArray array{};
        for (const auto &key: keys) {
            array.append(key);
        }
        return {QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact))};
    }
    if (name == "port_config_get") {
        const auto portType = m_portTypes.value(object.value("port_type").toString(), -1);
        const auto config = PortModule::portConfigGet(portType);
        if (config.isEmpty()) return {"Unsupported port type.", false};
        return {QString::fromUtf8(QJsonDocument(config).toJson(QJsonDocument::Compact))};
    }
    if (name == "port_create") {
        const auto portType = m_portTypes.value(object.value("port_type").toString(), -1);
        auto config = object.value("config").toObject();
        config["portType"] = portType;
        const auto portName = config.value("portName").toString().trimmed();
        const auto error = g_port->portInsert(-1, config, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Port '%1' created.").arg(portName)};
    }
    if (name == "port_delete") {
        const auto portName = object.value("port_name").toString().trimmed();
        const auto error = g_port->portRemove(portName, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Port '%1' deleted.").arg(portName)};
    }
    if (name == "diagnostics_get") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto diagnostics = g_document->diagnosticsGet(documentUrl);
        return {QString::fromUtf8(QJsonDocument(diagnostics).toJson(QJsonDocument::Compact))};
    }
    if (name == "directory_list") {
        const auto url = object.value("directory_url").toString();
        const auto directoryUrl = url.isEmpty() ? g_workspaceUrl : QUrl(url);
        if (!QFileInfo(directoryUrl.toLocalFile()).isDir()) return {"Directory list failed: directory does not exist.", false};
        return {QString::fromUtf8(QJsonDocument(g_document->directoryList(directoryUrl)).toJson(QJsonDocument::Compact))};
    }
    if (name == "directory_create") {
        const auto directoryUrl = QUrl(object.value("directory_url").toString());
        const auto error = g_document->directoryCreate(directoryUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Directory created: %1").arg(directoryUrl.toString())};
    }
    if (name == "directory_delete") {
        const auto directoryUrl = QUrl(object.value("directory_url").toString());
        const auto error = g_document->directoryDelete(directoryUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Directory deleted: %1").arg(directoryUrl.toString())};
    }
    if (name == "directory_rename") {
        const auto sourceUrl = QUrl(object.value("source_url").toString());
        const auto targetUrl = QUrl(object.value("target_url").toString());
        const auto error = g_document->directoryRename(sourceUrl, targetUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Directory renamed: %1").arg(targetUrl.toString())};
    }
    if (name == "document_create") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto error = g_document->documentCreate(documentUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Document created: %1").arg(documentUrl.toString())};
    }
    if (name == "document_delete") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto error = g_document->documentDelete(documentUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Document deleted: %1").arg(documentUrl.toString())};
    }
    if (name == "document_rename") {
        const auto sourceUrl = QUrl(object.value("source_url").toString());
        const auto targetUrl = QUrl(object.value("target_url").toString());
        const auto error = g_document->documentRename(sourceUrl, targetUrl, g_agent->undoGroupIdGet());
        if (!error.isEmpty()) return {error, false};
        return {QString("Document renamed: %1").arg(targetUrl.toString())};
    }
    if (name == "grep_search") {
        const auto pattern = object.value("pattern").toString();
        const auto result = g_ripgrep->grep(pattern);
        return {QString::fromUtf8(QJsonDocument(result).toJson(QJsonDocument::Compact))};
    }
    if (name == "line_get") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto documentInfo = QFileInfo(documentUrl.toLocalFile());
        if (!documentInfo.isFile()) return {"Line get failed: document does not exist.", false};
        const auto startLine = object.value("start_line").toInt();
        const auto lineCount = object.value("line_count").toInt();
        if (startLine < 0) return {"Line get failed: start_line is out of range.", false};
        if (lineCount == 0 || lineCount < -1) return {"Line get failed: line_count is out of range.", false};

        const auto text = g_document->linesGet(documentUrl, startLine, lineCount);
        if (text.isNull()) return {"Line get failed: start_line is out of range.", false};
        auto lines = text.split('\n', Qt::KeepEmptyParts);
        for (qsizetype i = 0; i < lines.size(); ++i) lines[i].prepend(QString::number(startLine + i) + "|");
        return {lines.join('\n')};
    }
    if (name == "line_set") {
        const auto documentUrl = QUrl(object.value("document_url").toString());
        const auto documentInfo = QFileInfo(documentUrl.toLocalFile());
        if (!documentInfo.isFile()) return {"Line set failed: document does not exist.", false};
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
            else if (g_document->linesGet(documentUrl, startLine, 1).trimmed() != edit.value("expected").toString().section('\n', 0, 0).trimmed()) reason = "document_changed";

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
            const auto error = g_document->linesSet(documentUrl, texts, startLines, lineCounts, g_agent->undoGroupIdGet());
            if (!error.isEmpty()) return {error, false};
        }
        return {
            QString::fromUtf8(QJsonDocument(QJsonObject{{"applied", applied}, {"rejected", rejected}}).toJson(QJsonDocument::Compact)),
            rejected.isEmpty()
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
        return {QString::fromUtf8(json.toJson(QJsonDocument::Compact))};
    }
    return {"Unknown tool.", false};
}
