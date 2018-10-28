' csv�t�@�C���̓��e�ƈ�v��������ǂ�W���o�͂ɏo��
' �����P�F�i���́j�o�͏��I���i1:�����ǖ��F���p 2:�����ǖ��ݒ�p 3:�����Ǘ��́j
' �����Q�F�i���́j�g�p����csv�t�@�C����
' �����R�F�i���́j���̕�����Ɋ܂܂�Ă�������ǂ�T��
'
Option Explicit

Const strSymHan1 = "!""#$%&'()*+,-./"
Const strSymHan2 = ":;<=>?[\]^`{|}~"
Const strSymHan3 = " _"
Const strSymZen1 = "�I�h���������f�i�j���{�C�|�D�^"
Const strSymZen2 = "�F�G�������H�m���n�O�e�o�b�p�`"
Const strSymZen3 = "�@�Q"
Const strSymZen4 = "�E����"                  ' ��r�s�v�ȋL��
Const strSymPar1 = "�i�k�m�o�q�s�u�w�y��"
Const strSymPar2 = "�j�l�n�p�r�t�v�x�z��"
Const strSymStp  = " _�i�j"                  ' �����ǋ�؂茟�o�p


'---------------------------------------------------------------------
' ��������
'---------------------------------------------------------------------
Dim oParam, nOutType, strFileRead, strIn1
Set oParam = WScript.Arguments

If (oParam.Count < 2) Then
  WScript.Echo "�����s��"
  WScript.Quit
End If

nOutType     = oParam(0)
strFileRead  = oParam(1)
strIn1       = oParam(2)

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
Dim strBufRead
Dim strInData, strInDataA
Dim nDimItem
Dim strItemData()
Dim strTmp
Dim strPreCh1, strPreCh2
Dim nResultCur
Dim nResultLevel
Dim strResultA
Dim strResultB
Dim strResultC

Dim nMatch
Dim re, matches
Set re = New RegExp

'--- ������ ---
nResultLevel = 0

'--- ���̓f�[�^�̕����񂩂��r��������擾 ---
strInData = ProcString(strIn1, 1)
strInDataA = Replace(strInData, "�|", "")     ' ���̂̓n�C�t���Ȃ��Ŕ�r

'--- �P�s�ڂ̕�����擾 ---
strBufRead = objStream.ReadLine
strBufRead = Replace(strBufRead, vbCrLf, "")
' �e���ڎ擾�E�ݒ�
nDimItem = GetCsvData(strItemData,strBufRead, 0)
ReDim strItemData(nDimItem)

'--- �Q�s�ڈȍ~�̏��� ---
Do While objStream.AtEndOfStream <> True
  strBufRead = objStream.ReadLine
  strBufRead = Replace(strBufRead, vbCrLf, "")

  Call GetCsvData(strItemData,strBufRead, nDimItem)
  nMatch = 1
  nResultCur = 0

  '--- �����ǂ̈�v�m�F ---
  If (nResultLevel < 5) Then        ' ��v�������o�̏ꍇ
    If (strItemData(0) <> "") Then
      strTmp = ProcString(strItemData(0), 1)
      re.Pattern = "( ?)(.?)" & strTmp
      Set matches = re.Execute(strInData)
      If (matches.Count > 0) Then
        '--- ��v�̎�O��������؂肩�m�F ---
        strPreCh1 = matches.Item(0).SubMatches.Item(0)
        strPreCh2 = matches.Item(0).SubMatches.Item(1)
        If (strPreCh1 = "" And strPreCh2 = "") Then      ' �t�@�C���擪��
          nResultCur = 5
        ElseIf (strPreCh1 = " " And strPreCh2 = "") Then ' �󔒎�
          nResultCur = 1
        ElseIf (InStr(strSymStp, strPreCh2) > 0) Then    ' �t�@�C����؂�
          If (strPreCh1 = " " And strPreCh2 = "_") Then  ' ���m�ȋ�؂�
            nResultCur = 5
          ElseIf (strPreCh2 = "�i") Then                 ' ����
            nResultCur = 4
          Else                                           ' �s���m�ȋ�؂�
            nResultCur = 1
          End If
        End If
      End If
      Set matches = Nothing
    End If
  End If

  '--- ���̂̈�v�m�F ---
  If (nResultLevel < 5) Then        ' ��v�������o�̏ꍇ
    If (strItemData(2) <> "") Then
      strTmp = ProcString(strItemData(2), 1)
      strTmp = Replace(strTmp, "�|", "")       ' ���̂̓n�C�t���Ȃ��Ŕ�r
      re.Pattern = "(.?)" & strTmp & "(.?)"
      Set matches = re.Execute(strInDataA)
      If (matches.Count > 0) Then
        '--- ��v�̑O�㕶������؂肩�m�F ---
        strPreCh1 = matches.Item(0).SubMatches.Item(0)
        strPreCh2 = matches.Item(0).SubMatches.Item(1)
        If ((strPreCh1 = "" Or InStr(strSymStp, strPreCh1) > 0) And _
            (strPreCh2 = "" Or InStr(strSymStp, strPreCh2) > 0)) Then
          If (strPreCh1 = "�i") Then    ' ��O��"("�̎��D�揇�ʂ��グ��
            nResultCur = 5
          ElseIf (nResultCur < 2) Then  ' ����ȊO�̋�؂�
            nResultCur = 2
          End If
        End If
      End If
      Set matches = Nothing
    End If
  End If

  '--- ��v���̏��� ---
  If (nResultCur > 0) Then
'    WScript.echo "*** match ***" & nResultCur & " " & strItemData(0)
    If (nResultLevel < nResultCur) Then
      nResultLevel = nResultCur
      strResultA = strItemData(0)
      strResultB = strItemData(1)
      strResultC = strItemData(2)
    End If
  End If
Loop

objStream.Close
Set objStream = Nothing
Set objFileSystem = Nothing
Set re = Nothing

'---------------------------------------------------------------------
' ���ʏo��
'---------------------------------------------------------------------
If (nResultLevel = 0) Then       ' ��v���Ȃ��������̏o��
  WScript.echo ""
ElseIf (nOutType = 1) Then       ' �����ǖ�(�F���p�j���o��
  WScript.echo strResultA
ElseIf (nOutType = 2) Then       ' �����ǖ��i�ݒ�p�j���o��
  WScript.echo strResultB
Else                             ' �����Ǘ��̂��o��
  WScript.echo strResultC
End If

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
