' csv�t�@�C���̓��e�i�����ǁA�^�C�g���j�ƈ�v������ϐ���W���o�͂ɏo��
' �����P�F�i���́j�g�p����csv�t�@�C����
' �����Q�F�i���́j����̕����Ǘ���
' �����R�F�i���́j����̃^�C�g��������
'
Option Explicit

Const strSymHan1 = "!""#$%&'()*+,-./"
Const strSymHan2 = ":;<=>?[\]^`{|}~"
Const strSymHan3 = " _-"
Const strSymZen1 = "�I�h���������f�i�j���{�C�|�D�^"
Const strSymZen2 = "�F�G�������H�m���n�O�e�o�b�p�`"
Const strSymZen3 = "�@�Q�|"
Const strSymZen4 = "�E����"                  ' ��r�s�v�ȋL��
Const strSymPar1 = "�i�k�m�o�q�s�u�w�y��"
Const strSymPar2 = "�j�l�n�p�r�t�v�x�z��"
Const strSymStp  = " _�i�j"                  ' �����ǋ�؂茟�o�p
Const strSymRExp = ".*+?|[]^"                ' ���K�\���������o�p

'---------------------------------------------------------------------
' ��������
'---------------------------------------------------------------------
Dim oParam, strFileRead, strIn1, strIn2
Set oParam = WScript.Arguments

If (oParam.Count < 3) Then
  WScript.Echo "�����s��"
  WScript.Quit
End If

strFileRead  = oParam(0)
strIn1       = oParam(1)
strIn2       = oParam(2)

Set oParam   = Nothing

'---------------------------------------------------------------------
' �t�@�C���I�[�v��
'---------------------------------------------------------------------
Dim objFileSystem, objStream

Set objFileSystem = WScript.CreateObject("Scripting.FileSystemObject")
Set objStream = objFileSystem.OpenTextFile(strFileRead)

'---------------------------------------------------------------------
' ���ڎ擾
'---------------------------------------------------------------------
Dim i
Dim strBufRead
Dim strInData1, strInData2
Dim nDimItem
Dim nLocComment
Dim strItemHead()
Dim strItemData()
Dim nMatch

'--- ������ ---
nLocComment = -1

'--- ���̓f�[�^�̕����񂩂��r��������擾 ---
strInData1 = ProcString(strIn1, 2)
strInData2 = ProcString(strIn2, 2)

'--- �P�s�ڂ̕�����擾 ---
strBufRead = objStream.ReadLine
strBufRead = Replace(strBufRead, vbCrLf, "")
' �e���ڎ擾�E�ݒ�
nDimItem = GetCsvData(strItemHead,strBufRead, 0)
ReDim strItemHead(nDimItem)
ReDim strItemData(nDimItem)
Call GetCsvData(strItemHead, strBufRead, nDimItem)
' �R�����g�\������擾
For i=0 To nDimItem-1
  If (Left(strItemHead(i), 1) = "#") Then
    If (nLocComment < 0) Then
      nLocComment = i
    End If
  End If
Next

'--- �Q�s�ڈȍ~�̏��� ---
Do While objStream.AtEndOfStream <> True
  strBufRead = objStream.ReadLine
  strBufRead = Replace(strBufRead, vbCrLf, "")

  '--- csv�f�[�^�ǂݍ��� ---
  Call GetCsvData(strItemData,strBufRead, nDimItem)
  nMatch = 1

  '--- �����ǂ̈�v�m�F ---
  If (strItemData(0) <> "") Then
    If (CompareString(strInData1, strItemData(0), 1) = 0) Then
      nMatch = 0
    ElseIf (Left(strItemData(0), 1) = "#") Then
      nMatch = 0
    End If
  End If

  '--- �^�C�g���̈�v�m�F ---
  If (strItemData(1) <> "") Then
    If (CompareString(strInData2, strItemData(1), 2) = 0) Then
      nMatch = 0
    End If
  End If

  '--- ��v���̏��� ---
  If (nMatch > 0) Then
    ' �R�����g�s�\��
    If (nLocComment >= 0) Then
      If (strItemData(nLocComment) <> "") Then
        WScript.echo "rem ## " & strItemData(nLocComment)
      End If
    End If

    ' �e���ڕ\��
    For i=2 To nDimItem-1
      If (strItemHead(i) <> "" And strItemData(i) <> "") Then
        If (Left(strItemHead(i), 1) <> "#") Then
          If (StrComp(strItemData(i), "@") = 0) Then
            WScript.echo "set " & strItemHead(i) & "="
          Else
            WScript.echo "set " & strItemHead(i) & "=" & strItemData(i)
          End If
        End If
      End If
    Next

    WScript.echo
  End If
Loop

objStream.Close


Set objStream = Nothing
Set objFileSystem = Nothing


'---------------------------------------------------------------------
' �Q�̕�������r
' ����
'   strSrc  : ������̕�����
'   strCmp  : �������镶����
'   nType   : 1=�����ǔ�r�p  2=�^�C�g����r�p
' �߂�l�F 0=�s��v  1=��v
'---------------------------------------------------------------------
Function CompareString(strSrc, strCmp, nType)
  Dim i
  Dim nLen
  Dim nRegExp
  Dim nResult
  Dim strTmp
  Dim ary
  Dim nChk
  Dim re, matches
  Set re = New RegExp

  nResult = 1
  nLen = Len(strCmp)
  If (nLen > 0) Then
    '--- ���K�\���ɂ��邩�`�F�b�N ---
    nRegExp = 0
    i = 1
    Do While (i <= nLen And nRegExp = 0)
      if (InStr(strSymRExp, Mid(strCmp, i, 1)) > 0) Then
        nRegExp = 1              ' ���K�\����r���s��
      End If
      i = i + 1
    Loop
    '--- ������̔�r ---
    If (nRegExp > 0) Then        ' ���K�\�����̔�r
      strTmp = ProcString(strCmp, 3)     ' ���p�L���͂��̂܂�
      If (nType = 1) Then
        re.Pattern = "^" & strTmp & "$"
      Else
        re.Pattern = strTmp
      End If
      Set matches = re.Execute(strSrc)
      If (matches.Count = 0) Then
        nResult = 0
      End If
      Set matches = Nothing
    Else                          ' �X�y�[�X��؂茟����
      If (nType = 1) Then         ' �����ǌ��o����OR����邽�ߏ�����Ԃ�s��v
        nResult = 0
      End If
      strTmp = ProcString(strCmp, 2)     ' �^�C�g�������p������ϊ�
      ary = Split(strTmp)                 ' �X�y�[�X��؂�
      For i=0 To UBound(ary)
        strTmp = ary(i)
        If (nType = 1) Then                     ' ������
          If (StrComp(strSrc, strTmp) = 0) Then ' �����񊮑S��v
            nResult = 1
          End If
        Else                                    ' �^�C�g��
          If (InStr(strSrc, strTmp) = 0) Then   ' �����񌟏o�Ȃ�
            nResult = 0
          End If
        End If
      Next
    End If
  End If

  CompareString = nResult
  Set re = Nothing
End Function


'---------------------------------------------------------------------
' csv�`���̕����񂩂�e���ڂ��擾
' ����
'   strItem : �擾�����e���ڂ̕�����z��i�o�́j
'   strLine : csv�`���̕�����i���́j
'   nDim    : ��������i�[���鍀�ڐ��i���́j
' �߂�l�́A�擾�������ڐ�
'---------------------------------------------------------------------
Function GetCsvData(strItem, strLine, nDimItem)
  Dim i
  Dim str1, str2
  Dim nCount
  Dim nDQ, nPDQ, nAdd

  str1 = ""
  nCount = 0
  nDQ = 0
  nPDQ = 0
  '--- ����������ԂɊm�F ---
  For i=1 To Len(strLine)
    str2 = Mid(strLine, i, 1)
    nAdd = 1
    '--- �P�O���_�u���N�H�[�g�̎��A���ڏI�����f ---
    If (nPDQ > 0) Then
      If (str2 = "," And nDQ = 1) Then
        nDQ = 0
      End If
    End If
    '--- ���ڋ�؂肪����΍��ڊm�菈�� ---
    If (str2 = "," And nDQ = 0) Then
      If (nCount < nDimItem) Then    ' �擾������������o��
        strItem(nCount) = str1
      End If
      str1 = ""
      nCount = nCount + 1
      nAdd = 0
      nPDQ = 0
    '--- �_�u���N�H�[�g�̎� ---
    ElseIf (str2 = """") Then
      If (nPDQ = 0) Then
        nPDQ = 1
        nAdd = 0
      ElseIf (nPDQ = 1 And nDQ = 0 And str1 = "") Then
        nPDQ = 2
        nAdd = 0
      End If
    End If
    '--- ���ڂɕ����ǉ� ---
    If (nAdd > 0) Then
      If (nPDQ > 0) Then
        nPDQ = 0
        If (str1 = "") Then
          nDQ  = 1
        End If
      End If
      str1 = str1 & str2
    End If
  Next
  '--- ���ڍŌ�܂Ŋi�[ ---
  Do While(nCount < nDimItem)
    strItem(nCount) = str1
    str1 = ""
    nCount = nCount + 1
  Loop

  GetCsvData = nCount + 1
End Function


'---------------------------------------------------------------------
' �������r�̂��߁A�S�p�p�����͔��p�啶���ɕϊ��A�L���͈����ɂ�鏈��
' ����
'   strData : ���H���镶����i���́j
'   nType   : �����񏈗��i���́j
'              0:���̂܂܏o��
'              1:�����Ǖ����񌟍��p�i�L���͑S�p���{���H����j
'              2:�^�C�g�������p�i�L���͑S�p���j
'              3:���K�\���^�C�g�������p�i���p�L���͂��̂܂܁j
'
' �߂�l�́A�ϊ���̕�����
'---------------------------------------------------------------------
Function ProcString(strData, nType)
  Dim i
  Dim strNew, strCh

  strNew = ""
  For i=1 To Len(strData)
    strCh = Mid(strData, i, 1)
    Call ConvChar(strCh, nType)
    strNew = strNew & strCh
  Next
  ProcString = strNew
End Function


'---------------------------------------------------------------------
' ��������ɑS�p���p�̕ϊ�������
' ����
'   strCh : �`�F�b�N�Ώۂ̕����i���́{�o�́j
'   nType : �����񏈗��i���́j
' �߂�l�́A�L�����o�����ꍇ��1�A����ȊO��0
'---------------------------------------------------------------------
Function ConvChar(strCh, nType)
  Dim j, k
  Dim nDet

  nDet = 0
  ' ���p�L���͑S�p�ɕϊ�
  If (nType = 1 Or nType = 2) Then
    j = InStr(strSymHan1, strCh)         ' ���p�L���P�p�^�[����
    If (j > 0) Then
      strCh = Mid(strSymZen1, j, 1)
      nDet = 1
    End If

    j = InStr(strSymHan2, strCh)         ' ���p�L���Q�p�^�[����
    If (j > 0) Then
      strCh = Mid(strSymZen2, j, 1)
      nDet = 1
    End If
  End If

  '�S�p�L�������H
  If (nType = 1) Then
    j = InStr(strSymPar1, strCh)
    If (j > 0) Then
      strCh = "�i"
      nDet = 1
    End If

    j = InStr(strSymPar2, strCh)
    If (j > 0) Then
      strCh = "�j"
      nDet = 1
    End If

    j = InStr(strSymZen4, strCh)
    If (j > 0) Then
      strCh = ""
      nDet = 1
    End If
  End If

  ' �p�����̑S�p�����𔼊p�ɂ���
  If (nType >= 0 And nType <= 3) Then
    '�p���������̑S�p�����𔼊p�ɂ���
    j = InStr(strSymZen3, strCh)
    If (j > 0) Then
      strCh = Mid(strSymHan3, j, 1)
      nDet = 1
    End If

    If (nDet = 0) Then
      k = Asc(strCh)
      If (k >= Asc("�O") And k <= Asc("�X")) Then
        strCh = Chr(k - Asc("�O") + Asc("0"))
        nDet = 1
      ElseIf (k >= Asc("�`") And k <= Asc("�y")) Then
        strCh = Chr(k - Asc("�`") + Asc("A"))
        nDet = 1
      ElseIf (k >= Asc("��") And k <= Asc("��")) Then
        strCh = Chr(k - Asc("��") + Asc("a"))
        nDet = 1
      End If
    End If
  End If

  ' ��������啶���ɂ���
  If (nType >= 0 And nType <= 3) Then
    k = Asc(strCh)
    If (k >= Asc("a") And k <= Asc("z")) Then
      strCh = Chr(k - Asc("a") + Asc("A"))
      nDet = 1
    End If
  End If

  ConvChar = nDet

End Function
