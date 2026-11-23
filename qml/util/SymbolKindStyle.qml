import QtQml

import UniComm

QtObject {
    function kindIcon(kind: int): string {
        switch (kind) {
            case LspSymbolKind.Package:
                return "qrc:/icon/symbolPackage.svg"
            case LspSymbolKind.Method:
            case LspSymbolKind.Function:
                return "qrc:/icon/symbolMethod.svg"
            case LspSymbolKind.Variable:
                return "qrc:/icon/symbolVariable.svg"
            case LspSymbolKind.Constant:
                return "qrc:/icon/symbolConstant.svg"
            case LspSymbolKind.String:
                return "qrc:/icon/symbolString.svg"
            case LspSymbolKind.Number:
                return "qrc:/icon/symbolNumeric.svg"
            case LspSymbolKind.Boolean:
                return "qrc:/icon/symbolBoolean.svg"
            case LspSymbolKind.Array:
                return "qrc:/icon/symbolArray.svg"
            default:
                return "qrc:/icon/symbolMisc.svg"
        }
    }

    function kindColor(kind: int): string {
        let key
        switch (kind) {
            case LspSymbolKind.Package:
                key = "package";
                break
            case LspSymbolKind.Method:
                key = "method";
                break
            case LspSymbolKind.Function:
                key = "function";
                break
            case LspSymbolKind.Variable:
                key = "variable";
                break
            case LspSymbolKind.Constant:
                key = "constant";
                break
            case LspSymbolKind.String:
                key = "string";
                break
            case LspSymbolKind.Number:
                key = "number";
                break
            case LspSymbolKind.Boolean:
                key = "boolean";
                break
            case LspSymbolKind.Array:
                key = "array";
                break
            case LspSymbolKind.Object:
                key = "object";
                break
            default:
                return global.theme.general.fore
        }
        return global.theme.lua?.structure?.[key]?.fore ?? global.theme.general.fore
    }
}
