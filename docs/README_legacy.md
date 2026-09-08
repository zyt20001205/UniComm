<h1 align="center">
    UniComm
</h1>

<div align="center">

<img src="resources/icon/icon.ico" alt="icon" width="128">

</div>

<div align="center">

<a href="https://github.com/zyt20001205/UniComm" target="_blank">GitHub</a>

</div>

<p align="center">
    A programmable communication debugging platform for multiple protocols
</p>

<div align="center">

[![GitHub release](https://img.shields.io/github/v/release/zyt20001205/UniComm?color=%2334D058&label=Version)](https://github.com/zyt20001205/UniComm/releases)
[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://www.gnu.org/licenses/gpl-3.0)
[![Qt](https://img.shields.io/badge/Qt-6.11.1-green)]()
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)]()

</div>

<div align="center">

<img src="resources/assets/screenshot/preview.png" alt="preview">

</div>

```mermaid
gantt
    title Development Schedule 2026
    dateFormat MM-DD
    axisFormat %m-%d
    tickInterval 1month

    section version
        v0.1.0: done, 02-10, 1d
        v0.2.0(Infrastructure Construction): active, 02-11, 140d
        v0.2.0-alpha1: done, 02-11, 49d
        v0.2.0-alpha2: done, 04-01, 54d
        v0.2.0-alpha3: done, 05-25, 37d
        v0.3.0(Interface Optimization): 07-01, 62d
        v0.3.0-alpha1: 07-01, 31d
        v0.3.0-alpha2: 08-01, 31d

    section apis
        port(ringbuffer refractor): done, 03-17, 15d
        string(raw<->hex): done, 03-17, 15d
        modbusAscii(ascii string refractor): done, 04-01, 7d
        modbusRtu(hex string refractor): done, 04-01, 7d
        modbusTcp: done, 04-01, 7d
        imap: done, 04-08, 8d
        file: done, 04-20, 2d
        mqtt: crit, 04-16, 15d

    section infra
        scintilla migration: done, 02-11, 30d
        ringbuffer class: done, 03-01, 16d
        fullscreen overlay: done, 03-26, 3d
        undo stack: crit, 04-01, 10d
        document module: done, 04-10, 7d

    section coding
        assembly view: done, 03-13, 19d
        signatureHelp(overload): done, 03-24, 1d
        documentSymbol bar: done, 03-25, 2d
        search & replace: done, 03-30, 4d
        rename: crit, 04-11, 20d

    section ui
        status bar: done, 03-10, 3d
        file property dialog: done, 04-10, 3d
        dark theme: done, 05-01, 7d
        port session monitor: crit, 05-01, 7d
        cli integration: done, 05-08, 48d
        git integration: done, 05-20, 42d
        custom settings: 07-01, 31d
        i18n: 08-01, 31d
```

# Gallery

## Agent

### Modbus read & export

<img src="resources/assets/screenshot/agent_modbus.png" alt="preview">

[![Bilibili](https://img.shields.io/badge/dynamic/json?color=fb7299&label=Bilibili&query=%24.data.stat.view&url=https%3A%2F%2Fapi.bilibili.com%2Fx%2Fweb-interface%2Fview%3Fbvid%3DBV1UXGn66Enh&style=flat-square&logo=bilibili)](https://www.bilibili.com/video/BV1UXGn66Enh/?share_source=copy_web&vd_source=be1c786f9652180888686cbcd18d4223)

# APIS

## Miscellaneous

### [IO](#io-apis)

### [String](#string-apis)

### [File](#file-apis)

### [Thread](#thread-apis)

## Port Communication

### [Base](#base-apis)

### [Modbus](#modbus-apis)

### [SMTP](#smtp-apis)

### [IMAP](#imap-apis)

## Data Process

### [Database](#database-apis)

### [Datatable](#datatable-apis)

## Input Simulation

### [Mouse](#mouse-apis)

### [Key](#key-apis)

# Port Module

## Support Port Types

<table align = "center">
    <tr>
        <th colspan="4">OSI Model</th>
    </tr>
    <tr>
        <td>Application</td>
        <td align = "center">
            <img src="https://img.shields.io/badge/Modbus_Tcp-Supported-brightgreen" alt="Modbus Tcp Support">
        </td>
        <td align = "center">
            <img src="https://img.shields.io/badge/Modbus_Ascii-Supported-brightgreen" alt="Modbus Ascii Support">
            <br><img src="https://img.shields.io/badge/Modbus_Rtu-Supported-brightgreen" alt="Modbus Rtu Support">
            <br><img src="https://img.shields.io/badge/SMTP-Supported-brightgreen" alt="SMTP Support">
            <br><img src="https://img.shields.io/badge/USB_TMC-WIP-yellow" alt="USB TMC Support">
        </td>
        <td align = "center">
            <img src="https://img.shields.io/badge/OCR-Supported-brightgreen" alt="OCR Support">
        </td>
    </tr>
    <tr>
        <td>Presentation</td>
        <td align = "center">
            <img src="https://img.shields.io/badge/SSL/TLS-Supported-brightgreen" alt="SSL/TLS Support">
        </td>
        <td></td>
        <td align = "center">
            <img src="https://img.shields.io/badge/Vision-Supported-brightgreen" alt="Vision Support">
        </td>
    </tr>
    <tr>
        <td>Session</td>
        <td></td>
        <td></td>
        <td></td>
    </tr>
    <tr>
        <td>Transport</td>
        <td align = "center">
            <img src="https://img.shields.io/badge/TCP-Supported-brightgreen" alt="TCP Support">
            <br><img src="https://img.shields.io/badge/UDP-Supported-brightgreen" alt="UDP Support">
        </td>
        <td></td>
        <td></td>
    </tr>
    <tr>
        <td>Network</td>
        <td align = "center">IP</td>
        <td></td>
        <td></td>
    </tr>
    <tr>
        <td>Data Link</td>
        <td align = "center">Ethernet</td>
        <td align = "center">Serial Framing</td>
        <td></td>
    </tr>
    <tr>
        <td>Physical</td>
        <td align = "center">RJ45</td>
        <td align = "center">
            <img src="https://img.shields.io/badge/Serial_Port-Supported-brightgreen" alt="Serial Port Support">
        </td>
        <td align = "center">Screen
            <br>Camera</td>
    </tr>
</table>

## Base APIS

|    APIS    |                             Serial Port                             |                             Tcp Client                              |                             Ssl Client                              |                             Tcp Server                              |                             Udp Socket                              |                               Vision                                |
|:----------:|:-------------------------------------------------------------------:|:-------------------------------------------------------------------:|:-------------------------------------------------------------------:|:-------------------------------------------------------------------:|:-------------------------------------------------------------------:|:-------------------------------------------------------------------:|
| port.open  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| port.close | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| port.info  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
| port.read  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| port.write | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## Modbus APIS

|                    APIS                    | Modbus Protocol                                  | ASCII                                                               | RTU                                                                 | Tcp                                                                 |
|:------------------------------------------:|:-------------------------------------------------|:--------------------------------------------------------------------|:--------------------------------------------------------------------|:--------------------------------------------------------------------|
|                                            | 01 (0x01) Read Coils                             | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 02 (0x02) Read Discrete Inputs                   | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|   modbusAscii/Rtu/Tcp.readHoldRegisters    | 03 (0x03) Read Holding Registers                 | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|                                            | 04 (0x04) Read Input Registers                   | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 05 (0x05) Write Single Coil                      | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|  modbusAscii/Rtu/Tcp.writeSingleRegister   | 06 (0x06) Write Single Register                  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|                                            | 08 (0x08) Diagnostics                            | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 11 (0x0B) Get Comm Event Counter                 | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 15 (0x0F) Write Multiple Coils                   | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
| modbusAscii/Rtu/Tcp.writeMultipleRegisters | 16 (0x10) Write Multiple Registers               | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|                                            | 17 (0x11) Report Server ID                       | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 22 (0x16) Mask Write Register                    | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 23 (0x17) Read/Write Multiple Registers          | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
|                                            | 43 / 14 (0x2B / 0x0E) Read Device Identification | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |

## SMTP APIS

|      APIS      |   RFC 4954    |                               Status                                |                           
|:--------------:|:-------------:|:-------------------------------------------------------------------:|
| smtp:authLogin |  AUTH LOGIN   | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
|                |  AUTH PLAIN   |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|                | AUTH CRAM-MD5 |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 

|   APIS    |               RFC 5321                |                               Status                                |                           
|:---------:|:-------------------------------------:|:-------------------------------------------------------------------:|
| smtp:ehlo | Extended HELLO (EHLO) or HELLO (HELO) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
|           |              MAIL (MAIL)              |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|           |           RECIPIENT (RCPT)            |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |              DATA (DATA)              |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |             RESET (RSET)              |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |             VERIFY (VRFY)             |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |             EXPAND (EXPN)             |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |              HELP (HELP)              |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|           |              NOOP (NOOP)              |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
| smtp:quit |              QUIT (QUIT)              | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

|   APIS    |      UniComm       |                               Status                                |                           
|:---------:|:------------------:|:-------------------------------------------------------------------:|
| smtp:send | MAIL + RCPT + DATA | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## IMAP APIS

|   APIS    | RFC 2177 |                               Status                                |                           
|:---------:|:--------:|:-------------------------------------------------------------------:|
| imap:idle |   IDLE   | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

|    APIS     |   RFC 3501   |                               Status                                |                           
|:-----------:|:------------:|:-------------------------------------------------------------------:|
|             |   STARTTLS   |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|             | AUTHENTICATE |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
| imap:login  |    LOGIN     | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|             |              |                                                                     |
| imap:select |    SELECT    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|             |   EXAMINE    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    CREATE    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    DELETE    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    RENAME    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |  SUBSCRIBE   |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             | UNSUBSCRUBE  |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |     LIST     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |     LSUB     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    STATUS    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    APPEND    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |              |                                                                     |
|             |    CHECK     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    CLOSE     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |   EXPUNGE    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |    SEARCH    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
| imap:fetch  |    FETCH     | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|             |    STORE     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |     COPY     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |
|             |     UID      |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |

|     APIS     |   UniComm    |                               Status                                |                           
|:------------:|:------------:|:-------------------------------------------------------------------:|
| imap:receive | IDLE + FETCH | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

# Document Module

## Architecture

```mermaid
flowchart LR
    luaLanguageServer[Lua Language Server 3.16.4]
    documentModule[Document Module]
    subgraph scintilla[Scintilla 5.5.9]
        direction LR
        luaPage1[Lua Page 1]
        luaPage2[Lua Page 2]
        luaPage3[Lua Page 3]
        luaPage4[...]
    end
    documentModule e1@ -->|LSP Request| luaLanguageServer
    documentModule <==>|LSP Notification| luaLanguageServer
    luaLanguageServer e2@ -->|LSP Response| documentModule
    documentModule --- scintilla
    e1@{animate: true}
    e2@{animate: true}
```

## Editor Features

| Feature                 | Status                                                              |
|:------------------------|:--------------------------------------------------------------------|
| comment (Ctrl+/)        | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| duplicate line (Ctrl+D) | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| auto pair ( [ { \" '    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| search                  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| replace                 | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## Supported LSP Specifications

| LSP Specification               | Type         | Status                                                              |
|:--------------------------------|:-------------|:--------------------------------------------------------------------|
| textDocument/publishDiagnostics | Notification | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/codeAction         | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/codeLens           | Request      | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
| textDocument/completion         | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/definition         | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/documentHighlight  | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/documentSymbol     | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/foldingRange       | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/formatting         | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/hover              | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/implementation     | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/onTypeFormatting   | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/rangeFormatting    | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/references         | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/rename             | Request      | ![WIP](https://img.shields.io/badge/Status-WIP-yellow)              |
| textDocument/semanticTokens     | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/signatureHelp      | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| textDocument/typeDefinition     | Request      | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## Debug Features

| Feature                | Status                                                              |
|:-----------------------|:--------------------------------------------------------------------|
| stop                   | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| resume                 | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| pause                  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| step over              | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| step into              | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| step out               | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| run to cursor          | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
|                        |                                                                     |
| breakpoint console     | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| conditional breakpoint | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| variable watch         | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| variable hot update    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| callstack              | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| multithreading         | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

# Data Process

## Database Module

### Database APIS

|      APIS      |                               Status                                |                           
|:--------------:|:-------------------------------------------------------------------:|
| database.list  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| database.write | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| database.clear | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## Datatable Module

### Datatable APIS

|       APIS       |                               Status                                |                           
|:----------------:|:-------------------------------------------------------------------:|
|  datatable.list  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| datatable.write  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| datatable.clear  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| datatable.export | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

# Input Simulation

## Mouse APIS

|       APIS        |                               Status                                |                           
|:-----------------:|:-------------------------------------------------------------------:|
|    mouse.click    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| mouse.doubleClick | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | 
| mouse.rightClick  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|    mouse.down     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|     mouse.up      |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|    mouse.drag     |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|   mouse.scroll    |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 

## Key APIS

|   APIS   |                               Status                                |                           
|:--------:|:-------------------------------------------------------------------:|
| key.tap  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| key.type | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| key.down |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        | 
|  key.up  |       ![WIP](https://img.shields.io/badge/Status-WIP-yellow)        |

# Miscellaneous

## IO APIS

|    APIS    |                               Status                                | Discription         |
|:----------:|:-------------------------------------------------------------------:|:--------------------|
|   print    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | structured output  |
| io.message | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | messagebox          |
|  io.speak  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) | tts                 |

## String APIS

|       APIS        |                               Status                                |                           
|:-----------------:|:-------------------------------------------------------------------:|
|   string.toHex    | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|  string.fromHex   | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
|  string.toBase64  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| string.fromBase64 | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## File APIS

|  APIS   |                               Status                                |                           
|:-------:|:-------------------------------------------------------------------:|
| f.close | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| f.open  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| f.read  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| f.write | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |

## Thread APIS

|     APIS     |                               Status                                |                           
|:------------:|:-------------------------------------------------------------------:|
| thread.start | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| thread.stop  | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
| thread.sleep | ![Passing](https://img.shields.io/badge/Status-Passing-brightgreen) |
