Option Explicit
' unicode�ŕ�������t�@�C���ɏo��
' �����P�F�i���́j�o�̓t�@�C����
' �����Q�F�i���́j�o�͕�����

Dim oParam
Dim strFileName
Dim strTextData
Set oParam = WScript.Arguments
If (oParam.Count < 2) Then
  WScript.Echo "�����s��"
  WScript.Quit
End If
strFileName = oParam(0)
strTextData = oParam(1)
Set oParam = Nothing

Dim objFS
Dim objStreamW
Set objFS = WScript.CreateObject("Scripting.FileSystemObject")
Set objStreamW = objFS.OpenTextFile(strFileName, 2, 1, -1)

objStreamW.WriteLine strTextData

objStreamW.Close

Set objStreamW = Nothing
Set objFS = Nothing

