Option Explicit
'���͕��������ꕶ���͕ϊ��A����ȊO�͂��̂܂ܕԂ�

Dim oParam
Dim oStr
Set oParam = WScript.Arguments
oStr = oParam(0)
oStr = Replace(oStr, "&", "��")
oStr = Replace(oStr, "^", "�O")
oStr = Replace(oStr, "%", "��")
WScript.echo oStr
Set oParam   = Nothing
