pragma Singleton

import QtQml

import UniComm

QtObject {
    function icon(kind: int): string {
        switch (kind) {
            case LspCompletionKind.Text:
                return "qrc:/icon/symbolString.svg"
            case LspCompletionKind.Method:
            case LspCompletionKind.Function:
                return "qrc:/icon/symbolMethod.svg"
            case LspCompletionKind.Field:
            case LspCompletionKind.Property:
                return "qrc:/icon/symbolField.svg"
            case LspCompletionKind.Variable:
            case LspCompletionKind.Interface:
                return "qrc:/icon/symbolVariable.svg"
            case LspCompletionKind.Value:
                return "qrc:/icon/symbolConstant.svg"
            case LspCompletionKind.Enum:
                return "qrc:/icon/symbolEnum.svg"
            case LspCompletionKind.Keyword:
            case LspCompletionKind.Operator:
                return "qrc:/icon/symbolKeyword.svg"
            case LspCompletionKind.Snippet:
                return "qrc:/icon/symbolSnippet.svg"
            case LspCompletionKind.File:
                return "qrc:/icon/symbolFile.svg"
            case LspCompletionKind.EnumMember:
                return "qrc:/icon/symbolEnumMember.svg"
            case LspCompletionKind.Event:
                return "qrc:/icon/symbolEvent.svg"
            default:
                return "qrc:/icon/symbolMisc.svg"
        }
    }

    function color(kind: int): string {
        const colors = global.theme.lua.completion
        switch (kind) {
            case LspCompletionKind.Text:
                return colors.text.fore
            case LspCompletionKind.Method:
                return colors.method.fore
            case LspCompletionKind.Function:
                return colors.function.fore
            case LspCompletionKind.Field:
                return colors.field.fore
            case LspCompletionKind.Variable:
                return colors.variable.fore
            case LspCompletionKind.Class:
                return colors.class.fore
            case LspCompletionKind.Interface:
                return colors.interface.fore
            case LspCompletionKind.Property:
                return colors.property.fore
            case LspCompletionKind.Value:
                return colors.value.fore
            case LspCompletionKind.Enum:
                return colors.enum.fore
            case LspCompletionKind.Keyword:
                return colors.keyword.fore
            case LspCompletionKind.Snippet:
                return colors.snippet.fore
            case LspCompletionKind.File:
                return colors.file.fore
            case LspCompletionKind.EnumMember:
                return colors.enumMember.fore
            case LspCompletionKind.Event:
                return colors.event.fore
            case LspCompletionKind.Operator:
                return colors.operator.fore
            default:
                return global.theme.general.fore
        }
    }
}
