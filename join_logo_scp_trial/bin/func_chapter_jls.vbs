' Trim�t�@�C����join_logo_scp�\����̓t�@�C������`���v�^�[��W���o�͂ɏo��
' �����P�F�i���́j�o�̓`���v�^�[�`���iorg cut tvtplay tvtcut�j
' �����Q�F�i���́jTrim�t�@�C����
' �����R�F�i���́jjoin_logo_scp�\����̓t�@�C����

Option Explicit

'--------------------------------------------------
' �萔
'--------------------------------------------------
const PREFIX_TVTI = "ix"     ' �J�b�g�J�n��������itvtplay�p�j
const PREFIX_TVTO = "ox"     ' �J�b�g�I����������itvtplay�p�j
const PREFIX_ORGI = ""       ' �J�b�g�J�n��������i�J�b�g�Ȃ�chapter�j
const PREFIX_ORGO = ""       ' �J�b�g�I����������i�J�b�g�Ȃ�chapter�j
const PREFIX_CUTO = ""       ' �J�b�g�I����������i�J�b�g��j
const SUFFIX_CUTO = ""       ' �J�b�g�I���������ǉ�������i�J�b�g��j

const MODE_ORG = 0
const MODE_CUT = 1
const MODE_TVT = 2
const MODE_TVC = 3

const MSEC_DIVMIN = 100      ' �`���v�^�[�ʒu�𓯈�Ƃ��Ȃ����ԊԊu�imsec�P�ʁj

'--------------------------------------------------
' �����ǂݍ���
'--------------------------------------------------
Dim strFormat, strFile1, strFile2
Dim nOutFormat

If WScript.Arguments.Unnamed.Count < 3 Then
  WScript.StdErr.WriteLine "usage:func_chapter_jls.vbs org|cut|tvtplay <TrimFile> <jlsFile>"
  WScript.Quit
End If

strFormat = WScript.Arguments(0)
strFile1  = WScript.Arguments(1)
strFile2  = WScript.Arguments(2)

'--- �o�͌`�� ---
If StrComp(strFormat, "cut") = 0 Then           '�J�b�g���chapter
  nOutFormat = MODE_CUT
ElseIf StrComp(strFormat, "tvtplay") = 0 Then   '�J�b�g���Ȃ�TvtPlay
  nOutFormat = MODE_TVT
ElseIf StrComp(strFormat, "tvtcut") = 0 Then    '�J�b�g���TvtPlay
  nOutFormat = MODE_TVC
Else                                            '�J�b�g���Ȃ�chapter
  nOutFormat = MODE_ORG
End If

'--------------------------------------------------
' Trim�ɂ��J�b�g���ǂݍ���
' �ǂݍ��݃f�[�^�B�J�n�ʒu��\�����ߏI���ʒu�ł́{�P����B
' nTrimTotal  : Trim�ʒu��񍇌v�iTrim�P�ɂ��i�J�n,�I���j�łQ�j
' nItemTrim() : Trim�ʒu���i�P�ʂ̓t���[���j
'--------------------------------------------------
'--- ���ʕϐ� ---
Dim objFileSystem, objStream
Dim strBufRead
Dim i
Dim re, matches
Set re = New RegExp
re.Global = True

'--- �t�@�C���ǂݍ��� ---
Set objFileSystem = WScript.CreateObject("Scripting.FileSystemObject")
If Not objFileSystem.FileExists(strFile1) Then
  WScript.StdErr.WriteLine "�t�@�C����������܂���:" & strFile1
  WScript.Quit
End If
Set objStream = objFileSystem.OpenTextFile(strFile1)
strBufRead = objStream.ReadLine

'--- trim�p�^�[�� ---
Const strRegTrim = "Trim\((\d+)\,(\d+)\)"
'--- �p�^�[���}�b�` ---
re.Pattern = strRegTrim
Set matches = re.Execute(strBufRead)
If matches.Count = 0 Then
  WScript.StdErr.WriteLine "Trim�f�[�^���ǂݍ��߂܂���:" & strBufRead
  WScript.Quit
End If

'--- �f�[�^�ʌ��� ---
Dim nTrimTotal
nTrimTotal = matches.Count * 2

'--- �ϐ��Ɋi�[ ---
ReDim nItemTrim(nTrimTotal)
For i=0 To nTrimTotal/2 - 1
  nItemTrim(i*2)   = CLng(matches(i).SubMatches(0))
  nItemTrim(i*2+1) = CLng(matches(i).SubMatches(1)) + 1
Next
Set matches = Nothing

'--- �t�@�C���N���[�Y ---
objStream.Close
Set objStream = Nothing
Set objFileSystem  = Nothing

'--------------------------------------------------
' �\����̓t�@�C���ƃJ�b�g��񂩂�CHAPTER���쐬
'--------------------------------------------------
'--- CHAPTER���擾�ɕK�v�ȕϐ� ---
Dim clsChapter
Dim bCutOn, bShowOn, bShowPre, bPartExist
Dim nTrimNum, nType, nLastType, nPart
Dim nFrmTrim, nFrmSt, nFrmEd, nFrmMgn, nFrmBegin
Dim nSecRd, nSecCalc
Dim strCmt, strChapterName, strChapterLast

'--- CHAPTER���i�[�p�N���X ---
Set clsChapter = New ChapterData

'--- �t�@�C���I�[�v�� ---
Set objFileSystem = WScript.CreateObject("Scripting.FileSystemObject")
If Not objFileSystem.FileExists(strFile2) Then
  WScript.StdErr.WriteLine "�t�@�C����������܂���:" & strFile2
  WScript.Quit
End If
Set objStream = objFileSystem.OpenTextFile(strFile2)

'--- trim�p�^�[�� ---
Const strRegJls  = "^\s*(\d+)\s+(\d+)\s+(\d+)\s+([-\d]+)\s+(\d+).*:(\S+)"
'--- �����ݒ� ---
re.Pattern = strRegJls
nFrmMgn    = 30          ' Trim�Ɠǂݍ��ݍ\���𓯂��ʒu�Ƃ݂Ȃ��t���[����
bShowOn    = 1           ' �ŏ��͕K���\��
nTrimNum   = 0           ' ���݂�Trim�ʒu�ԍ�
nFrmTrim   = 0           ' ���݂�Trim�t���[��
nLastType  = 0           ' ���O��ԃN���A
nPart      = 0           ' ������Ԃ�A�p�[�g
bPartExist = 0           ' ���݂̃p�[�g�͑��݂Ȃ�
nFrmBegin  = 0           ' ����chapter�J�n�n�_

'--- �J�n�n�_�ݒ� ---
' nTrimNum �������F����Trim�J�n�ʒu������
' nTrimNum ����F����Trim�I���ʒu������
If (nTrimTotal > 0) Then
  If (nItemTrim(0) <= nFrmMgn) Then  ' �ŏ��̗����オ���0�t���[���Ɠ��ꎋ
    nTrimNum   = 1
  End If
Else
  nTrimNum   = 1
End If

'--- �\�����f�[�^�����Ԃɓǂݏo�� ---
Do While objStream.AtEndOfLine = false
  strBufRead = objStream.ReadLine
  Set matches = re.Execute(strBufRead)
  If matches.Count > 0 Then
    '--- �ǂݏo���f�[�^�i�[ ---
    nFrmSt = CLng(matches(0).SubMatches(0))     ' �J�n�t���[��
    nFrmEd = CLng(matches(0).SubMatches(1))     ' �I���t���[��
    nSecRd = matches(0).SubMatches(2)           ' ���ԕb��
    strCmt = matches(0).SubMatches(5)           ' �\���R�����g
    '--- ���݌�������Trim�ʒu�f�[�^�擾 ---
    If nTrimNum < nTrimTotal Then
      nFrmTrim = nItemTrim(nTrimNum)
    End If

    '--- ���\���I���ʒu����O��Trim�n�_������ꍇ�̐ݒ菈�� ---
    Do While nFrmTrim < nFrmEd - nFrmMgn And nTrimNum < nTrimTotal
      bCutOn  = (nTrimNum+1) Mod 2              ' Trim�̃J�b�g��ԁi�P�ŃJ�b�g�j
      '--- CHAPTER������擾���� ---
      nType = ProcChapterTypeTerm(nSecCalc, nFrmBegin, nFrmTrim)
      strChapterName = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc)
      '--- CHAPTER�}������ ---
      Call clsChapter.InsertFrame(nFrmBegin, bCutOn, strChapterName)
      nFrmBegin = nFrmTrim                      ' chapter�J�n�ʒu�ύX
      nTrimNum = nTrimNum + 1                   ' Trim�ԍ������Ɉڍs
      If nTrimNum < nTrimTotal Then
        nFrmTrim = nItemTrim(nTrimNum)          ' ����Trim�ʒu�����ɕύX
      End If
    Loop

    '--- ���\���ʒu�̔��f�J�n ---
    bShowPre = 0
    bShowOn = 0
    bCutOn  = (nTrimNum+1) Mod 2                ' Trim�̃J�b�g��ԁi�P�ŃJ�b�g�j
    '--- ���I���ʒu��Trim�n�_�����邩���f�i�����CHAPTER�\���m��j ---
    If (nFrmTrim <= nFrmEd + nFrmMgn) And (nTrimNum < nTrimTotal) Then
      nFrmEd  = nFrmTrim              ' Trim�ʒu�Ƀt���[����ύX
      bShowOn = 1                     ' �\�����s��
      nTrimNum = nTrimNum + 1         ' Trim�ʒu�����Ɉڍs
    End If

    '--- �R�����g����CHAPTER�\����ނ𔻒f ---
    ' nType 0:�X���[ 1:CM���� 10:�Ɨ��\�� 11:part�����ɂ��Ȃ��Ɨ��\��
    nType = ProcChapterTypeCmt(strCmt, nSecRd)
    '--- CHAPTER��؂���m�F�i�O��ƍ���̍\���ŋ�؂邩���f�j ---
    If bCutOn <> 0 Then                  ' �J�b�g���镔��
      If nType = 1 Then                  ' �����I��CM��
        If nLastType <> 1 Then           ' �O��CM�ȊO�������ꍇ�\��
          bShowPre = 1                   ' �O��I���i����J�n�j��chapter�\��
        End If
      Else                               ' �����I��CM�ȊO
        If nLastType = 1 Then            ' �O��CM�������ꍇ�\��
          bShowPre = 1                   ' �O��I���i����J�n�j��chapter�\��
        End If
      End If
    End If

    '--- CHAPTER�}���i�O��I���ʒu�j ---
    If bShowPre > 0 Or nType >= 10 Then      ' �ʒu�m��̃t���O�m�F
      If nFrmBegin < nFrmSt - nFrmMgn Then   ' chapter�J�n�ʒu������J�n���O
        If nLastType <> 1 Then               ' �O��CM�ȊO�̎��͎�ލĊm�F
          nLastType = ProcChapterTypeTerm(nSecCalc, nFrmBegin, nFrmSt)
        End If
        '--- CHAPTER������������肵�}�� ---
        strChapterLast = ProcChapterName(bCutOn, nLastType, nPart, bPartExist, nSecCalc)
        Call clsChapter.InsertFrame(nFrmBegin, bCutOn, strChapterLast)
        nFrmBegin = nFrmSt                   ' chapter�J�n�ʒu������J�n�ʒu��
      End If
    End If
    '--- CHAPTER�}���i���I���ʒu�j ---
    If bShowOn > 0 Or nType >= 10 Then
      If nFrmEd > nFrmBegin + nFrmMgn Then   ' chapter�J�n�ʒu������I�����O
        '--- CHAPTER������������肵�}�� ---
        strChapterName = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecRd)
        Call clsChapter.InsertFrame(nFrmBegin, bCutOn, strChapterName)
        nFrmBegin = nFrmEd                   ' chapter�J�n�ʒu������I���ʒu��
      End If
    End If

    '--- ����m�F�p�̏��� ---
    nLastType = nType

  End If
  Set matches = Nothing
Loop

'--- Trim�ʒu�̏o�͊������Ă��Ȃ��ꍇ�̏��� ---
Do While nTrimNum < nTrimTotal
  nFrmTrim = nItemTrim(nTrimNum)

  '--- Trim�ʒu��chapter�֏o�� ---
  bCutOn  = (nTrimNum+1) Mod 2                   ' Trim�̃J�b�g��ԁi�P�ŃJ�b�g�j
  nType = ProcChapterTypeTerm(nSecCalc, nFrmBegin, nFrmTrim)
  strChapterName = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc)
  '--- CHAPTER�}������ ---
  Call clsChapter.InsertFrame(nFrmBegin, bCutOn, strChapterName)
  nTrimNum = nTrimNum + 1                            ' Trim�ԍ������Ɉڍs
Loop

'--- �ŏIchapter�̏o�� ---
If nFrmBegin < nFrmEd - nFrmMgn Then
  bCutOn  = (nTrimNum+1) Mod 2                   ' Trim�̃J�b�g��ԁi�P�ŃJ�b�g�j
  nType = ProcChapterTypeTerm(nSecCalc, nFrmBegin, nFrmEd)
  strChapterName = ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecCalc)
  '--- CHAPTER�}������ ---
  Call clsChapter.InsertFrame(nFrmBegin, bCutOn, strChapterName)
End If

'--- ���ʏo�� ---
Call clsChapter.OutputChapter(nOutFormat)

'--- �t�@�C���N���[�Y ---
objStream.Close
Set objStream = Nothing
Set objFileSystem  = Nothing

Set clsChapter = Nothing

'--- ���� ---


'--------------------------------------------------
' Chapter��ނ��擾�i�J�n�I���ʒu����b�����擾����j
'   nSecRd : �i�o�́j���ԕb��
'   nFrmS  : �J�n�t���[��
'   nFrmE  : �I���t���[��
'  �o��
'   nType  : 0:�ʏ� 1:�����I��CM 2:part�����̔��f�����\��
'            10:�P�ƍ\�� 11:part�����̔��f�����P�ƍ\�� 12:��
'--------------------------------------------------
Function ProcChapterTypeTerm(nSecRd, nFrmS, nFrmE)
  Dim nType

  nSecRd = ProcGetSec(nFrmE - nFrmS)
  If nSecRd = 0 Then
    nType = 12
  ElseIf nSecRd = 90 Then
    nType = 11
  ElseIf CInt(nSecRd) < 15 Then
    nType = 2
  Else
    nType = 0
  End If

  ProcChapterTypeTerm = nType
End Function


'--------------------------------------------------
' Chapter��ނ��擾�i�R�����g�����g�p����j
'   strCmt : �R�����g������
'   nSecRd : �R�����g�̕b��
'  �o��
'   nType  : 0:�ʏ� 1:�����I��CM 2:part�����̔��f�����\��
'            10:�P�ƍ\�� 11:part�����̔��f�����P�ƍ\�� 12:��
'--------------------------------------------------
Function ProcChapterTypeCmt(strCmt, nSecRd)
  Dim nType

  '--- CHAPTER�\�����e�����f ---
  ' nType  : 0:�ʏ� 1:�����I��CM 2:part�����̔��f�����\��
  '          10:�P�ƍ\�� 11:part�����̔��f�����P�ƍ\�� 12:��
  If InStr(strCmt, "Trailer(cut)") > 0 Then
    nType   = 0
  ElseIf InStr(strCmt, "Trailer") > 0 Then
    nType   = 10
  ElseIf InStr(strCmt, "Sponsor") > 0 Then
    nType   = 11
  ElseIf InStr(strCmt, "Endcard") > 0 Then
    nType   = 11
  ElseIf InStr(strCmt, "Edge") > 0 Then
    nType   = 11
  ElseIf InStr(strCmt, "Border") > 0 Then
    nType   = 11
  ElseIf InStr(strCmt, "CM") > 0 Then
    nType   = 1             ' 15�b�P��CM�Ƃ���ȊO�𕪂���K�v�Ȃ����0�ɂ���
  ElseIf nSecRd = 90 Then
    nType   = 11
  ElseIf nSecRd = 60 Then
    nType   = 10
  ElseIf CInt(nSecRd) < 15 Then
    nType   = 2
  Else
    nType   = 0
  End If

  ProcChapterTypeCmt = nType
End Function


'--------------------------------------------------
' CHAPTER���̕���������߂�
'   bCutOn : 0=�J�b�g���Ȃ����� 1=�J�b�g����
'   nType  : 0:�ʏ� 1:�����I��CM 2:part�����̔��f�����\��
'            10:�P�ƍ\�� 11:part�����̔��f�����P�ƍ\�� 12:��
'   nPart  : A�p�[�g���珇�Ԃɐ���0�`�ifunction���ōX�V����j
'   bPartExist : part�\���̗v�f�������2�ifunction���ōX�V����j
'   nSecRd     : �P�ƍ\�����̕b��
' �߂�l��CHAPTER��
'--------------------------------------------------
Function ProcChapterName(bCutOn, nType, nPart, bPartExist, nSecRd)
  Dim strChapterName

  If bCutOn = 0 Then                           ' �c������
    strChapterName = Chr(Asc("A") + (nPart Mod 23))
    If nType >= 10 Then
      strChapterName = strChapterName & nSecRd & "Sec"
    Else
      strChapterName = strChapterName
    End If
    If nType = 11 Or nType = 2 Then            ' part�����̔��f�����\��
      If bPartExist = 0 Then
        bPartExist = 1
      End If
    ElseIf nType <> 12 Then
      bPartExist = 2
    End If
  Else                                         ' �J�b�g���镔��
    If nType >= 10 Then
      strChapterName = "X" & nSecRd & "Sec"
    ElseIf nType = 1 Then
      strChapterName = "XCM"
    Else
      strChapterName = "X"
    End If
    If bPartExist > 0 And nType <> 12 Then
      nPart = nPart + 1
      bPartExist = 0
    End If
  End If
  ProcChapterName = strChapterName
End Function


'--------------------------------------------------
' �t���[�����ɑΉ�����b���擾
'--------------------------------------------------
Function ProcGetSec(nFrame)
  '29.97fps�̐ݒ�ŌŒ�
  ProcGetSec = Int((CLng(nFrame) * 1001 + 30000/2) / 30000)
End Function


'--------------------------------------------------
' CHAPTER�i�[�p�N���X
'  InsertMSec     : CHAPTER�ɒǉ��i�~���b�Ŏw��j
'  InsertFrame    : CHAPTER�ɒǉ��i�t���[���ʒu�w��j
'  OutputChapter  : CHAPTER����W���o�͂ɏo��
'--------------------------------------------------
Class ChapterData
  Private m_nMaxList        ' ���݂̊i�[�ő�
  Private m_nList           ' CHAPTER�i�[��
  Private m_nMSec()         ' CHAPTER�ʒu�i�~���b�P�ʁj
  Private m_bCutOn()        ' 0:�J�b�g���Ȃ��ʒu 1:�J�b�g�ʒu
  Private m_strName()       ' �`���v�^�[��
  Private m_strOutput       ' �o�͊i�[

  Private Sub Class_Initialize()
    m_nMaxList = 0
    m_nList    = 0
    m_strOutput = ""
  End Sub

  Private Sub Class_Terminate()
  End Sub

  '------------------------------------------------------------
  ' CHAPTER�\���p��������P���쐬�im_strOutput�Ɋi�[�j
  ' num     : �i�[chapter�ʂ��ԍ�
  ' nCount  : �o�͗pchapter�ԍ�
  ' nTime   : �ʒu�~���b�P��
  ' strName : chapter��
  '------------------------------------------------------------
  Private Sub GetDispChapter(num, nCount, nTime, strName)
    Dim strBuf
    Dim strCount, strTime
    Dim strHour, strMin, strSec, strMsec
    Dim nHour, nMin, nSec, nMsec

    '--- �`���v�^�[�ԍ� ---
    strCount = CStr(nCount)
    If (Len(strCount) = 1) Then
      strCount = "0" & strCount
    End If
    '--- �`���v�^�[���� ---
    nHour = Int(nTime / (60*60*1000))
    nMin  = Int((nTime Mod (60*60*1000)) / (60*1000))
    nSec  = Int((nTime Mod (60*1000))    / 1000)
    nMsec = nTime Mod 1000
    strHour = Right("0" & CStr(nHour), 2)
    strMin  = Right("0" & CStr(nMin),  2)
    strSec  = Right("0" & CStr(nSec),  2)
    strMsec = Right("00" & CStr(nMsec), 3)
    StrTime = strHour & ":" & strMin & ":" & strSec & "." & strMsec
    '--- �o�͕�����i�P�s�ځj ---
    strBuf = "CHAPTER" & strCount & "=" & strTime & vbCRLF
    '--- �o�͕�����i�Q�s�ځj ---
    strBuf = strBuf & "CHAPTER" & strCount & "NAME=" & strName & vbCRLF
    m_strOutput = m_strOutput & strBuf
  End Sub


  '---------------------------------------------
  ' CHAPTER�ɒǉ��i�~���b�Ŏw��j
  ' nMSec   : �ʒu�~���b
  ' bCutOn  : 1�̎��J�b�g
  ' strName : chapter�\���p������
  '---------------------------------------------
  Public Sub InsertMSec(nMSec, bCutOn, strName)
    If m_nList >= m_nMaxList Then      ' �z�񖞔t���͍Ċm��
      m_nMaxList = m_nMaxList + 100
      ReDim Preserve m_nMSec(m_nMaxList)
      ReDim Preserve m_bCutOn(m_nMaxList)
      ReDim Preserve m_strName(m_nMaxList)
    End If
    m_nMSec(m_nList)   = nMSec
    m_bCutOn(m_nList)  = bCutOn
    m_strName(m_nList) = strName
    m_nList = m_nList + 1
  End Sub

  '---------------------------------------------
  ' CHAPTER�ɒǉ��i�t���[���ʒu�w��j
  ' nFrame  : �t���[���ʒu
  ' bCutOn  : 1�̎��J�b�g
  ' strName : chapter�\���p������
  '---------------------------------------------
  Public Sub InsertFrame(nFrame, bCutOn, strName)
    Dim nTmp
    '29.97fps�̐ݒ�ŌŒ�
    nTmp = Int((CLng(nFrame) * 1001 + 30/2) / 30)
    Call InsertMSec(nTmp, bCutOn, strName)
  End Sub


  '---------------------------------------------
  ' CHAPTER����W���o�͂ɏo��
  ' nCutType : MODE_ORG / MODE_CUT / MODE_TVT / MODE_TVC
  '---------------------------------------------
  Public Sub OutputChapter(nCutType)
    Dim i, inext, nCount
    Dim bCutState, bSkip
    Dim nSumTime
    Dim strName

    nSumTime  = CLng(0)      ' ���݂̈ʒu�i�~���b�P�ʁj
    nCount    = 1            ' CHAPTER�o�͔ԍ�
    bCutState = 0            ' �O��̏�ԁi0:��J�b�g�p 1:�J�b�g�p�j
    m_strOutput = ""         ' �o��
    '--- tvtplay�p���������� ---
    If nCutType = MODE_TVT Or nCutType = MODE_TVC Then
      m_strOutput = "c-"
    End If

    '--- CHAPTER�ݒ萔�����J��Ԃ� ---
    inext = 0
    For i=0 To m_nList - 1
      '--- ����CHAPTER�Əd�Ȃ��Ă���ꍇ�͏��� ---
      bSkip = 0
      If (inext > i) Then
        bSkip = 1
      Else
        inext = i+1
        If (inext < m_nList-1) Then
          If (m_nMSec(inext+1) - m_nMSec(inext) < MSEC_DIVMIN) Then
            inext = inext + 1
          End If
        End If
      End If
      If (bSkip = 0) Then
        '--- �S���\�����[�hor�J�b�g���Ȃ��ʒu�̎��ɏo�� ---
        If nCutType = MODE_ORG Or nCutType = MODE_TVT Or m_bCutOn(i) = 0 Then
          '--- �ŏ���0�łȂ����̕␳ ---
          If nCutType = MODE_ORG Or nCutType = MODE_TVT Then
            If i = 0 And m_nMSec(i) > 0 Then
              nSumTime  = nSumTime + m_nMSec(i)
            End If
          End If
          '--- tvtplay�p ---
          If nCutType = MODE_TVT Or nCutType = MODE_TVC Then
            '--- CHAPTER����ݒ� ---
            If nCutType = MODE_TVC Then                    ' �J�b�g�ς�
              If bCutState > 0 And m_bCutOn(i) = 0 Then    ' �J�b�g�I��
                strName = m_strName(i) & SUFFIX_CUTO
              Else
                strName = m_strName(i)
              End If
            ElseIf bCutState = 0 And m_bCutOn(i) > 0 Then  ' �J�b�g�J�n
              strName = PREFIX_TVTI & m_strName(i)
            ElseIf bCutState > 0 And m_bCutOn(i) = 0 Then  ' �J�b�g�I��
              strName = PREFIX_TVTO & m_strName(i)
            Else
              strName = m_strName(i)
            End If
            strName = Replace(strName, "-", "�|")
            '--- tvtplay�pCHAPTER�o�͕�����ݒ� ---
            m_strOutput = m_strOutput & nSumTime & "c" & strName & "-"
          '--- �ʏ��chapter�p ---
          Else
            '--- CHAPTER����ݒ� ---
            If bCutState = 0 And m_bCutOn(i) > 0 Then      ' �J�b�g�J�n
              strName = PREFIX_ORGI & m_strName(i)
            ElseIf bCutState > 0 And m_bCutOn(i) = 0 Then  ' �J�b�g�I��
              If nCutType = MODE_CUT Then
                strName = PREFIX_CUTO & m_strName(i) & SUFFIX_CUTO
              Else
                strName = PREFIX_ORGO & m_strName(i)
              End If
            Else
              strName = m_strName(i)
            End If
            '--- CHAPTER�o�͕�����ݒ� ---
            Call GetDispChapter(i, nCount, nSumTime, strName)
          End If
          '--- �������݌㋤�ʐݒ� ---
          nSumTime  = nSumTime + (m_nMSec(inext) - m_nMSec(i))
          nCount    = nCount + 1
        End If
        '--- ��CHAPTER�ɏ�ԍX�V ---
        bCutState = m_bCutOn(i)
      End If
    Next

    '--- tvtplay�p�ŏI������ ---
    If nCutType = MODE_TVT Then
      If bCutState > 0 Then   ' CM�I������
        m_strOutput = m_strOutput & "0e" & PREFIX_TVTO & "-"
      Else
        m_strOutput = m_strOutput & "0e-"
      End If
      m_strOutput = m_strOutput & "c"
    ElseIf nCutType = MODE_TVC Then
      m_strOutput = m_strOutput & "c"
    End If
    '--- ���ʏo�� ---
    WScript.StdOut.Write m_strOutput
  End Sub
End Class
