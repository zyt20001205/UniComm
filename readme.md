<h1 align="center">
    UniComm
</h1>

<div align="center">

<img src="icon/icon.ico" alt="icon" width="128">

</div>

<div align="center">

<a href="https://github.com/zyt20001205/UniComm" target="_blank">GitHub</a> |
<a href="https://gitee.com/ZHOU_125/UniComm" target="_blank">Gitee</a>

</div>

<p align="center">
    A programmable communication debugging tool for multiple protocols
</p>

<div align="center">

[![GitHub release](https://img.shields.io/github/v/release/zyt20001205/UniComm?color=%2334D058&label=Version)](https://github.com/zyt20001205/UniComm/releases)
[![PySide6](https://img.shields.io/badge/Built%20with-PySide6-blue)](https://pypi.org/project/PySide6/)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-Windows-lightgrey)]()

</div>

## Features

### Multiple Port/Protocol Support

<div align="center">

<img src="screenshot/multiple_port_protocol_support.png" alt="Multiple Port/Protocol Control" width="600">

</div>

### Visual Script Coding & Saving

<div align="center">

<img src="screenshot/visual_script_coding_saving.gif" alt="Visual Script Coding & Saving" width="800">

</div>

### Port Data Collecting & Processing

<div align="center">

<img src="screenshot/port_data_collecting_processing.gif" alt="Port Data Collecting & Processing" width="800">

</div>

## For Users

### Install

Download the latest release from [Releases](https://github.com/zyt20001205/UniComm/releases) page.

### Quick Start

<div align="center">

<img src="screenshot/quick_start.gif" alt="Quick Start" width="800">

</div>

## For Developers

### Requirements

- `Nuitka`                    2.7.5
- `PySide6`                   6.9.0
- `pyqtdarktheme`             2.1.0
- `pyqtgraph`                 0.13.7
- `requests`                  2.32.3

### Pack Command

```shell
nuitka --standalone --enable-plugin=pyside6 --windows-console-mode=disable --windows-icon-from-ico="icon/icon.ico" --include-data-dir=icon=icon --include-data-dir=translation=translation --include-data-file=LICENSE=LICENSE --output-dir=dist --output-filename=UniComm.exe src/main.py
```

### Localization

Generate .ts file:

```shell
pyside6-lupdate src/data_module.py src/document_module.py src/gui_module.py src/io_module.py src/log_module.py src/setting_module.py -ts translation/zh_CN.ts
```

Do translation:

```shell
pyside6-linguist translation/zh_CN.ts
```

Generate .qm file:

```shell
pyside6-lrelease translation/zh_CN.ts -qm translation/zh_CN.qm
```
