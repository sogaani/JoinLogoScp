Option Explicit
'入力文字列を特殊文字は変換、それ以外はそのまま返す

Dim oParam
Dim oStr
Set oParam = WScript.Arguments
oStr = oParam(0)
oStr = Replace(oStr, "&", "＆")
oStr = Replace(oStr, "^", "＾")
oStr = Replace(oStr, "%", "％")
WScript.echo oStr
Set oParam   = Nothing
