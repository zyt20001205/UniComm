cmake_minimum_required(VERSION 3.31)
project(UniComm)

set(CMAKE_INSTALL_PREFIX "D:/Workspace/CLionProjects/UniComm-dist/latest" CACHE PATH "UniComm install directory" FORCE)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 REQUIRED COMPONENTS
        Core
        Core5Compat
        Graphs
        Gui
        Multimedia
        Network
        Pdf
        PrintSupport
        Qml
        Quick
        QuickControls2
        QuickWidgets
        SerialPort
        Sql
        Svg
        TextToSpeech
        WebSockets
        Widgets
)

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(GUI_TYPE WIN32)
else ()
    set(GUI_TYPE "")
endif ()

file(GLOB_RECURSE UNICOMM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_SOURCE_DIR}/include/*.h"
)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(UniComm ${GUI_TYPE}
        resources/icon/app.rc
        ${UNICOMM_SOURCES}
)

file(GLOB_RECURSE UNICOMM_QML CONFIGURE_DEPENDS
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "qml/*.qml"
)

qt_add_qml_module(UniComm
        URI UniComm
        RESOURCE_PREFIX "/"
        NO_RESOURCE_TARGET_PATH
        QML_FILES
        ${UNICOMM_QML}
)

file(GLOB_RECURSE UNICOMM_ICON CONFIGURE_DEPENDS
        "${CMAKE_SOURCE_DIR}/resources/*.ico"
        "${CMAKE_SOURCE_DIR}/resources/*.png"
        "${CMAKE_SOURCE_DIR}/resources/*.svg"
)

qt_add_resources(UniComm "icon_resources"
        PREFIX "/icon"
        BASE "${CMAKE_SOURCE_DIR}/resources/icon"
        FILES
        ${UNICOMM_ICON}
)

qt_add_resources(UniComm "config_resources"
        PREFIX "/config"
        BASE "resources/config"
        FILES
        "resources/config/config.json"
        "resources/config/api.json"
)

set(UNICOMM_INCLUDE "${CMAKE_SOURCE_DIR}/include")
set(UNICOMM_QML_INCLUDE "${UNICOMM_INCLUDE}/qml")
set(THEME_DIR "${CMAKE_SOURCE_DIR}/resources/theme")

set(QT_DIR "${CMAKE_SOURCE_DIR}/3rd/qt")
set(QT_BIN "${QT_DIR}/bin")
set(QT_QML "${QT_DIR}/qml")
set(QT_SHARE "${QT_DIR}/share")

set(MISC_DIR "${CMAKE_SOURCE_DIR}/3rd/misc")
set(MISC_BIN "${MISC_DIR}/bin")

#3rd alphabetical order
set(CMARK_DIR "${CMAKE_SOURCE_DIR}/3rd/cmark-gfm")
set(CMARK_INCLUDE "${CMARK_DIR}/include")
file(GLOB CMARK_LIB "${CMARK_DIR}/lib/*.a")
set(CMARK_BIN "${CMARK_DIR}/bin")

set(DRMINGW_DIR "${CMAKE_SOURCE_DIR}/3rd/drmingw")
set(DRMINGW_INCLUDE "${DRMINGW_DIR}/include")
file(GLOB DRMINGW_LIB "${DRMINGW_DIR}/lib/*.a")
set(DRMINGW_BIN "${DRMINGW_DIR}/bin")

set(HIGHLIGHT_DIR "${CMAKE_SOURCE_DIR}/3rd/highlight")

set(ICU_DIR "${CMAKE_SOURCE_DIR}/3rd/icu")
set(ICU_INCLUDE "${ICU_DIR}/include")
file(GLOB ICU_LIB "${ICU_DIR}/lib/*.a")
set(ICU_BIN "${ICU_DIR}/bin")

set(KDDW_DIR "${CMAKE_SOURCE_DIR}/3rd/kddw")
set(KDDW_INCLUDE "${KDDW_DIR}/include")
file(GLOB KDDW_LIB "${KDDW_DIR}/lib/*.a")
set(KDDW_BIN "${KDDW_DIR}/bin")

set(LEPTONICA_DIR "${CMAKE_SOURCE_DIR}/3rd/leptonica")
set(LEPTONICA_INCLUDE "${LEPTONICA_DIR}/include")
file(GLOB LEPTONICA_LIB "${LEPTONICA_DIR}/lib/*.a")
set(LEPTONICA_BIN "${LEPTONICA_DIR}/bin")

set(LEXILLA_DIR "${CMAKE_SOURCE_DIR}/3rd/lexilla")
set(LEXILLA_INCLUDE "${LEXILLA_DIR}/include")
file(GLOB LEXILLA_LIB "${LEXILLA_DIR}/lib/*.a")
set(LEXILLA_BIN "${LEXILLA_DIR}/bin")

set(LUA_DIR "${CMAKE_SOURCE_DIR}/3rd/lua")
set(LUA_INCLUDE "${LUA_DIR}/include")
file(GLOB LUA_LIB "${LUA_DIR}/lib/*.a")
set(LUA_BIN "${LUA_DIR}/bin")

set(LUALS_DIR "${CMAKE_SOURCE_DIR}/3rd/lua-language-server")

set(MDLS_DIR "${CMAKE_SOURCE_DIR}/3rd/marksman")

set(MERMAID_DIR "${CMAKE_SOURCE_DIR}/3rd/mermaid")

set(NUSPELL_DIR "${CMAKE_SOURCE_DIR}/3rd/nuspell")
set(NUSPELL_INCLUDE "${NUSPELL_DIR}/include")
file(GLOB NUSPELL_LIB "${NUSPELL_DIR}/lib/*.a")
set(NUSPELL_BIN "${NUSPELL_DIR}/bin")
set(NUSPELL_DICT "${NUSPELL_DIR}/dict")

set(OPENCV_DIR "${CMAKE_SOURCE_DIR}/3rd/opencv")
set(OPENCV_INCLUDE "${OPENCV_DIR}/include")
file(GLOB OPENCV_LIB "${OPENCV_DIR}/lib/*.a")
set(OPENCV_BIN "${OPENCV_DIR}/bin")

set(QTKEYCHAIN_DIR "${CMAKE_SOURCE_DIR}/3rd/qtkeychain")
set(QTKEYCHAIN_INCLUDE "${QTKEYCHAIN_DIR}/include")
file(GLOB QTKEYCHAIN_LIB "${QTKEYCHAIN_DIR}/lib/*.a")
set(QTKEYCHAIN_BIN "${QTKEYCHAIN_DIR}/bin")

set(RIPGREP_DIR "${CMAKE_SOURCE_DIR}/3rd/ripgrep")

set(SCINTILLA_DIR "${CMAKE_SOURCE_DIR}/3rd/scintilla")
set(SCINTILLA_INCLUDE "${SCINTILLA_DIR}/include")
file(GLOB SCINTILLA_LIB "${SCINTILLA_DIR}/lib/*.a")
set(SCINTILLA_BIN "${SCINTILLA_DIR}/bin")

set(SIMPLEBLE_DIR "${CMAKE_SOURCE_DIR}/3rd/simpleble")
set(SIMPLEBLE_INCLUDE "${SIMPLEBLE_DIR}/include")
set(SIMPLEBLE_LIB "${SIMPLEBLE_DIR}/lib/libsimpleble.dll.a")
set(SIMPLEBLE_BIN "${SIMPLEBLE_DIR}/bin")

set(SOL2_DIR "${CMAKE_SOURCE_DIR}/3rd/sol2")
set(SOL2_INCLUDE "${SOL2_DIR}/include")

set(TESSERACT_DIR "${CMAKE_SOURCE_DIR}/3rd/tesseract")
set(TESSERACT_INCLUDE "${TESSERACT_DIR}/include")
file(GLOB TESSERACT_LIB "${TESSERACT_DIR}/lib/*.a")
set(TESSERACT_BIN "${TESSERACT_DIR}/bin")
set(TESSERACT_LANG "${TESSERACT_DIR}/lang")

set(VISA_DIR "${CMAKE_SOURCE_DIR}/3rd/visa")
set(VISA_INCLUDE "${VISA_DIR}/include")
file(GLOB VISA_LIB "${VISA_DIR}/bin/*.dll")
set(VISA_BIN "${VISA_DIR}/bin")

set(VTERM_DIR "${CMAKE_SOURCE_DIR}/3rd/vterm-unicomm")
add_subdirectory("${VTERM_DIR}" "${CMAKE_BINARY_DIR}/3rd/vterm-unicomm")

set(WEBVIEW2_DIR "${CMAKE_SOURCE_DIR}/3rd/webview2")
set(WEBVIEW2_INCLUDE "${WEBVIEW2_DIR}/include")
file(GLOB WEBVIEW2_LIB "${WEBVIEW2_DIR}/lib/*.dll")
set(WEBVIEW2_BIN "${WEBVIEW2_DIR}/bin")

set(WHISPER_DIR "${CMAKE_SOURCE_DIR}/3rd/whisper")
set(WHISPER_INCLUDE "${WHISPER_DIR}/include")
file(GLOB WHISPER_LIB "${WHISPER_DIR}/lib/*.a")
set(WHISPER_BIN "${WHISPER_DIR}/bin")
set(WHISPER_MODEL "${WHISPER_DIR}/model")

target_include_directories(UniComm PRIVATE
        ${UNICOMM_INCLUDE}
        ${UNICOMM_QML_INCLUDE}
        ${CMARK_INCLUDE}
        ${DRMINGW_INCLUDE}
        ${ICU_INCLUDE}
        ${KDDW_INCLUDE}
        ${LEPTONICA_INCLUDE}
        ${LEXILLA_INCLUDE}
        ${LUA_INCLUDE}
        ${NUSPELL_INCLUDE}
        ${OPENCV_INCLUDE}
        ${QTKEYCHAIN_INCLUDE}
        ${SCINTILLA_INCLUDE}
        ${SIMPLEBLE_INCLUDE}
        ${SOL2_INCLUDE}
        ${TESSERACT_INCLUDE}
        ${VISA_INCLUDE}
        ${WEBVIEW2_INCLUDE}
        ${WHISPER_INCLUDE}
)

target_link_libraries(UniComm PRIVATE
        Qt6::Core
        Qt6::Core5Compat
        Qt6::Gui
        Qt6::Graphs
        Qt6::Multimedia
        Qt6::Network
        Qt6::Pdf
        Qt6::PrintSupport
        Qt6::Qml
        Qt6::Quick
        Qt6::QuickControls2
        Qt6::QuickWidgets
        Qt6::SerialPort
        Qt6::Sql
        Qt6::Svg
        Qt6::TextToSpeech
        Qt6::WebSockets
        Qt6::Widgets
        ${CMARK_LIB}
        ${DRMINGW_LIB}
        ${ICU_LIB}
        ${KDDW_LIB}
        ${LEPTONICA_LIB}
        ${LEXILLA_LIB}
        ${LUA_LIB}
        ${NUSPELL_LIB}
        ${QTKEYCHAIN_LIB}
        ${SCINTILLA_LIB}
        ${SIMPLEBLE_LIB}
        ${TESSERACT_LIB}
        ${VISA_LIB}
        ${WHISPER_LIB}
        UniComm_Vterm
        ${WEBVIEW2_LIB}
        ole32
        -Wl,--start-group
        ${OPENCV_LIB}
        -Wl,--end-group
)

target_compile_definitions(UniComm PRIVATE
        KDDW_FRONTEND_QT
        KDDW_FRONTEND_QTWIDGETS
        $<$<CONFIG:Release>:QT_NO_DEBUG_OUTPUT>
)

target_compile_options(UniComm PRIVATE
        $<$<CONFIG:Debug>:-g>
        $<$<CONFIG:Release>:-g1>
        $<$<CONFIG:Release>:-fno-omit-frame-pointer>
)

if(MINGW)
    set_source_files_properties(src/runtime/luaInterpreter.cpp PROPERTIES COMPILE_OPTIONS "-Wa,-mbig-obj")
endif()

# Keep the install layout aligned with the current POST_BUILD deployment.
install(TARGETS UniComm
        RUNTIME DESTINATION "."
)

install(DIRECTORY "${THEME_DIR}/"
        DESTINATION "theme"
)
install(DIRECTORY "${HIGHLIGHT_DIR}/"
        DESTINATION "javascript"
)
install(DIRECTORY "${MERMAID_DIR}/"
        DESTINATION "javascript"
)

install(DIRECTORY "${LUALS_DIR}/"
        DESTINATION "lua-language-server"
)
install(CODE [[
    file(REMOVE_RECURSE
        "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lua-language-server/meta/3rd/UniComm"
    )
]])
install(DIRECTORY "${CMAKE_SOURCE_DIR}/resources/meta/"
        DESTINATION "lua-language-server/meta"
)

install(DIRECTORY "${NUSPELL_DICT}/"
        DESTINATION "dict"
)
install(DIRECTORY "${RIPGREP_DIR}/"
        DESTINATION "ripgrep"
)
install(DIRECTORY "${TESSERACT_LANG}/"
        DESTINATION "."
)
install(DIRECTORY "${WHISPER_MODEL}/"
        DESTINATION "whisper/model"
)

set(UNICOMM_RUNTIME_BIN_DIRS
        "${QT_BIN}"
        "${MISC_BIN}"
        "${CMARK_BIN}"
        "${DRMINGW_BIN}"
        "${ICU_BIN}"
        "${KDDW_BIN}"
        "${LEPTONICA_BIN}"
        "${LEXILLA_BIN}"
        "${LUA_BIN}"
        "${NUSPELL_BIN}"
        "${OPENCV_BIN}"
        "${QTKEYCHAIN_BIN}"
        "${SCINTILLA_BIN}"
        "${SIMPLEBLE_BIN}"
        "${TESSERACT_BIN}"
        "${VISA_BIN}"
        "${WHISPER_BIN}"
        "${WEBVIEW2_BIN}"
)

foreach (RUNTIME_BIN_DIR IN LISTS UNICOMM_RUNTIME_BIN_DIRS)
    install(DIRECTORY "${RUNTIME_BIN_DIR}/"
            DESTINATION "."
    )
endforeach ()

install(DIRECTORY "${QT_SHARE}/"
        DESTINATION "share"
)

if(FALSE)
    #auto deploy(do not use)
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND Qt6::windeployqt
            --qmldir "${CMAKE_SOURCE_DIR}/qml"
            "$<TARGET_FILE:UniComm>")
else ()
    #theme
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${THEME_DIR}"
            "${CMAKE_BINARY_DIR}/theme"
            COMMENT "THEME DIR COPIED")
    #highlight-11.11.1
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${HIGHLIGHT_DIR}"
            "${CMAKE_BINARY_DIR}/javascript"
            COMMENT "HIGHLIGHT COPIED")
    #lua-language-server-3.18.2 + UniComm assets
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${LUALS_DIR}"
            "${CMAKE_BINARY_DIR}/lua-language-server"
            COMMAND ${CMAKE_COMMAND} -E remove_directory
            "${CMAKE_BINARY_DIR}/lua-language-server/meta/3rd/UniComm"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/resources/meta"
            "${CMAKE_BINARY_DIR}/lua-language-server/meta"
            COMMENT "LUALS DIR AND UNICOMM ASSETS COPIED")
    #mermaid-11.16.0
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MERMAID_DIR}"
            "${CMAKE_BINARY_DIR}/javascript"
            COMMENT "MERMAID COPIED")
    #nuspell-5.1.7-1
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${NUSPELL_DICT}"
            "${CMAKE_BINARY_DIR}/dict"
            COMMENT "NUSPELL DICT COPIED")
    #ripgrep-15.1.0
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${RIPGREP_DIR}"
            "${CMAKE_BINARY_DIR}/ripgrep"
            COMMENT "RIPGREP DIR COPIED")
    #tesseract-5.5.2-1
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TESSERACT_LANG}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "TESSERACT LANG COPIED")
    #whisper.cpp-1.9.1
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WHISPER_MODEL}"
            "${CMAKE_BINARY_DIR}/whisper/model"
            COMMENT "WHISPER MODEL COPIED")

    #qt-6.11.1
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "QT BIN COPIED")
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QT_SHARE}"
            "${CMAKE_BINARY_DIR}/share"
            COMMENT "QT SHARE COPIED")
    #misc
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${MISC_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "MISC BIN COPIED")
    #cmark-gfm-0.29.0.gfm.13
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMARK_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "CMARK BIN COPIED")
    #drmingw-0.9.11-2
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${DRMINGW_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "DRMINGW BIN COPIED")
    #icu-78.3-2
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${ICU_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "ICU BIN COPIED")
    #kddw-2.4.0
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${KDDW_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "KDDW BIN COPIED")
    #leptonica-1.87.0-1
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${LEPTONICA_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "LEPTONICA BIN COPIED")
    #lexilla-5.5.0
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${LEXILLA_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "LEXILLA BIN COPIED")
    #lua-5.5.0-1
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${LUA_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "LUA BIN COPIED")
    #nuspell-5.1.7-1
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${NUSPELL_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "NUSPELL BIN COPIED")
    #opencv-4.13.0-4
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${OPENCV_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "OPENCV BIN COPIED")
    #qtkeychain-0.15.0-2
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${QTKEYCHAIN_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "QTKEYCHAIN BIN COPIED")
    #scintilla-5.6.2
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SCINTILLA_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "SCINTILLA BIN COPIED")
    #simpleble-1.0.0
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${SIMPLEBLE_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "SIMPLEBLE BIN COPIED")
    #tesseract-5.5.2-1
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${TESSERACT_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "TESSERACT BIN COPIED")
    #visa
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${VISA_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "VISA BIN COPIED")
    #whisper.cpp-1.9.1
    add_custom_command(TARGET UniComm POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WHISPER_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "WHISPER BIN COPIED")
    #webview2
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${WEBVIEW2_BIN}"
            "${CMAKE_BINARY_DIR}"
            COMMENT "WEBVIEW2 BIN COPIED")
endif()
