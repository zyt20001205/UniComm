# Capability Overview

This page is a compact map of the current UniComm surface. It is not intended to
replace API annotations or the changelog. Alpha features can still change between
releases.

## Agent

- OpenAI-compatible provider management.
- Solo strategy with a general Agent.
- Team strategy with supervisor, hardware, and software roles.
- Streaming responses and reasoning display.
- Planning, permission requests, and structured user input.
- Persistent conversations, context compaction, and token usage tracking.
- Workspace search, document reading and editing, diagnostics, and memory search.
- Port inspection and configuration tools.
- Lua API discovery, example retrieval, and script execution.

## Lua runtime and editor

- Embedded Lua execution with multiple worker threads.
- Lua language-server integration and UniComm API annotations.
- Completion, diagnostics, formatting, symbols, navigation, and semantic tokens.
- Breakpoints, stepping, call stacks, variable watches, and runtime value updates.
- Search and replace, document viewers, Markdown, PDF, image, and Web pages.

## Connectivity

Core transports:

- Serial port.
- TCP client and server.
- UDP socket.

Additional or experimental transports:

- SSL client and server.
- WebSocket client and server.
- Bluetooth LE.
- VISA.
- Video stream with ROI, image processing, and OCR-oriented reads.

## Protocol and service APIs

- Raw port read, write, open, close, monitoring, and configuration.
- Modbus RTU, Modbus ASCII, and Modbus TCP helpers.
- HTTP.
- FTP.
- SMTP and IMAP.
- Workspace-relative standard Lua file I/O.
- Thread, input, string, and utility APIs.

The Lua-facing declarations and examples live under
[`resources/meta/3rd/UniComm`](../resources/meta/3rd/UniComm).

## Data and application modules

- Script-owned database windows for live key/value display.
- Script-owned data tables with CSV export.
- Integrated logs and notifications.
- Terminal sessions based on ConPTY and the UniComm vterm fork.
- Git status, history, diff, commit, push, merge/rebase, and conflict workflows.

## Detailed historical matrix

The previous README contained an exhaustive Passing/WIP matrix and a development
schedule. It is preserved in [`README_legacy.md`](../README_legacy.md) as a
historical reference. Some individual statuses may be older than the current
source; use the current [changelog](../CHANGELOG.md) and implementation when an
exact feature status matters.
