# v0.3.2

## Highlights

- Added a local MCP server so external clients can discover UniComm APIs, manage ports, inspect diagnostics, and run Lua scripts.
- Expanded communication workflows with MQTT, implicit FTPS, rebuilt Bluetooth LE support, and configurable TLS for SSL and WebSocket ports.
- Added script-owned UI windows for prompts, file selection, images, key-value data, tables, and plots.
- Added a vision agent for image analysis and visual verification, with dedicated model selection.
- Added a unified Services panel for MCP, language servers, search, and network activity.
- Improved terminal selection, clipboard actions, hyperlinks, and editor assistance.

## Ports and Protocols

- Reworked Bluetooth LE discovery, characteristic and descriptor reads/writes, notification subscriptions, and per-characteristic receive buffers.
- Reworked WebSocket client/server handling and added configurable WS/WSS operation.
- Added persisted TLS settings for CA certificates, certificate chains, private keys, and key passwords, including optional client-certificate verification on servers.
- Added MQTT 3.1.1 connect, subscribe, publish, consume, and disconnect workflows. Publishing and consuming currently support QoS 0; automatic keep-alive heartbeats are not provided.
- Added implicit FTPS with protected data connections and improved FTP reply handling and compatibility fallbacks.
- Exposed UDP socket configuration to Lua and Agent tools.
- Added non-consuming port reads through `peek` and `peekUntil`, and improved buffered protocol reads and HTTP streaming.
- Expanded vision-port access from Lua and Agent tools, including OCR results, and increased the receive-buffer size limit to 64 MiB.
- Reorganized port examples by transport and expanded API annotations.

## Lua and Script UI

- Added `ui.message`, `ui.input`, and `ui.file` for script-driven user interaction.
- Added independent `ui.image`, `ui.database`, `ui.table`, and `ui.plot` windows, with handles for updating and closing them; tables also support export.
- Added function-based `thread.start` with copied captures and independent interpreter globals.
- Added JSON conversion, hashing, and encryption/decryption helpers to the `string` API.
- Expanded keyboard and mouse automation with press/release, position, movement, button selection, and scrolling operations.

## Agent

- Added the vision role and `vision_analyze` tool, with image attachments passed to the selected vision model.
- Added model input-modality display and separate primary, subagent, and vision model selection.
- Improved conversation rendering, attachment history, follow-tail controls, and Markdown tables with copy actions.
- Reworked conversation, turn, message, and tool models and simplified active terminal presentation.
- Made Agent `line_set`, undo, and redo operations save document changes to disk.

## Services

- Added a loopback MCP endpoint at `http://127.0.0.1:32167/mcp` exposing API/demo lookup, port management, diagnostics, and script execution tools.
- Added service status, uptime, event details, activity indicators, and context-menu actions.
- Added network request monitoring with active-request counts, request sources, HTTP status codes, and errors.

## Terminal and Editor

- Added terminal drag selection, double-click word selection, and selection handling across wrapped lines.
- Improved terminal copy/paste menus, bracketed paste, and OSC 8 hyperlink handling.
- Reworked completion, symbol navigation, and signature widgets, including symbol colors and theme integration.
- Refined diagnostics and port settings interfaces, including clearable, revealable, and file-browsing fields.

## Fixes and Dependencies

- Fixed delayed Scintilla callbacks accessing destroyed widgets.
- Fixed WebView native-window leakage outside its host area and unloaded QML views during shutdown.
- Fixed FTP handling of non-standard PWD responses and the MLST-to-SIZE fallback in `exists`.
- Fixed SSE parsing and moved receive-buffer clearing from port close to port open.
- Updated SimpleBLE from 1.0.0 to 1.4.0 and Lua Language Server from 3.18.2 to 3.19.2.

## API Migration

- Replace `io.message` with `ui.message`; review the new title, text, and button arguments.
- Migrate the removed docked database, data-table, and plot workflows to script-owned `ui.database`, `ui.table`, and `ui.plot` windows. The old database/data-table Agent management tools and dedicated data-agent role were removed.
- Replace `thread.start(path)` with `thread.start(function() ... end)`; captured values are copied, and globals are not shared between interpreters.
- Replace `mouse.rightClick(x, y)` with `mouse.click(x, y, "right")`.
- Review existing vision and BLE port scripts against the updated port annotations and transport-specific examples.

# v0.3.1

## Highlights

- Expanded the Agent into an observable end-to-end workflow from workspace inspection and Lua generation to permission-gated execution, tracing, and evaluation.
- Added MCP integration and configurable OpenAI-compatible providers.
- Added transactional Agent changes backed by the global undo/redo stack across documents, databases, data tables, and ports.
- Added runtime steering, lifecycle hooks, retry handling, and configurable execution guardrails.
- Reworked the Agent interface for clearer planning, change review, streaming responses, and execution activity.

## Agent Runtime

- Added custom provider creation and editing with configurable base, chat, and model endpoints.
- Added MCP server discovery, tool registration, execution, and compatibility fallback between stateless and stateful protocol variants.
- Added structured tool-result status propagation across built-in, MCP, Lua, and subagent tools.
- Added configurable limits for context compaction, consecutive tool failures, planning cadence, and total tool calls.
- Added in-turn steering that queues additional user guidance for the next model request.
- Added Lua lifecycle hooks for turn start, turn finish, permission requests, and user input requests.
- Added transient network retries with visible reconnect progress for primary and subagent runs.
- Added text and image attachments to Agent requests.
- Added a dedicated data-agent role and improved subagent result aggregation.
- Added persisted execution evaluation for turn status, timing, time to first token, token usage, model calls, and tool calls.

## Workspace Changes

- Added Agent change transactions with grouped undo, redo, rollback, and document commit handling.
- Extended undo-backed operations to documents, databases, data tables, and ports.
- Added LSP rename support for single-file and workspace-wide edits.
- Added plan and change cards for reviewing an active task and its resulting workspace modifications.
- Improved conversation rollback to restore compaction state and context usage correctly.

## Runtime and UI

- Routed embedded Lua standard input, output, and error streams through terminal sessions, with optional captured output for Agent execution.
- Added throttled streaming Markdown rendering with parsed code blocks, horizontal scrolling, and copy actions.
- Added thinking, context compaction, and reconnect activity indicators.
- Improved turn summaries, collapsed execution traces, steering messages, tool groups, and tooltips.
- Added interactive toast actions and persistent error notifications.
- Added a standalone crash-report viewer.
- Added the product introduction video project and a new real-device Agent workflow demo.
- Unified runtime and MCP client version reporting through `QApplication::applicationVersion()`.

## Fixes

- Fixed a crash when closing document pages.
- Fixed text attachment compatibility and double-wrapped tool arguments from some providers.
- Fixed Agent abort, error, and completion state handling.
- Fixed conversation rollback after context compaction and refreshed the displayed usage value.
- Fixed several Agent layout, turn tooltip, and execution-card behavior issues.

# v0.3.0-alpha1

## Highlights

- Reworked the LLM module into the new Agent module.
- Added solo/team agent workflows with supervisor, software, hardware, and general agent roles.
- Added persistent agent context, SQLite-backed history, token usage tracking, permission controls, and user input request handling.
- Expanded port support with BLE, WebSocket client/server, SSL server, and richer port configuration tools.
- Reworked editor/document internals, including code page/document page naming, search window, Scintilla coordinate cleanup, and UTF-8/UTF-16 fixes.
- Updated terminal rendering and input handling with the UniComm vterm fork.
- Refined the UI with toast notifications, setting windows, permission cards, task state icons, and theme updates.

## Agent

- Replaced the old LLM module with the Agent module.
- Added provider management based on OpenAI-compatible providers.
- Added agent roles for supervisor, software, hardware, and general workflows.
- Added solo/team strategy support.
- Added context compaction, token usage display, and SQLite-backed agent storage.
- Added `plan_update`, `request_user_input`, permission display/control, and multi-input request handling.
- Added file, memory, port, terminal, and tool-call integration for agent workflows.

## Ports and APIs

- Added Bluetooth LE support.
- Added WebSocket client and server support.
- Added SSL server support.
- Added FTP and workspace-relative standard Lua I/O.
- Reworked HTTP, IMAP, SMTP, ModbusAscii, ModbusRtu, ModbusTcp, and port APIs.
- Moved UniComm Lua annotations and demos under `resources/meta/3rd/UniComm`.
- Removed the standalone SendModule.

## Document and Editor

- Reworked Lua pages into code pages and base pages into document pages.
- Added Web page support.
- Moved assistant widgets under the document assistant/module structure.
- Added document search window support.
- Cleaned up Scintilla coordinate handling and fixed UTF-8/UTF-16 position issues.
- Improved Markdown/WebView handling and page naming/icons.

## Terminal

- Switched to the UniComm vterm fork.
- Improved terminal sizing, scrolling, input modifiers, and rendering behavior.
- Expanded terminal session management.

## UI

- Added toast notifications.
- Added agent management UI and permission cards.
- Updated status/task icons and warning/info visuals.
- Improved threadpool, database/datatable, port setting, Git, and main window interactions.
- Updated themes.

## Infrastructure and Dependencies

- Added SQLite runtime dependencies.
- Added Qt SQL and WebSockets runtime dependencies.
- Added SimpleBLE dependencies.
- Added Whisper dependencies.
- Added Lexilla dependencies.
- Updated build/dependency packaging scripts.

## Fixes

- Fixed UTF-8/UTF-16 editor position handling.
- Fixed vterm Ctrl modifier behavior.
- Fixed terminal size policy issues.
- Fixed WebView whitespace/page naming issues.
- Fixed agent abort/error state behavior.

# v0.2.0-alpha3

## Highlights

- Added a built-in Git workflow for status, log graph, diff/show, commit, push, fetch, merge/rebase, upstream detection, proxy configuration, and conflict resolution.
- Introduced a new terminal experience based on vterm/ConPTY, with ANSI/true-color rendering, UTF-8 support, cursor handling, mouse scroll, middle-click paste, and OSC 52 clipboard
  support.
- Added WebView and Markdown page support on top of the DocumentModule foundation.
- Added BigModel provider support.
- Added terminal session management and Markdown syntax highlighting support.
- Improved the desktop UI with better docking and split handles, a tree view toolbar, theme fixes, and explorer fixes.

## Git

- Added Git status, branch, remote, upstream, log graph, and file change display.
- Added commit and push windows, first-push handling, fetch support, and reset/show flows.
- Added merge/rebase and conflict resolution pages/widgets, including single-file and multi-file conflict handling.
- Added Git proxy support.
- Improved Git reliability with process queueing, file watcher handling, UTF-8 fixes, path fixes, and safer behavior during conflict resolution.

## Terminal

- Added vterm/ConPTY terminal widget infrastructure.
- Added ANSI color and true-color rendering.
- Added UTF-8 support, cursor rendering/blinking, resize behavior, scroll handling, mouse/key/modifier event routing, middle-click paste, bell callback, and OSC 52 clipboard
  support.
- Added configurable terminal sessions and batch/PowerShell icons.
- Added a terminal management window and persisted terminal configuration back to `config.json`.

## Document

- Added Markdown page support.
- Added WebView widget support.
- Added document navigation integration with status callbacks.
- Added conflict page/widget support for Git conflict resolution.
- Added Highlight.js-backed Markdown code highlighting.
- Improved Markdown HTML rendering and fixed several Markdown page issues.

## Coding

- Added BigModel provider support.
- Added agent interpreter mode for returning results to the LLM.

## UI

- Improved menu bar, status bar, tree view, split handles, docking visuals, file property dialog, and executable icon.
- Added tooltips to background task/status models.
- Fixed several layout, margin, overlay, theme, and explorer issues.

## Infrastructure

- Added background task infrastructure and status callbacks.
- Reorganized Git-related files under `service/git`.
- Added `QHtmlString` and `QFullHtmlString` conversions to `uni_cast`.
- Optimized CMake configuration and internal signal/property handling.

# v0.2.0-alpha2

## Coding

- Integrated initial LLM support with DeepSeek.

## Dependency

- Upgraded Qt from 6.10.1 to 6.11.1.

## APIs

- Rewrote ModbusAscii APIs.
- Rewrote ModbusRtu APIs.
- Added ModbusTcp support.
- Upgraded SMTP APIs.
- Added IMAP support.
- Added file operation APIs.

## Infrastructure

- Implemented the DocumentModule foundation.
- Added initial image, text, code, and PDF page support.

## UI

- Added dark theme support.
- Added password reveal-on-hover.
- Expanded the menu bar with edit and code actions.
- Added the file property dialog.

# v0.2.0-alpha1

## Infrastructure

- Switched from QScintilla to Scintilla to support newer editor features.
- Added a ring buffer class and API registration for improved port control.
- Changed the overlay to a fullscreen window for better visual presentation.

## Coding

- Added assembly view.
- Improved `textDocument/signatureHelp` support, including function overloads.
- Improved `textDocument/documentSymbol` support, with symbol breadcrumbs displayed below the coding area.

## UI

- Expanded status bar details for cursor position, EOL type, and encoding format.

# v0.1.0

## Initial release
