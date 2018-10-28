//
// ロゴ表示区間と無音シーンチェンジ情報からCMカット情報を作成
// 必要なもの
//    logoframe結果のロゴ表示区間情報
//    chapter_exe結果の無音シーンチェンジ情報
//    CMカット内容を記載した実行内容スクリプト
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include "join_logo_scp.h"
#include "join_logo_scp_extern.h"

//---------------------------------------------------------------------
// ロゴ表示期間情報を読み込み（ファイル名fnameの内容を取得）
//---------------------------------------------------------------------
int read_logoframe(LOGO_RESULTREC *plogo, const char *fname){
	FILE *fp;
	int num_find, set_rise, pos, line;
	int n1, n2, n3, interlace, fade, flag_start;
	char buf[SIZE_BUF_MAX];
	char str[SIZE_BUF_MAX];

	num_find = 0;
	set_rise = 0;
	line = 0;
	fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "error: failed to open '%s'\n", fname);
		return 2;
	}
	while( fgets(buf, SIZE_BUF_MAX, fp) != NULL && num_find < LOGO_FIND_MAX ){
		line ++;
		if (buf[0] != '#'){
			pos = 0;
			pos = get_str_num(&n1, buf, pos);		// get frame number
			if (pos >= 0){
				fade = 0;
				interlace = 0;
				n2 = n1;
				n3 = n1;
				pos = get_str(str, buf, pos);	// get S or E
				if (pos < 0){
					if (set_rise == 0){
						strcpy(str, "S");
					}
					else{
						strcpy(str, "E");
					}
				}
				if (str[0] == 'S'){
					flag_start = 1;
				}
				else if (str[0] == 'E'){
					flag_start = 0;
				}
				else{
					flag_start = -1;
				}

				pos = get_str_num(&fade, buf, pos);		// get fade
				pos = get_str(str, buf, pos);			// get interlace
				if (pos >= 0){
					if (str[0] == 'T'){
						interlace = 1;
					}
					else if (str[0] == 'B'){
						interlace = 2;
					}
				}
				pos = get_str_num(&n2, buf, pos);		// get frame-left
				pos = get_str_num(&n3, buf, pos);		// get frame-right

				if (flag_start > 0 && set_rise == 0){		// set start edge
					plogo->frm_rise[num_find]   = n1;
					plogo->frm_rise_l[num_find] = n2;
					plogo->frm_rise_r[num_find] = n3;
					plogo->fade_rise[num_find]  = fade;
					plogo->intl_rise[num_find]  = interlace;
					set_rise = 1;
				}
				else if (flag_start == 0 && set_rise > 0){	// set end edge
					plogo->frm_fall[num_find]   = n1;
					plogo->frm_fall_l[num_find] = n2;
					plogo->frm_fall_r[num_find] = n3;
					plogo->fade_fall[num_find]  = fade;
					plogo->intl_fall[num_find]  = interlace;
					set_rise = 0;
					num_find ++;
				}
				else{
					fprintf(stderr, "error:ignored line%d:'%s'\n", line, buf);
				}
			}
		}
	}
	if (set_rise > 0 && num_find > 0){		// ignore if not found end edge
		num_find --;
	}
	plogo->num_find = num_find;
	fclose(fp);

	if (plogo->num_find == 0){
		fprintf(stderr, "warning: no logo information found in '%s'\n", fname);
	}
	return 0;
}

//---------------------------------------------------------------------
// 無音シーンチェンジ情報を読み込み（ファイル名fnameの内容を取得）
//---------------------------------------------------------------------
int read_scpos(LOGO_RESULTREC *plogo, const char *fname){
	FILE *fp;
	int j;
	int num_scpos, pos;
	int tmp_scpos;
	int frm_totalmax;
	int frm_smute_s, frm_smute_w;
	char buf[SIZE_BUF_MAX];
	char *ptstr;
	char *ptstr2;

	frm_totalmax = 0;

	// 最初は0フレームとする
	num_scpos = 0;
	plogo->frm_scpos_s[num_scpos] = 0;
	plogo->frm_scpos_e[num_scpos] = 0;
	plogo->frm_smute_s[num_scpos] = -1;
	plogo->frm_smute_e[num_scpos] = -1;
	num_scpos ++;

	// 無音範囲情報初期化
	frm_smute_s = -1;
	frm_smute_w = -1;

	fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "error: failed to open '%s'\n", fname);
		return 2;
	}
	while( fgets(buf, SIZE_BUF_MAX, fp) != NULL && num_scpos < SCPOS_FIND_MAX ){
		// 無音範囲情報
		ptstr = strstr(buf, "NAME=");
		if (ptstr != NULL){
			pos = get_str_numhead(&frm_smute_w, ptstr, 5);
		}
		else{
			ptstr = strstr(buf, "CHAPTER");
			if (ptstr != NULL){
				pos = 7;
				while(ptstr[pos] >= '0' && ptstr[pos] <= '9') pos ++;
				if (ptstr[pos] == '='){
					pos ++;
					pos = get_str_frm(&frm_smute_s, ptstr, pos);
				}
			}
		}

		// シーンチェンジ情報
		ptstr = strstr(buf, "SCPos:");
		if (ptstr != NULL){
			pos = get_str_frml(&(plogo->frm_scpos_s[num_scpos]), ptstr, 6);
			if (pos >= 0){
				pos = get_str_frml(&(plogo->frm_scpos_e[num_scpos]), ptstr, pos);
				if (pos < 0){
					plogo->frm_scpos_e[num_scpos] = plogo->frm_scpos_s[num_scpos] - 1;
				}
				if (frm_totalmax < plogo->frm_scpos_e[num_scpos]){
					frm_totalmax = plogo->frm_scpos_e[num_scpos];
				}
				// シーンチェンジ変化情報
				plogo->still_scpos[num_scpos] = 0;
				ptstr2 = strstr(buf, "＿");				// マーク検出
				if (ptstr2 != NULL && ptstr2 < ptstr){	// マークがSCPos:より前
					plogo->still_scpos[num_scpos] = 1;	// シーンチェンジ変化なしフラグ付加
				}

				// 無音範囲情報設定
				if (frm_smute_s >= 0 && frm_smute_w >= 0){
					plogo->frm_smute_s[num_scpos] = frm_smute_s;
					plogo->frm_smute_e[num_scpos] = frm_smute_s + frm_smute_w;
					// 連続無音区間の確認および追加
					j = num_scpos - 1;
					while(j > 0){
						if (plogo->frm_smute_e[j] + 1 >= plogo->frm_smute_s[num_scpos]){
							if (plogo->frm_smute_s[j] < plogo->frm_smute_s[num_scpos]){
								plogo->frm_smute_s[num_scpos] = plogo->frm_smute_s[j];
							}
							if (plogo->frm_smute_e[j] < plogo->frm_smute_e[num_scpos]){
								plogo->frm_smute_e[j] = plogo->frm_smute_e[num_scpos];
							}
						}
						else{
							j = 0;
						}
						j --;
					}
				}
				else{
					plogo->frm_smute_s[num_scpos] = -1;
					plogo->frm_smute_e[num_scpos] = -1;
				}
				frm_smute_s = -1;
				frm_smute_w = -1;

				// シーンチェンジ重複時の処理
				// シーンチェンジはfrm_scpos_eが前の終了地点のため、frm_scpos_sが後になる
				// 無音区間はfrm_smute_sが開始地点、frm_smute_eが終了地点のためこの順番
				if (num_scpos > 1 && plogo->frm_scpos_e[num_scpos] != plogo->frm_scpos_s[num_scpos]){
					// ２領域の無音区間でシーンチェンジが重なった場合
					if (((plogo->frm_scpos_e[num_scpos-1] <= plogo->frm_scpos_e[num_scpos]) &&
						 (plogo->frm_scpos_s[num_scpos-1] >= plogo->frm_scpos_e[num_scpos])) ||
						((plogo->frm_scpos_e[num_scpos-1] <= plogo->frm_scpos_s[num_scpos]) &&
						 (plogo->frm_scpos_s[num_scpos-1] >= plogo->frm_scpos_s[num_scpos]))){
						// 少しだけずれていた場合は後側のシーンチェンジ位置を有効にする
						if (plogo->frm_scpos_e[num_scpos-1] < plogo->frm_scpos_e[num_scpos]){
							plogo->frm_scpos_e[num_scpos-1] = plogo->frm_scpos_e[num_scpos];
							plogo->frm_scpos_s[num_scpos-1] = plogo->frm_scpos_s[num_scpos];
						}
						// 無音区間の結合
						if (plogo->frm_smute_s[num_scpos-1] > plogo->frm_smute_s[num_scpos]){
							plogo->frm_smute_s[num_scpos-1] = plogo->frm_smute_s[num_scpos];
						}
						if (plogo->frm_smute_e[num_scpos-1] < plogo->frm_smute_e[num_scpos]){
							plogo->frm_smute_e[num_scpos-1] = plogo->frm_smute_e[num_scpos];
						}
						num_scpos --;
					}
				}

				num_scpos ++;
			}
		}
	}
	// 最終シーンチェンジ補正
	if (num_scpos > 2){
		tmp_scpos = num_scpos-1;
		plogo->frm_scpos_s[tmp_scpos] -= plogo->frm_lastcut;
		plogo->frm_scpos_e[tmp_scpos] -= plogo->frm_lastcut;
		frm_totalmax -= plogo->frm_lastcut;
		for(j=1; j<num_scpos-1; j++){
			if (plogo->frm_scpos_s[j] > frm_totalmax){
				plogo->frm_scpos_s[j] = frm_totalmax;
			}
			if (plogo->frm_scpos_e[j] > frm_totalmax){
				plogo->frm_scpos_e[j] = frm_totalmax;
			}
			if (plogo->frm_smute_s[j] > frm_totalmax){
				plogo->frm_smute_s[j] = frm_totalmax;
			}
			if (plogo->frm_smute_e[j] > frm_totalmax){
				plogo->frm_smute_e[j] = frm_totalmax;
			}
		}
		if (plogo->frm_smute_s[tmp_scpos] < 0 && plogo->frm_smute_e[tmp_scpos] < 0){
			plogo->frm_smute_s[tmp_scpos] = plogo->frm_scpos_e[tmp_scpos];
			plogo->frm_smute_e[tmp_scpos] = plogo->frm_scpos_e[tmp_scpos];
		}
	}
	plogo->num_scpos = num_scpos;
	plogo->frm_totalmax = frm_totalmax;

	fclose(fp);
	return 0;
}


//---------------------------------------------------------------------
// 変数を設定
//   type : 0=通常設定  1=未定義時のみ設定
//---------------------------------------------------------------------
int set_regvar(REGVARREC *regvar, const char *strname, const char *strval, int type){
	int i;
	int n;

	// 文字列長の制約チェック
	if (strlen(strname) >= SIZE_VARNAME_MAX || strlen(strval) >= SIZE_VARVAL_MAX){
		fprintf(stderr, "too long string length (%s)or(%s)\n", strname, strval);
		return -1;
	}

	// 既存変数の書き換えかチェック
	n = regvar->var_num;
	if (n > SIZE_VARNUM_MAX){
		n = SIZE_VARNUM_MAX;
	}
	for(i=regvar->var_num-1; i >= 0; i--){
		if (stricmp(strname, regvar->var_name[i]) == 0){
			n = i;
		}
	}
	if (n == SIZE_VARNUM_MAX){		// 変数が定義最大数を超える設定だったら失敗
		fprintf(stderr, "exceed limit number of variable(%d)\n", SIZE_VARNUM_MAX);
		return -1;
	}
	if (type == 0 || n == regvar->var_num){
		strcpy(regvar->var_name[n], strname);
		strcpy(regvar->var_val[n], strval);
		if (n == regvar->var_num){	// 新規書き込み
			regvar->var_num += 1;
		}
	}

	return 0;
}

//---------------------------------------------------------------------
// 変数に設定されているフレーム数を取得
//---------------------------------------------------------------------
int get_regvar_frm(int *result, const REGVARREC *regvar, const char *strname){
	int i;
	int n, ret;

	ret = -1;
	n = -1;
	for(i=regvar->var_num-1; i >= 0; i--){
		if (stricmp(strname, regvar->var_name[i]) == 0){
			n = i;
		}
	}
	if (n >= 0){
		if (get_str_frm(result, regvar->var_val[n], 0) >= 0){
			ret = 1;
		}
	}
	return ret;
}


//---------------------------------------------------------------------
// 文字列から数値および単語文字列を取得
// 出力：
//   dst : 取得文字列
//   val : 取得フレーム数（文字列が数値以外の時、フラグ定義時は1、未定義時は0）
// 返り値：読み終わった位置（失敗時は-1）
//---------------------------------------------------------------------
int cmd_analyze_flags_word(char *dst, int *val, const REGVARREC *regvar, const char *buf, int pos){
	int i;
	int npos;
	int inv;

	inv = 0;
	if (pos >= 0){
		while( buf[pos] == ' ' ){
			pos ++;
		}
		if (buf[pos] == '!'){		// 論理反転
			inv = 1;
			pos ++;
		}
	}

	npos = get_str_frm(val, buf, pos);			// 数値取得
	pos = get_str_word(dst, buf, pos);			// 文字列取得
	if (pos >= 0){
		if (npos < 0){			// 数値以外の時は変数定義されているか取得
			*val = 0;
			for(i=0; i<regvar->var_num; i++){
				if (!stricmp(dst, regvar->var_name[i])){
					if (stricmp(regvar->var_val[i], "0")){	// "0"以外
						*val = 1;
					}
				}
			}
		}
	}
	if (inv > 0){					// 論理反転処理
		if (*val == 0){
			*val = 1;
		}
		else{
			*val = 0;
		}
	}
	return pos;
}

//---------------------------------------------------------------------
// 文字列から比較演算命令を取得
//---------------------------------------------------------------------
int cmd_analyze_flags_opget(const char *str){
	int op;

	if (!strcmp(str, "==")){
		op = 1;
	}
	else if (!strcmp(str, "!=")){
		op = 2;
	}
	else if (!strcmp(str, "<")){
		op = 3;
	}
	else if (!strcmp(str, "<=")){
		op = 4;
	}
	else if (!strcmp(str, ">")){
		op = 5;
	}
	else if (!strcmp(str, ">=")){
		op = 6;
	}
	else if (str[0] == '=' || str[0] == '<' || str[0] == '>'){
		op = -1;
	}
	else{
		op = 0;
	}
	return op;
}

//---------------------------------------------------------------------
// 比較演算実施
//---------------------------------------------------------------------
int cmd_analyze_flags_opcmp(int val1, int val2, int op){
	int res;

	res = 0;
	if (op == 1){
		if (val1 == val2) res = 1;
	}
	else if (op == 2){
		if (val1 != val2) res = 1;
	}
	else if (op == 3){
		if (val1 < val2) res = 1;
	}
	else if (op == 4){
		if (val1 <= val2) res = 1;
	}
	else if (op == 5){
		if (val1 > val2) res = 1;
	}
	else if (op == 6){
		if (val1 >= val2) res = 1;
	}
	return res;
}


//---------------------------------------------------------------------
// 指定フラグがオプションで設定されているかチェック
// 返り値：
//   0 : 該当せず（または設定値0）
//   1 : 該当あり
//---------------------------------------------------------------------
int cmd_analyze_flags(const REGVARREC *regvar, const char *buf, int pos){
	int exist;
	int val, val_last, op;
	char substr[SIZE_BUF_MAX];

	exist = 0;
	op = 0;
	val_last = 0;
	while(pos >= 0 && exist == 0){
		pos = cmd_analyze_flags_word(substr, &val, regvar, buf, pos);
		if (pos >= 0){
			if (op == 0){
				op = cmd_analyze_flags_opget(substr);
				// 演算子以外であれば１回前の値を確定
				if (op == 0){
					if (val_last != 0){
						exist = 1;
					}
					val_last = val;
				}
				else if (op < 0){
					fprintf(stderr, "wrong operator %s in %s\n", substr, buf);
				}
			}
			else{			// 比較演算実行
				val = cmd_analyze_flags_opcmp(val_last, val, op);
				if (val != 0){
					exist = 1;
				}
				op = 0;
				val_last = 0;
			}
		}
		else{						// 最後の処理
			if (val_last != 0){
				exist = 1;
			}
			if (op != 0){
				fprintf(stderr, "wrong operator exist in %s\n", buf);
			}
		}
	}

	return exist;
}

//---------------------------------------------------------------------
// 文字列に変数が入っていれば置換する
//---------------------------------------------------------------------
int cmd_analyze_replacestr(char *dstbuf, const char *srcbuf, const REGVARREC *regvar){
	int i,j;
	int num, nlen, vlen, tmplen, flag;
	int through;
	int len, pos, ret;

	through = 0;
	len = 0;
	pos = 0;
	ret = 0;
	while(len < SIZE_BUF_MAX-1 && srcbuf[pos] != '\0'){
		if (srcbuf[pos] == '#'){	// コメント
			through = 1;
		}
		if (srcbuf[pos] != '$'){
			dstbuf[len++] = srcbuf[pos++];
		}
		else{
			num = -1;
			nlen = 0;
			if (srcbuf[pos+1] != '{'){			// 通常
				for(i=0; i<regvar->var_num; i++){
					tmplen = strlen(regvar->var_name[i]);
					if (tmplen > nlen && tmplen < SIZE_VARNAME_MAX){
						if (strnicmp(&srcbuf[pos+1], regvar->var_name[i], tmplen) == 0){
							num = i;
							nlen = tmplen;
						}
					}
				}
			}
			else{								// ${変数名} 形式時
				while(srcbuf[pos+2+nlen] != '}' && srcbuf[pos+2+nlen] != '\0'){
					nlen ++;
				}
				if (srcbuf[pos+2+nlen] == '}' && nlen < SIZE_VARNAME_MAX){
					for(i=0; i<regvar->var_num; i++){
						tmplen = strlen(regvar->var_name[i]);
						if (tmplen == nlen){
							if (strnicmp(&srcbuf[pos+2], regvar->var_name[i], tmplen) == 0){
								num = i;
							}
						}
					}
					if (num >= 0){
						nlen += 2;
					}
				}
			}
			if (num >= 0){
				vlen = strlen(regvar->var_val[num]);
				for(j=0; j<vlen; j++){
					if (len < SIZE_BUF_MAX-1){
						dstbuf[len++] = regvar->var_val[num][j];
					}
				}
				pos += nlen+1;
			}
			else{
				dstbuf[len++] = srcbuf[pos++];
				if (through == 0){
					fprintf(stderr, "Not found definition\n");
					ret = -1;
				}
			}
		}
	}
	dstbuf[len] = '\0';
	//--- 最後の空白、特殊文字は削除 ---
	flag = 0;
	while(len > 0 && flag == 0){
		if (dstbuf[len-1] > 0 && dstbuf[len-1] <= ' '){
			len --;
			dstbuf[len] = '\0';
		}
		else{
			flag = 1;
		}
	}

	if (srcbuf[pos] != '\0'){
		fprintf(stderr, "Too long string\n");
		ret = -1;
	}
	return ret;
}


//---------------------------------------------------------------------
// コマンド内容を１行文字列から解析
// 返り値：
//   -1: コマンド異常（オプション）
//   -2: コマンド異常（範囲）
//   -3: コマンド異常（S/E/B選択）
//   -4: コマンド異常（変数関連）
//   -5: コマンド異常（TR/SP/ED選択）
//   0 : コマンド通常受付
//   1 : If文  （条件真の場合は flag_cond=1 を格納）
//   2 : EndIf文
//   3 : Else, ElsIf文  （条件真の場合は flag_cond=1 を格納）
//   11: Call文
//---------------------------------------------------------------------
int cmd_analyze(CMDSETREC *cmdset, REGVARREC *regvar, PRMVARREC *prmvar, const char *buf_org, char ifskip){
	int pos;
	int ndata;
	int retval;
	int lens_num;
	int ntmp;
	int i;
	char buf[SIZE_BUF_MAX];
	char str[SIZE_BUF_MAX];
	char varname[SIZE_VARNAME_MAX];

	// set default value
	retval = 0;
	strcpy(cmdset->cmdname, "");
	cmdset->pos_str = 0;
	cmdset->flag_cond = 0;
	cmdset->flag_shift = 0;
	cmdset->flag_fromabs = 0;
	cmdset->flag_wide = 0;
	cmdset->flag_fromlast = 0;
	cmdset->flag_withp = 0;
	cmdset->flag_withn = 0;
	cmdset->flag_noedge = 0;
	cmdset->flag_overlap = 0;
	cmdset->flag_confirm = 0;
	cmdset->flag_unit = 0;
	cmdset->flag_else = 0;
	cmdset->flag_cont = 0;
	cmdset->flag_reset = 0;
	cmdset->flag_flat = 0;
	cmdset->flag_fr = 0;
	cmdset->flag_fhead = 0;
	cmdset->flag_ftail = 0;
	cmdset->flag_fmid = 0;
	cmdset->flag_force = 0;
	cmdset->flag_autochg = 0;
	cmdset->flag_autoeach = 0;
	cmdset->select_edge = 0;
	cmdset->select_auto = 0;
	cmdset->select_num = 0;
	cmdset->select_nr = 0;
	cmdset->select_frm_left = -1;
	cmdset->select_frm_right = -1;
	cmdset->select_frm_min = -1;
	cmdset->select_frm_max = -1;
	cmdset->lenp_min = -1;
	cmdset->lenp_max = -1;
	cmdset->lenn_min = -1;
	cmdset->lenn_max = -1;
	cmdset->lens_num = 0;
	cmdset->endlen_center = 0;
	cmdset->endlen_left = 0;
	cmdset->endlen_right = 0;
	cmdset->sft_center = 0;
	cmdset->sft_left = 0;
	cmdset->sft_right = 0;
	cmdset->frm_fromabs = -1;
	cmdset->logoext_left = 0;
	cmdset->logoext_right = 0;
	cmdset->autop_code   = 0;
	cmdset->autop_limit  = 0;
	cmdset->autop_scope  = 0;
	cmdset->autop_scopen = 0;
	cmdset->autop_scopex = 0;
	cmdset->autop_period = 0;
	cmdset->autop_maxprd = 0;
	cmdset->autop_secnext = 0;
	cmdset->autop_secprev = 0;
	cmdset->autop_trscope  = 0;
	cmdset->autop_trsumprd = 0;
	cmdset->autop_tr1stprd = 0;

	//--- 変数を置換 ---
	if (cmd_analyze_replacestr(buf, buf_org, regvar) < 0){
		return -4;
	}
//printf("%s\n", buf);

	//--- コマンド読み込み開始 ---
	pos = get_str(str, buf, 0);
	if (pos >= 0 && str[0] != '#'){
		strcpy(cmdset->cmdname, str);				// get command

		//--- If条件分岐、Call分岐用 ---
		if (pos >= 0){
			if (!stricmp(cmdset->cmdname, "If")){			// command:If
				cmdset->flag_cond = cmd_analyze_flags(regvar, buf, pos);
				return 1;
			}
			else if (!stricmp(cmdset->cmdname, "EndIf")){	// command:EndIf
				return 2;
			}
			else if (!stricmp(cmdset->cmdname, "Else")){		// command:Else
				cmdset->flag_cond = 1;
				return 3;
			}
			else if (!stricmp(cmdset->cmdname, "ElsIf")){	// command:ElsIf
				cmdset->flag_cond = cmd_analyze_flags(regvar, buf, pos);
				return 3;
			}
			else if (!stricmp(cmdset->cmdname, "Call")){	// command:Call
				cmdset->pos_str = pos;
				return 11;
			}
			else if (!stricmp(cmdset->cmdname, "Echo")){	// command:Echo
				if (buf[pos] == ' ') pos++;
				printf("%s\n", &buf[pos]);
				return 12;
			}
		}

		//--- Set 変数設定用 ---
		if (!stricmp(cmdset->cmdname, "Set") ||			// command:Set
			!stricmp(cmdset->cmdname, "Default")){		// command:Default
			pos = get_str(str, buf, pos);
			if (strlen(str) < SIZE_VARNAME_MAX){
				strcpy(varname, str);
			}
			else{
				return -4;
			}
			pos = get_str(str, buf, pos);
			if (pos < 0){
				return -4;
			}
			if (ifskip == 0){			// IF内skip中ではない
				if (!stricmp(cmdset->cmdname, "Default")){
					ntmp = 1;
				}
				else{
					ntmp = 0;
				}
				if (set_regvar(regvar, varname, str, ntmp) < 0){
					return -4;
				}
			}
			strcpy(cmdset->cmdname, "");	// コマンド内容クリア
			return 0;						// 変数設定正常終了
		}
		//--- SetParam 変数設定用 ---
		if (!stricmp(cmdset->cmdname, "SetParam")){		// command:SetP
			pos = get_str(str, buf, pos);
			if (strlen(str) < SIZE_VARNAME_MAX){
				strcpy(varname, str);
			}
			else{
				return -4;
			}
			if (!stricmp(varname, "WLogoTRMax")){
				pos = get_str_frml(&(prmvar->frm_wlogo_trmax), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WCompTRMax")){
				pos = get_str_frml(&(prmvar->frm_wcomp_trmax), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WLogoSftMrg")){
				pos = get_str_frml(&(prmvar->frm_wlogo_sftmrg), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WCompLast")){
				pos = get_str_frml(&(prmvar->frm_wcomp_last), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WLogoSumMin")){
				pos = get_str_frml(&(prmvar->frm_wlogo_summin), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WCompSpMin")){
				pos = get_str_secl(&(prmvar->sec_wcomp_spmin), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "WCompSpMax")){
				pos = get_str_secl(&(prmvar->sec_wcomp_spmax), buf, pos);
				if (pos < 0){
					return -4;
				}
			}
			else if (!stricmp(varname, "CutTR")){
				pos = get_str_num(&ndata, buf, pos);
				if (pos >= 0){
					prmvar->flag_cuttr = ndata;
				}
				else{
					return -4;
				}
			}
			else if (!stricmp(varname, "CutSP")){
				pos = get_str_num(&ndata, buf, pos);
				if (pos >= 0){
					prmvar->flag_cutsp = ndata;
				}
				else{
					return -4;
				}
			}
			else if (!stricmp(varname, "AddLogo")){
				pos = get_str_num(&ndata, buf, pos);
				if (pos >= 0){
					prmvar->flag_addlogo = ndata;
				}
				else{
					return -4;
				}
			}
			else if (!stricmp(varname, "AddUC")){
				pos = get_str_num(&ndata, buf, pos);
				if (pos >= 0){
					prmvar->flag_adduc = ndata;
				}
				else{
					return -4;
				}
			}
			else{
				return -4;
			}
			strcpy(cmdset->cmdname, "");	// コマンド内容クリア
			return 0;						// 変数設定正常終了
		}

		if (pos >= 0){
			pos = get_str(str, buf, pos);			// get start or end or both
			if (!stricmp(cmdset->cmdname, "AutoCut")   ||
				!stricmp(cmdset->cmdname, "AutoAdd")){
				if (!stricmp(str, "TR")){
					cmdset->select_auto = D_CMDAUTO_TR;
				}
				else if (!stricmp(str, "SP")){
					cmdset->select_auto = D_CMDAUTO_SP;
				}
				else if (!stricmp(str, "EC")){
					cmdset->select_auto = D_CMDAUTO_EC;
				}
				else{
					return -5;
				}
			}
			else{						// 通常のコマンド
				if (str[0] == 'S' || str[0] == 's'){
					cmdset->select_edge = 0;
				}
				else if (str[0] == 'E' || str[0] == 'e'){
					cmdset->select_edge = 1;
				}
				else if (str[0] == 'B' || str[0] == 'b'){
					cmdset->select_edge = 2;
				}
				else{
					return -3;
				}
			}
			if (!stricmp(cmdset->cmdname, "AutoEdge")){
				cmdset->select_auto += D_CMDAUTO_EDGE + cmdset->select_edge;
			}
			else if (!stricmp(cmdset->cmdname, "AutoCM")){
				cmdset->select_auto = D_CMDAUTO_OTH;
			}
			else if (!stricmp(cmdset->cmdname, "AutoUp")){
				cmdset->select_auto = D_CMDAUTO_OTH;
			}
		}

		if (!stricmp(cmdset->cmdname, "Find")   ||			// command:Find
			!stricmp(cmdset->cmdname, "MkLogo") ||			// command:MkLogo
			!stricmp(cmdset->cmdname, "DivLogo") ||			// command:DivLogo
			!stricmp(cmdset->cmdname, "Select")){			// command:Select
			pos = get_str_frml(&(cmdset->frm_center), buf, pos);
			pos = get_str_frml(&(cmdset->frm_left),   buf, pos);
			pos = get_str_frml(&(cmdset->frm_right),  buf, pos);
			if (pos < 0){
				return -2;
			}
			// 開始と終了が逆の場合は反転
			if (cmdset->frm_left > cmdset->frm_right){
				ntmp = cmdset->frm_left;
				cmdset->frm_left  = cmdset->frm_right;
				cmdset->frm_right = ntmp;
			}
		}
		else if (!stricmp(cmdset->cmdname, "Force")){		// command:Force
			pos = get_str_frml(&(cmdset->frm_center), buf, pos);
			if (pos < 0){
				return -2;
			}
		}

		//get options
		while(pos >= 0 && retval == 0){
			pos = get_str(str, buf, pos);
			if (pos >= 0 && buf[0] == '#'){
				break;
			}
			if (pos >= 0){
				if (!stricmp(str, "-N")){
					pos = get_str_num(&ndata, buf, pos);
					if (pos >= 0){
						cmdset->select_num = ndata;
					}
					else{
						retval = -1;
					}
				}
				else if (!stricmp(str, "-NR")){
					pos = get_str_num(&ndata, buf, pos);
					if (pos >= 0){
						cmdset->select_nr = ndata;
					}
					else{
						retval = -1;
					}
				}
				else if (!stricmp(str, "-F")){
					cmdset->flag_fr = 0;
					pos = get_str_frml(&(cmdset->select_frm_left),  buf, pos);
					pos = get_str_frml(&(cmdset->select_frm_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-FR")){
					cmdset->flag_fr = 1;
					pos = get_str_frml(&(cmdset->select_frm_left),  buf, pos);
					pos = get_str_frml(&(cmdset->select_frm_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-Fhead")){
					cmdset->flag_fhead = 1;
					pos = get_str_frml(&(cmdset->select_frm_left) , buf, pos);
					pos = get_str_frml(&(cmdset->select_frm_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-Ftail")){
					cmdset->flag_ftail = 1;
					pos = get_str_frml(&(cmdset->select_frm_left) , buf, pos);
					pos = get_str_frml(&(cmdset->select_frm_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-Fmid")){
					cmdset->flag_fmid = 1;
					pos = get_str_frml(&(cmdset->select_frm_left) , buf, pos);
					pos = get_str_frml(&(cmdset->select_frm_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-LenP")){
					pos = get_str_frml(&(cmdset->lenp_min), buf, pos);
					pos = get_str_frml(&(cmdset->lenp_max), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-LenN")){
					pos = get_str_frml(&(cmdset->lenn_min), buf, pos);
					pos = get_str_frml(&(cmdset->lenn_max), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-SC")){
					lens_num = cmdset->lens_num;
					if (lens_num < 10){
						pos = get_str_frml(&(cmdset->lens_min[lens_num]), buf, pos);
						pos = get_str_frml(&(cmdset->lens_max[lens_num]), buf, pos);
						cmdset->lens_sctype[lens_num] = 0;
						cmdset->lens_num += 1;
					}
					else{
						pos = -1;
					}
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-NoSC")){
					lens_num = cmdset->lens_num;
					if (lens_num < 10){
						pos = get_str_frml(&(cmdset->lens_min[lens_num]), buf, pos);
						pos = get_str_frml(&(cmdset->lens_max[lens_num]), buf, pos);
						cmdset->lens_sctype[lens_num] = 1;
						cmdset->lens_num += 1;
					}
					else{
						pos = -1;
					}
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-SM")){
					lens_num = cmdset->lens_num;
					if (lens_num < 10){
						pos = get_str_frml(&(cmdset->lens_min[lens_num]), buf, pos);
						pos = get_str_frml(&(cmdset->lens_max[lens_num]), buf, pos);
						cmdset->lens_sctype[lens_num] = 2;
						cmdset->lens_num += 1;
					}
					else{
						pos = -1;
					}
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-NoSM")){
					lens_num = cmdset->lens_num;
					if (lens_num < 10){
						pos = get_str_frml(&(cmdset->lens_min[lens_num]), buf, pos);
						pos = get_str_frml(&(cmdset->lens_max[lens_num]), buf, pos);
						cmdset->lens_sctype[lens_num] = 3;
						cmdset->lens_num += 1;
					}
					else{
						pos = -1;
					}
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-SMA")){
					lens_num = cmdset->lens_num;
					if (lens_num < 10){
						pos = get_str_frml(&(cmdset->lens_min[lens_num]), buf, pos);
						pos = get_str_frml(&(cmdset->lens_max[lens_num]), buf, pos);
						cmdset->lens_sctype[lens_num] = 4;
						cmdset->lens_num += 1;
					}
					else{
						pos = -1;
					}
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-EndLen")){
					pos = get_str_frml(&(cmdset->endlen_center), buf, pos);
					pos = get_str_frml(&(cmdset->endlen_left), buf, pos);
					pos = get_str_frml(&(cmdset->endlen_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-Shift")){
					cmdset->flag_shift = 1;
					pos = get_str_frml(&(cmdset->sft_center), buf, pos);
					pos = get_str_frml(&(cmdset->sft_left), buf, pos);
					pos = get_str_frml(&(cmdset->sft_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-fromabs")){
					cmdset->flag_fromabs = 1;
					pos = get_str_frml(&(cmdset->frm_fromabs), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-logoext")){
					pos = get_str_frml(&(cmdset->logoext_left), buf, pos);
					pos = get_str_frml(&(cmdset->logoext_right), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-code")){
					pos = get_str_numl(&(cmdset->autop_code), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-limit")){
					pos = get_str_numl(&(cmdset->autop_limit), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-scope")){
					pos = get_str_secl(&(cmdset->autop_scope), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-scopen")){
					pos = get_str_secl(&(cmdset->autop_scopen), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-scopex")){
					pos = get_str_secl(&(cmdset->autop_scopex), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-period")){
					pos = get_str_secl(&(cmdset->autop_period), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-maxprd")){
					pos = get_str_secl(&(cmdset->autop_maxprd), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-secnext")){
					pos = get_str_secl(&(cmdset->autop_secnext), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-secprev")){
					pos = get_str_secl(&(cmdset->autop_secprev), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-trscope")){
					pos = get_str_secl(&(cmdset->autop_trscope), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-trsumprd")){
					pos = get_str_secl(&(cmdset->autop_trsumprd), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-tr1stprd")){
					pos = get_str_secl(&(cmdset->autop_tr1stprd), buf, pos);
					if (pos < 0){
						retval = -1;
					}
				}
				else if (!stricmp(str, "-wide")){
					cmdset->flag_wide = 1;
				}
				else if (!stricmp(str, "-fromlast")){
					cmdset->flag_fromlast = 1;
				}
				else if (!stricmp(str, "-withp")){
					cmdset->flag_withp = 1;
				}
				else if (!stricmp(str, "-withn")){
					cmdset->flag_withn = 1;
				}
				else if (!stricmp(str, "-noedge")){
					cmdset->flag_noedge = 1;
				}
				else if (!stricmp(str, "-overlap")){
					cmdset->flag_overlap = 1;
				}
				else if (!stricmp(str, "-confirm")){
					cmdset->flag_confirm = 1;
				}
				else if (!stricmp(str, "-unit")){
					cmdset->flag_unit = 1;
				}
				else if (!stricmp(str, "-else")){
					cmdset->flag_else = 1;
				}
				else if (!stricmp(str, "-cont")){
					cmdset->flag_cont = 1;
				}
				else if (!stricmp(str, "-reset")){
					cmdset->flag_reset = 1;
				}
				else if (!stricmp(str, "-flat")){
					cmdset->flag_flat = 1;
				}
				else if (!stricmp(str, "-force")){
					cmdset->flag_force = 1;
				}
				else if (!stricmp(str, "-autochg")){
					cmdset->flag_autochg = 1;
				}
				else if (!stricmp(str, "-autoeach")){
					cmdset->flag_autoeach = 1;
				}
				else if (str[0] == '#'){
					pos = -1;
				}
				else{
					retval = -1;
				}
			}
		}
		// オプションで開始と終了が逆の場合は反転
		for(i=0; i<cmdset->lens_num; i++){
			if (cmdset->lens_min[i] > cmdset->lens_max[i] &&
				cmdset->lens_min[i] != -1 && cmdset->lens_max[i] != -1){
				ntmp = cmdset->lens_min[i];
				cmdset->lens_min[i] = cmdset->lens_max[i];
				cmdset->lens_max[i] = ntmp;
			}
		}
		if (cmdset->endlen_left > cmdset->endlen_right){
			ntmp = cmdset->endlen_left;
			cmdset->endlen_left  = cmdset->endlen_right;
			cmdset->endlen_right = ntmp;
		}
		if (cmdset->sft_left > cmdset->sft_right){
			ntmp = cmdset->sft_left;
			cmdset->sft_left  = cmdset->sft_right;
			cmdset->sft_right = ntmp;
		}
	}

	//--- フレーム範囲検出用設定読み出し ---
	if (retval >= 0){
		if (get_regvar_frm(&ndata, regvar, "HEADFRAME") >= 0){
			cmdset->select_frm_min = ndata;
		}
		else{
			cmdset->select_frm_min = -1;
		}
		if (get_regvar_frm(&ndata, regvar, "TAILFRAME") >= 0){
			cmdset->select_frm_max = ndata;
		}
		else{
			cmdset->select_frm_max = -1;
		}
	}

	return retval;
}


//---------------------------------------------------------------------
// ロゴデータがあるものとして追加
// overlap=0の場合、設定範囲内に既にロゴ表示がある場合は追加しない
// 入力：
//   frm_st  : 挿入開始フレーム
//   frm_ed  : 挿入終了フレーム
//   overlap : 0:既存ロゴと重なる場合は追加しない 1:既存ロゴに関係なく追加
//   confirm : 0:候補として取得  1:確定データとして取得
// 返り値：
//   0 : 追加せず
//   1 : ロゴがあるものとして追加
//---------------------------------------------------------------------
int insert_logo(LOGO_RESULTREC *plogo, int frm_st, int frm_ed, int overlap, int confirm, int unit){
	int i, num_ins;
	int retval;
	int total_ins;
	int bak_flag_rise, bak_flag_fall;
	int bak_frm_result_rise, bak_frm_result_fall;
	int wid_ovl = 30;

	// 挿入箇所と同じ位置の確定情報保存用
	bak_flag_rise = -1;
	bak_flag_fall = -1;

	// ロゴ挿入箇所を検索
	num_ins = -1;
	for(i=1; i<plogo->num_find; i++){
		if (frm_st >= plogo->frm_fall[i-1] && frm_ed <= plogo->frm_rise[i]){
			num_ins = i;
		}
	}
	if (num_ins < 0){
		if (frm_ed <= plogo->frm_rise[0]){
			num_ins = 0;
		}
		else if (frm_st >= plogo->frm_fall[plogo->num_find-1]){
			num_ins = plogo->num_find;
		}
	}
	if (plogo->num_find >= LOGO_FIND_MAX){
		num_ins = -1;
	}

	total_ins = 1;
	// overlap許可時の追加検索
	if (overlap > 0 && num_ins < 0 && plogo->num_find < LOGO_FIND_MAX){
		for(i=0; i<plogo->num_find; i++){
			// 挿入箇所と同じ位置の確定情報保存
			if (abs(frm_st - plogo->frm_rise[i]) <= wid_ovl){
				bak_flag_rise       = plogo->flag_rise[i];
				bak_frm_result_rise = plogo->frm_result_rise[i];
			}
			if (abs(frm_ed - plogo->frm_fall[i]) <= wid_ovl){
				bak_flag_fall       = plogo->flag_fall[i];
				bak_frm_result_fall = plogo->frm_result_fall[i];
			}
			// 挿入ロゴの前半のみロゴと重なる場合
			if (frm_st > plogo->frm_rise[i] + wid_ovl && frm_st < plogo->frm_fall[i] &&
				frm_ed > plogo->frm_fall[i] - wid_ovl){
				plogo->frm_fall[i]   = frm_st-1;
				plogo->frm_fall_l[i] = frm_st-1;
				plogo->frm_fall_r[i] = frm_st-1;
				plogo->fade_fall[i]  = 0;
				plogo->intl_fall[i]  = 0;
				plogo->stat_fall[i]  = 2;
				plogo->flag_fall[i]  = 0;
				if (confirm > 0){
					plogo->flag_fall[i]  = 1;
					plogo->frm_result_fall[i] = plogo->frm_fall[i];
				}
			}
			// 挿入ロゴの後半のみロゴと重なる場合
			if (frm_st < plogo->frm_rise[i] + wid_ovl &&
				frm_ed > plogo->frm_rise[i] && frm_ed < plogo->frm_fall[i] - wid_ovl){
				plogo->frm_rise[i]   = frm_ed+1;
				plogo->frm_rise_l[i] = frm_ed+1;
				plogo->frm_rise_r[i] = frm_ed+1;
				plogo->fade_rise[i]  = 0;
				plogo->intl_rise[i]  = 0;
				plogo->stat_rise[i]  = 2;
				plogo->flag_rise[i]  = 0;
				if (confirm > 0){
					plogo->flag_rise[i]  = 1;
					plogo->frm_result_rise[i] = plogo->frm_rise[i];
				}
			}
			// 挿入ロゴ内にロゴ全体が入る場合
			if (frm_st - wid_ovl <= plogo->frm_rise[i] && frm_ed + wid_ovl >= plogo->frm_fall[i]){
					plogo->flag_rise[i] = 2;			// abort
					plogo->flag_fall[i] = 2;			// abort
					plogo->frm_result_rise[i] = -1;
					plogo->frm_result_fall[i] = -1;
					if (num_ins < 0){
						num_ins = i;
						total_ins = 0;		// 挿入ロゴは上書きする
					}
			}
			// 挿入ロゴ全体がロゴ範囲内に入る場合
			if (frm_st > plogo->frm_rise[i] + wid_ovl && frm_ed < plogo->frm_fall[i] - wid_ovl){
				num_ins = i+1;
				total_ins = 2;				// 挿入ロゴが２つになる
			}
			// 挿入位置
			if (num_ins < 0 && frm_st <= plogo->frm_rise[i]){
				num_ins = i;
			}
		}
		if (num_ins < 0){
			num_ins = plogo->num_find;
		}
	}
	if (plogo->num_find + total_ins > LOGO_FIND_MAX){
		num_ins = -1;
	}

	// 結果を格納
	retval = 0;
	if (num_ins >= 0){
		retval = 1;
		plogo->num_find += total_ins;
		for(i=plogo->num_find-1; i >= num_ins + total_ins; i--){
			plogo->frm_rise[i] = plogo->frm_rise[i - total_ins];
			plogo->frm_fall[i] = plogo->frm_fall[i - total_ins];
			plogo->frm_rise_l[i] = plogo->frm_rise_l[i - total_ins];
			plogo->frm_rise_r[i] = plogo->frm_rise_r[i - total_ins];
			plogo->frm_fall_l[i] = plogo->frm_fall_l[i - total_ins];
			plogo->frm_fall_r[i] = plogo->frm_fall_r[i - total_ins];
			plogo->fade_rise[i] = plogo->fade_rise[i - total_ins];
			plogo->fade_fall[i] = plogo->fade_fall[i - total_ins];
			plogo->intl_rise[i] = plogo->intl_rise[i - total_ins];
			plogo->intl_fall[i] = plogo->intl_fall[i - total_ins];
			plogo->stat_rise[i] = plogo->stat_rise[i - total_ins];
			plogo->stat_fall[i] = plogo->stat_fall[i - total_ins];

			plogo->flag_rise[i] = plogo->flag_rise[i - total_ins];
			plogo->flag_fall[i] = plogo->flag_fall[i - total_ins];
			plogo->frm_result_rise[i] = plogo->frm_result_rise[i - total_ins];
			plogo->frm_result_fall[i] = plogo->frm_result_fall[i - total_ins];
			plogo->flag_unit[i] = plogo->flag_unit[i - total_ins];
		}
		if (total_ins == 2){		// 既存ロゴ途中にロゴ挿入して元ロゴを２分割する場合
			i = num_ins + 1;
			plogo->frm_rise[i] = frm_ed+1;
			plogo->frm_fall[i] = plogo->frm_fall[i-2];
			plogo->frm_rise_l[i] = frm_ed+1;
			plogo->frm_rise_r[i] = frm_ed+1;
			plogo->frm_fall_l[i] = plogo->frm_fall_l[i-2];
			plogo->frm_fall_r[i] = plogo->frm_fall_r[i-2];
			plogo->fade_rise[i] = 0;
			plogo->fade_fall[i] = plogo->fade_fall[i-2];
			plogo->intl_rise[i] = 0;
			plogo->intl_fall[i] = plogo->intl_fall[i-2];
			plogo->stat_rise[i] = 1;
			plogo->stat_fall[i] = plogo->stat_fall[i-2];

			plogo->flag_rise[i] = 0;
			plogo->flag_fall[i] = plogo->flag_fall[i-2];
			plogo->frm_result_rise[i] = 0;
			plogo->frm_result_fall[i] = plogo->frm_result_fall[i-2];
			plogo->flag_unit[i] = plogo->flag_unit[i-2];

			plogo->frm_fall[i-2] = frm_st-1;
			plogo->frm_fall_l[i-2] = frm_st-1;
			plogo->frm_fall_r[i-2] = frm_st-1;
			plogo->fade_fall[i-2] = 0;
			plogo->intl_fall[i-2] = 0;
			plogo->stat_fall[i-2] = 2;

			plogo->flag_fall[i-2] = 0;
			plogo->frm_result_fall[i-2] = 0;

			if (confirm > 0){
				plogo->flag_rise[i] = 1;
				plogo->frm_result_rise[i] = plogo->frm_rise[i];
				plogo->flag_fall[i-2] = 1;
				plogo->frm_result_fall[i-2] = plogo->frm_fall[i-2];
			}
		}
		i = num_ins;
		plogo->frm_rise[i] = frm_st;
		plogo->frm_fall[i] = frm_ed;
		plogo->frm_rise_l[i] = frm_st;
		plogo->frm_rise_r[i] = frm_st;
		plogo->frm_fall_l[i] = frm_ed;
		plogo->frm_fall_r[i] = frm_ed;
		plogo->fade_rise[i] = 0;
		plogo->fade_fall[i] = 0;
		plogo->intl_rise[i] = 0;
		plogo->intl_fall[i] = 0;
		plogo->stat_rise[i] = 2;
		plogo->stat_fall[i] = 2;

		if (confirm > 0){
			plogo->flag_rise[i] = 1;
			plogo->flag_fall[i] = 1;
			plogo->frm_result_rise[i] = frm_st;
			plogo->frm_result_fall[i] = frm_ed;
		}
		else{
			plogo->flag_rise[i] = 0;
			plogo->flag_fall[i] = 0;
			plogo->frm_result_rise[i] = 0;
			plogo->frm_result_fall[i] = 0;
			if (bak_flag_rise > 0){
				plogo->flag_rise[i]       = bak_flag_rise;
				plogo->frm_result_rise[i] = bak_frm_result_rise;
			}
			if (bak_flag_fall > 0){
				plogo->flag_fall[i]       = bak_flag_fall;
				plogo->frm_result_fall[i] = bak_frm_result_fall;
			}
		}
		if (unit > 0){
			plogo->flag_unit[i] = 1;
		}
	}

	return retval;
}


//---------------------------------------------------------------------
// -SC, -NoSCオプションに対応するシーンチェンジ有無判定
// 入力：
//   frm_base  : 基準となるフレーム
//   num_sc_st : シーンチェンジ検索開始番号
//   num_sc_ed : シーンチェンジ検索終了番号
//   edge      : 0:start edge  1:end edge
// 返り値：
//   0 : 一致せず
//   1 : 一致確認
//---------------------------------------------------------------------
int cmd_execute_chk_sc(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset,  long *frm_scpos_rf,
						int frm_base, int num_sc_st, int num_sc_ed){
	int j, k;
	int frm_now;
	int frm_smute_s, frm_smute_e;
	int lens_min, lens_max;
	int num_scpos_sc;
	int num_smute_all;
	int num_smute_part;
	int result;

	result = 1;
	for(k=0; k<cmdset->lens_num; k++){
		num_scpos_sc   = -1;
		num_smute_all  = -1;
		num_smute_part = -1;
		lens_min = cmdset->lens_min[k];
		lens_max = cmdset->lens_max[k];
		for(j=num_sc_st; j<num_sc_ed; j++){
			frm_now = frm_scpos_rf[j];
			if ((frm_now - frm_base >= lens_min || lens_min == -1) &&
				(frm_now - frm_base <= lens_max || lens_min == -1)){
				if (plogo->stat_scpos[j] >= 0){
					num_scpos_sc = j;
				}
			}
			// 無音系
			frm_smute_s = plogo->frm_smute_s[j];
			frm_smute_e = plogo->frm_smute_e[j];
			if (frm_smute_s < 0 || frm_smute_e < 0){
				frm_smute_s = frm_now;
				frm_smute_e = frm_now;
			}
			// for -SMA option （無音情報がある場合のみ検出）
			if ((frm_smute_s - frm_base <= lens_min) &&
				(frm_smute_e - frm_base >= lens_max)){
				num_smute_all = j;
			}
			//for -SM option
			if ((frm_smute_s - frm_base <= lens_max || lens_max == -1) &&
				(frm_smute_e - frm_base >= lens_min || lens_min == -1)){
				num_smute_part = j;
			}
//printf("(%d %d %d %d %d %d)", k, frm_base, frm_smute_s, frm_smute_e, num_smute_all, num_smute_part);
		}
		if (num_scpos_sc < 0 && cmdset->lens_sctype[k] == 0){	// -SC
			result = 0;
		}
		else if (num_scpos_sc >= 0 && cmdset->lens_sctype[k] == 1){	// -NoSC
			result = 0;
		}
		else if (num_smute_part < 0 && cmdset->lens_sctype[k] == 2){	// -SM
			result = 0;
		}
		else if (num_smute_part >= 0 && cmdset->lens_sctype[k] == 3){	// -NoSM
			result = 0;
		}
		else if (num_smute_all < 0 && cmdset->lens_sctype[k] == 4){	// -SMA
			result = 0;
		}
	}
	return result;
}

//---------------------------------------------------------------------
// -EndLenオプションに対応するシーンチェンジ位置取得
// 入力：
//   frm_base  : 基準となるフレーム
//   num_sc_st : シーンチェンジ検索開始番号
//   num_sc_ed : シーンチェンジ検索終了番号
//   edge      : 0:start edge  1:end edge
// 返り値：
//   -1    : 該当なし
//   0以上 : 一致するシーンチェンジ番号
//---------------------------------------------------------------------
int cmd_execute_chk_endlen(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, long *frm_scpos_rf,
						int frm_base, int num_sc_st, int num_sc_ed){
	int j;
	int frm_now;
	int frm_endlen_c, frm_endlen_l, frm_endlen_r;
	int val_dif, val_difmin;
	int stat_now, stat_scpos;
	int num_scpos_end;

	frm_endlen_c = frm_base + cmdset->endlen_center;
	frm_endlen_l = frm_base + cmdset->endlen_left;
	frm_endlen_r = frm_base + cmdset->endlen_right;

	num_scpos_end = -1;
	val_difmin = 0;
	stat_scpos = 0;
	for(j=num_sc_st; j<num_sc_ed; j++){
		frm_now = frm_scpos_rf[j];
		stat_now = plogo->stat_scpos[j];
		val_dif = abs(frm_now - frm_endlen_c);
		if (val_difmin > val_dif || num_scpos_end < 0){
			if (frm_now >= frm_endlen_l && frm_now <= frm_endlen_r){
				if (stat_now >= stat_scpos){
					val_difmin = val_dif;
					num_scpos_end = j;
					stat_scpos = stat_now;
				}
			}
		}
	}
	return num_scpos_end;
}


//---------------------------------------------------------------------
// フレーム位置限定
//---------------------------------------------------------------------
void cmd_execute_limitfrm(int *r_frm_limit_left, int *r_frm_limit_right, 
				LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int j;
	int num_1st_rise;
	int num_tmp1, num_tmp2;
	int frm_limit_left, frm_limit_right, frm_now;
	int frm_min, frm_max, frm_tmp1, frm_tmp2;

	// -FRオプションのフレームを検索し、フレーム制限値を取得
	num_1st_rise = 0;
	while( plogo->flag_rise[num_1st_rise] == 2 && num_1st_rise < plogo->num_find - 1){
		num_1st_rise ++;
	}
	if (plogo->flag_rise[num_1st_rise] == 2 && num_1st_rise == plogo->num_find - 1){
		num_1st_rise = 0;		// 全部無効ロゴだった場合の設定
	}
	// フレーム制限値を設定
	frm_limit_left  = cmdset->select_frm_left;
	frm_limit_right = cmdset->select_frm_right;
	if (cmdset->flag_fr > 0){
		if (frm_limit_left != -1){
			frm_limit_left += plogo->frm_rise[num_1st_rise];
		}
		if (frm_limit_right != -1){
			frm_limit_right += plogo->frm_rise[num_1st_rise];
		}
	}
//	printf("(frame %d,%d)\n", frm_limit_left, frm_limit_right);

	// -Fhead,-Ftail,-Fmidでフレーム指定時のフレーム計算
	if (cmdset->flag_fhead > 0 || cmdset->flag_ftail > 0 || cmdset->flag_fmid > 0){
		// 最初のロゴ開始から最後のロゴ終了の中間地点を取得
		num_tmp1 = -1;
		num_tmp2 = -1;
		for(j=0; j<plogo->num_find; j++){
			if (num_tmp1 < 0){
				if (plogo->flag_rise[j] != 2){
					num_tmp1 = j;
				}
			}
			if (plogo->flag_fall[j] != 2){
				num_tmp2 = j;
			}
		}
		if (num_tmp1 < 0) num_tmp1 = 0;
		if (num_tmp2 < 0) num_tmp2 = plogo->num_find - 1;
		if (plogo->num_find > 0){
			frm_tmp1 = plogo->frm_rise[num_tmp1];
			frm_tmp2 = plogo->frm_fall[num_tmp2];
		}
		else{
			frm_tmp1 = 0;
			frm_tmp2 = plogo->frm_totalmax;
		}
		// フレーム範囲の取得
		frm_min = 0;
		frm_max = plogo->frm_totalmax;
		if (cmdset->select_frm_min != -1){
			frm_min  = cmdset->select_frm_min;
			frm_tmp1 = frm_min;
		}
		if (cmdset->select_frm_max != -1){
			frm_max  = cmdset->select_frm_max;
			frm_tmp2 = frm_max;
		}
		frm_now = (frm_tmp1 + frm_tmp2) / 2;
		// フレーム制限範囲を設定
		if (cmdset->flag_fhead > 0){
			frm_limit_left  = frm_min + cmdset->select_frm_left;
			frm_limit_right = frm_min + cmdset->select_frm_right;
			if (frm_limit_right > frm_now){
				frm_limit_right = frm_now;
			}
		}
		else if (cmdset->flag_ftail > 0){
			frm_limit_left  = frm_max - cmdset->select_frm_right;
			frm_limit_right = frm_max - cmdset->select_frm_left;
			if (frm_limit_left < frm_now){
				frm_limit_left = frm_now;
			}
		}
		else if (cmdset->flag_fmid > 0){
			frm_limit_left  = frm_min + cmdset->select_frm_left;
			frm_limit_right = frm_max - cmdset->select_frm_right;
			if (frm_limit_left > frm_now){
				frm_limit_left = frm_now;
			}
			if (frm_limit_right < frm_now){
				frm_limit_right = frm_now;
			}
		}
	}
	*r_frm_limit_left  = frm_limit_left;
	*r_frm_limit_right = frm_limit_right;
}


//---------------------------------------------------------------------
// コマンドを実行
// 返り値：
//   0 : コマンド実行なし
//   1 : コマンド実行扱い（実行済み、または-elseオプションによるスキップで実行扱い）
//---------------------------------------------------------------------
int cmd_execute(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int i, j, num;
	int num_rel_rf, num_rel_rise, num_rel_fall;
	int n, edge;
	int exeflag;
	int floatbase, candflag, within_logo, en_reset, igncomp;
	int exefix_only;
	int sea1st_only;
	int rangeflag;
	long *frm_rf, *frm_rf_l, *frm_rf_r;
	long *frm_scpos_rf, *frm_result_rf;
	char *stat_rf;
	char *flag_rf;
	int frm_last, frm_last_l, frm_last_r, frm_next;
	int frm_base_c, frm_base_l, frm_base_r;
	int frm_find_c, frm_find_l, frm_find_r;
	int frm_logo_l, frm_logo_r;
	int frm_now, frm_max;
	int frm_st, frm_ed;
	int frm_diftmp;
	int frm_limit_left, frm_limit_right;
	int j2, num_dst, flag_shiftup;
	int val_sft_tmp, val_sftdif_tmp, val_sftdif_min;
	int val_dif, val_difmin, val_tmp;
	int flag_sft1st;
	int flag_autocmd;
	int valid_force;
	int stat_tmp;
	int stat_scpos, stat_now, stat_flat;
	int num_sc_st, num_sc_ed;
	int num_scpos_sel;
	int num_scpos_end, num_scpos_end_now;
	int exe_command, exe_command_total;
	int num_find_last, num_logoins_last;


	// ロゴ挿入時の一時状態保管用
	num_logoins_last = -1;

	// -NRオプションの相対ロゴ番号に対応する実体番号を取得（num_rel_rise/fallに格納）
	// 見つからなかったら存在しない値（plogo->num_find）に設定
	j = cmdset->select_nr;
	if (j > 0){							// -NRオプションの値が正の時
		//--- 立ち上がり番号検出 ---
		num_rel_rise = 0;
		while( j > 0 && num_rel_rise < plogo->num_find){
			if (plogo->flag_rise[num_rel_rise] != 2){		// abort以外
				j --;
			}
			num_rel_rise ++;
		}
		//--- 立ち下がり番号検出 ---
		j = cmdset->select_nr;
		num_rel_fall = 0;
		while( j > 0 && num_rel_fall < plogo->num_find){
			if (plogo->flag_fall[num_rel_fall] != 2){		// abort以外
				j --;
			}
			num_rel_fall ++;
		}
	}
	else if (j < 0){					//  -NRオプションの値が負の時
		//--- 立ち上がり番号検出 ---
		num_rel_rise = plogo->num_find;
		while( j < 0 && num_rel_rise > 0){
			num_rel_rise --;
			if (plogo->flag_rise[num_rel_rise] != 2){		// abort以外
				j ++;
			}
		}
		num_rel_rise ++;
		if (j < 0){						// when not found
			num_rel_rise = plogo->num_find;
		}
		//--- 立ち下がり番号検出 ---
		j = cmdset->select_nr;
		num_rel_fall = plogo->num_find;
		while( j < 0 && num_rel_fall > 0){
			num_rel_fall --;
			if (plogo->flag_fall[num_rel_fall] != 2){		// abort以外
				j ++;
			}
		}
		num_rel_fall ++;
		if (j < 0){						// when not found
			num_rel_fall = plogo->num_find;
		}
	}
	else{								// -NRオプションの値が０の時
		num_rel_rise = 0;
		num_rel_fall = 0;
	}
//if (cmdset->select_nr < 0){
//	printf("(%d %d)", num_rel_fall, plogo->flag_rise[2]);
//}

	// -F系オプションのフレームを検索し、フレーム制限値を取得
	cmd_execute_limitfrm(&frm_limit_left, &frm_limit_right, plogo, cmdset);

	// 無音シーンチェンジ検出用の範囲
	if (cmdset->flag_noedge == 0){
		num_sc_st = 0;
		num_sc_ed = plogo->num_scpos;
	}
	else{								// -noedge時は0と最終フレームは除く
		num_sc_st = 1;
		num_sc_ed = plogo->num_scpos - 1;
	}

	// ロゴ基準位置がロゴで固定かチェック
	floatbase = 0;
	// 確定状態を無視するコマンドかチェック
	igncomp = 0;
	// ロゴ不確定候補内で選択するコマンドかチェック
	within_logo = 0;
	en_reset = 0;
	sea1st_only = 0;		// 最初のロゴ開始前の区切り位置設定用(0:なし 1:検出を行う 2:検出済み)
	if (!stricmp(cmdset->cmdname, "MkLogo")){
		floatbase = 1;
		igncomp = 1;
	}
	if (!stricmp(cmdset->cmdname, "DivLogo")){
		floatbase = 1;
		igncomp = 1;
	}
	if (cmdset->flag_shift > 0){
		floatbase = 1;
	}
	if (!stricmp(cmdset->cmdname, "Select")){
		floatbase = 1;
		within_logo = 1;
		en_reset = 1;
		sea1st_only = 1;
	}

	exe_command_total = 0;
	flag_autocmd = 0;		// AutoCut/AutoAdd/AutoCM用
	// 各ロゴの開始／終了エッジそれぞれについて確認していく
	for(i=0; i<plogo->num_find*2; i++){
		n    = i/2;							// logo number
		edge = i % 2;						// 0:start edge  1:end edge

		// set pointer of start/end data
		if (edge == 0){						// start edge
			frm_rf   = plogo->frm_rise;
			frm_rf_l = plogo->frm_rise_l;
			frm_rf_r = plogo->frm_rise_r;
			stat_rf  = plogo->stat_rise;
			flag_rf  = plogo->flag_rise;
			frm_scpos_rf  = plogo->frm_scpos_s;
			frm_result_rf = plogo->frm_result_rise;
			num_rel_rf  = num_rel_rise;
		}
		else{								// end edge
			frm_rf   = plogo->frm_fall;
			frm_rf_l = plogo->frm_fall_l;
			frm_rf_r = plogo->frm_fall_r;
			stat_rf  = plogo->stat_fall;
			flag_rf  = plogo->flag_fall;
			frm_scpos_rf  = plogo->frm_scpos_e;
			frm_result_rf = plogo->frm_result_fall;
			num_rel_rf  = num_rel_fall;
		}

		// get previous/next point
		// 既にabortされた位置は飛ばして取得
		if (edge == 0){						// start edge
			num = n-1;
			frm_last   = -1;
			frm_last_l = -1;
			frm_last_r = -1;
			while (num >= 0){
				if (plogo->flag_fall[num] != 2){		// abort以外
					frm_last   = plogo->frm_fall[num];
					frm_last_l = plogo->frm_fall_l[num];
					frm_last_r = plogo->frm_fall_r[num];
					break;
				}
				num --;
			}
			num = n;
			frm_next   = -1;
			while (num < plogo->num_find){
				if (plogo->flag_fall[num] != 2){		// abort以外
					frm_next   = plogo->frm_fall[num];
					break;
				}
				num ++;
			}
		}
		else{								// end edge
			num = n;
			frm_last   = -1;
			frm_last_l = -1;
			frm_last_r = -1;
			while(num >= 0){
				if (plogo->flag_rise[num] != 2){		// abort以外
					frm_last   = plogo->frm_rise[num];
					frm_last_l = plogo->frm_rise_l[num];
					frm_last_r = plogo->frm_rise_r[num];
					break;
				}
				num --;
			}
			num = n+1;
			frm_next   = -1;
			while (num < plogo->num_find){
				if (plogo->flag_rise[num] != 2){		// abort以外
					frm_next   = plogo->frm_rise[num];
					break;
				}
				num ++;
			}
		}

		// for Select command
		exefix_only = 0;
		// check constarint to execute
		if (flag_rf[n] > 0 &&  								// when already finished
			cmdset->select_auto == D_CMDAUTO_NONE &&		// Auto系コマンドでは確定状態を見ない
			igncomp == 0){									// 確定状態を見る時
				exeflag = 0;
		}
		else if (cmdset->select_edge != edge && cmdset->select_edge != 2){	// check start/end edge
			exeflag = 0;
		}
		else if (num_rel_rf != 0 && num_rel_rf != n+1){	// check relative logo number
			exeflag = 0;
		}
		else if (cmdset->select_num != 0 && cmdset->select_num != n+1 &&	// check logo number
				 plogo->num_find + cmdset->select_num != n ){
			exeflag = 0;
		}
		else if (((frm_limit_left > frm_rf_r[n]) ||		// check frame number
				  (frm_limit_right < frm_rf_l[n] && frm_limit_right >= 0)) &&
				 (cmdset->select_auto == D_CMDAUTO_NONE || cmdset->select_auto >= D_CMDAUTO_EDGE)){
														// AutoCut/AutoAddコマンドはフレーム範囲をここでは見ない
				exeflag = 0;
		}
		else if (cmdset->flag_fromabs > 0 && cmdset->frm_fromabs < 0){	// check -fromabs option
			exeflag = 0;
		}
		else if (en_reset > 0 && cmdset->flag_reset == 0 &&		// check -reset option for Select
				 stat_rf[n] >= 2){
			// Selectでロゴ検索拡張が３秒以上の時だけ確定候補が２つ以上ある場合があるため検索続行する
			if ((cmdset->logoext_right - cmdset->logoext_left) <= CnvTimeFrm(2,500)){
				exeflag = 1;
				exefix_only = 1;		// sea1st_only検索のため続行
			}
			else{
				exeflag = 1;
				exefix_only = 2;		// 確定候補が２つ以上ある場合があるため検索続行
			}
			// 0位置検索する場合はAutoCMコマンドで先頭カットしない処理
			if (n == 0 && edge == 0 && cmdset->frm_fromabs == 0){
				if (plogo->frm_scpos_1stsel < 0){
					plogo->frm_scpos_1stsel = 0;
				}
			}
		}
		else{
			exeflag = 1;
		}


		// limit by length constraint(-LENP -LENN option)
		if (exeflag == 1){
			if ((frm_rf[n] - frm_last < cmdset->lenp_min &&
				 cmdset->lenp_min >= 0 &&
				 frm_last >= 0) ||
				(frm_rf[n] - frm_last > cmdset->lenp_max &&
				 cmdset->lenp_max >= 0 &&
				 frm_last >= 0) ||
				(frm_next - frm_rf[n] < cmdset->lenn_min &&
				 cmdset->lenn_min >= 0 &&
				 frm_next >= 0) ||
				(frm_next - frm_rf[n] > cmdset->lenn_max &&
				 cmdset->lenn_max >= 0 &&
				 frm_next >= 0)){
				exeflag = 0;
			}
		}

		// limit by scene change (-SC -NoSC option)
		if (exeflag == 1){
			if (floatbase == 0){
				if (cmdset->flag_fromlast > 0){		// from last edge
					frm_find_c = frm_last;
				}
				else{
					frm_find_c = frm_rf[n];
				}
				exeflag = cmd_execute_chk_sc( plogo, cmdset, frm_scpos_rf, frm_find_c, num_sc_st, num_sc_ed );
			}
		}

		// 前コマンド実行済みか確認 (-else option)
		exe_command = 0;
		if (cmdset->flag_else > 0){
			if (cmdset->exe_last > 0){
				exe_command = 1;
				exeflag = 0;
			}
		}
		// 前コマンド実行済みか確認 (-cont option)
		if (cmdset->flag_cont > 0){
			if (cmdset->exe_last == 0){
				exeflag = 0;
			}
		}
		// set address to execute
		if (exeflag == 1){
			// ロゴデータのフレーム範囲を読み込み
			if (cmdset->flag_fromlast > 0){		// from last edge
				frm_base_c = frm_last;
				if (cmdset->flag_wide > 0){
					frm_base_l = frm_last_l;
					frm_base_r = frm_last_r;
				}
				else{
					frm_base_l = frm_last;
					frm_base_r = frm_last;
				}
			}
			else if (cmdset->flag_fromabs > 0){	// from absolute frame number
				frm_base_c = cmdset->frm_fromabs;
				frm_base_l = cmdset->frm_fromabs;
				frm_base_r = cmdset->frm_fromabs;
			}
			else{								// from current edge
				frm_base_c = frm_rf[n];
				if (cmdset->flag_wide > 0){
					frm_base_l = frm_rf_l[n];
					frm_base_r = frm_rf_r[n];
				}
				else{
					frm_base_l = frm_rf[n];
					frm_base_r = frm_rf[n];
				}
			}
			// コマンド指定の範囲をフレーム範囲に追加
			frm_find_c = frm_base_c + cmdset->frm_center;	// set point to find
			frm_find_l = frm_base_l + cmdset->frm_left;
			frm_find_r = frm_base_r + cmdset->frm_right;
			// Selectコマンド用の範囲
			if (within_logo > 0){
				frm_logo_l = frm_rf_l[n] + cmdset->logoext_left;
				frm_logo_r = frm_rf_r[n] + cmdset->logoext_right;
				if (frm_find_l > frm_logo_r || frm_find_r < frm_logo_l){
					exeflag = 0;
				}
				else{
					if (frm_find_l < frm_logo_l){
						frm_find_l = frm_logo_l;
					}
					if (frm_find_r > frm_logo_r){
						frm_find_r = frm_logo_r;
					}
				}
				if (frm_find_c < frm_find_l){
					frm_find_c = frm_find_l;
				}
				if (frm_find_c > frm_find_r){
					frm_find_c = frm_find_r;
				}
				// Select時の先頭位置search確認
				if (sea1st_only >= 1){
					// 最初のロゴ開始前の区切り位置設定用に使用するか確認
					if (n == 0 && edge == 0){
						sea1st_only = 2;
					}
					else{
						sea1st_only = 1;
					}
				}
			}
		}

		// execute
		if (exeflag == 1){
			// find nearest point from logo-change to scene-change pos
			val_difmin = 0;
			num_scpos_sel = -1;
			stat_scpos = 0;
			stat_flat = 0;
			if (within_logo > 0 || cmdset->flag_flat > 0){
				stat_flat = 1;
			}
			val_sftdif_min = -1;
			for(j=num_sc_st; j<num_sc_ed; j++){
				frm_now = frm_scpos_rf[j];
				stat_now = plogo->stat_scpos[j];
				if (stat_flat > 0){
					stat_now = 0;
				}
				val_dif = abs(frm_now - frm_find_c);
				num_dst = j;
				candflag = 1;
				// 候補位置を基準にして-SCオプションのフレーム位置判断する場合
				if (floatbase > 0){
					candflag = cmd_execute_chk_sc( plogo, cmdset, frm_scpos_rf, frm_now, num_sc_st, num_sc_ed );
				}
				// -EndLen処理
				num_scpos_end_now = -1;
				if (candflag > 0 && cmdset->endlen_center != 0){
					num_scpos_end_now = cmd_execute_chk_endlen(
											plogo, cmdset, frm_scpos_rf, frm_now, num_sc_st, num_sc_ed );
					if (num_scpos_end_now < 0){
						candflag = 0;
					}
				}
				// -Shift処理
				flag_shiftup = 0;
				if (candflag > 0 && cmdset->flag_shift != 0){
					val_sft_tmp = frm_now - frm_base_c;
					val_sftdif_tmp = abs(val_sft_tmp - cmdset->sft_center);
//printf("%d %d %d %d %d\n", frm_now, val_sft_tmp, frm_find_c, (int)cmdset->sft_right, stat_now);
					if (val_sft_tmp >= cmdset->sft_left &&
						val_sft_tmp <= cmdset->sft_right &&
						(val_sftdif_min > val_sftdif_tmp || val_sftdif_min < 0) &&
						stat_now >= 0){
						flag_sft1st = 1;
						for(j2=num_sc_st; j2<num_sc_ed; j2++){
							frm_diftmp = frm_scpos_rf[j2] - frm_now;
							if (frm_diftmp >= cmdset->frm_left && frm_diftmp <= cmdset->frm_right){
								val_tmp = abs(frm_diftmp - cmdset->frm_center);
								stat_tmp = plogo->stat_scpos[j2];
								if (stat_flat > 0){
									stat_tmp = 0;
								}
								if ((val_tmp < val_dif || flag_sft1st > 0) && stat_tmp >= 0){
//printf("%d+%d+%d+%d+%d ", frm_now,frm_diftmp, val_dif,val_difmin,num_scpos_sel);
									flag_shiftup = 1;
									flag_sft1st = 0;
									val_dif = val_tmp;
									stat_now = stat_tmp;
									num_dst = j2;
								}
							}
						}
					}
					if (flag_shiftup > 0){
						val_sftdif_min = val_sftdif_tmp;
					}
				}
				// 位置決め
				if (candflag > 0 &&
					(val_difmin > val_dif || num_scpos_sel < 0 || flag_shiftup > 0 || sea1st_only >= 2)){
					if ((frm_now >= frm_find_l && frm_now <= frm_find_r) || flag_shiftup > 0){
						rangeflag = 1;			// 通常検索の範囲内
					}
					else if (sea1st_only == 2 && frm_now < frm_find_l){
						if (val_difmin > val_dif || num_scpos_sel < 0){
							rangeflag = 2;			// 最初のロゴ開始前の区切り位置設定用
						}
						else{
							rangeflag = 0;
						}
					}
					else{
						rangeflag = 0;
					}
					if (rangeflag > 0){
						if (stat_now >= stat_scpos || stat_flat > 0 || (sea1st_only >= 2 && rangeflag == 1)){
							val_difmin = val_dif;
							num_scpos_sel = num_dst;
							num_scpos_end = num_scpos_end_now;
							stat_scpos = stat_now;
							if (sea1st_only >= 2 && rangeflag == 1){
								sea1st_only = 1;
							}
						}
					}
				}
			}

			// execute command
			if (!stricmp(cmdset->cmdname, "Find")){
				if (num_scpos_sel >= 0){
					frm_result_rf[n] = frm_scpos_rf[num_scpos_sel];
					flag_rf[n] = 1;
					exe_command = 1;
					if (cmdset->flag_autochg > 0){		// 推測構成に反映
						autochg_start(plogo, n, edge, frm_scpos_rf[num_scpos_sel]);
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "AutoCM")){
				if (flag_autocmd <= 1){
					flag_autocmd = autocm_start(plogo, cmdset);
					if ((flag_autocmd & 0x1) == 1){
						exe_command = 1;
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "AutoUp")){
				if (flag_autocmd <= 1){
					flag_autocmd = autoup_start(plogo, cmdset);
					if ((flag_autocmd & 0x1) == 1){
						exe_command = 1;
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "AutoCut")){
				if (flag_autocmd <= 1){
					flag_autocmd = autocut_start(plogo, cmdset, frm_limit_left, frm_limit_right);
					if ((flag_autocmd & 0x1) == 1){
						exe_command = 1;
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "AutoAdd")){
				if (flag_autocmd <= 1){
					flag_autocmd = autoadd_start(plogo, cmdset, frm_limit_left, frm_limit_right);
					if ((flag_autocmd & 0x1) == 1){
						exe_command = 1;
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "AutoEdge")){
				if (num_scpos_sel >= 0 || cmdset->lens_num == 0){
					exe_command = autoedge_start(plogo, cmdset, n);
				}
			}
			else if (!stricmp(cmdset->cmdname, "Force")){
				if (frm_find_c >= 0){
					frm_result_rf[n] = frm_find_c;
					flag_rf[n] = 1;
					exe_command = 1;
					// 最終シーンチェンジより後の場合変更
					if (plogo->num_scpos > 0){
						frm_max = plogo->frm_scpos_e[plogo->num_scpos-1];
						if (frm_find_c > frm_max){
							frm_result_rf[n] = frm_max;
						}
					}
					if (cmdset->flag_autochg > 0){		// 推測構成に反映
						autochg_start(plogo, n, edge, frm_result_rf[n]);
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "Abort")){
				frm_result_rf[n] = -1;
				flag_rf[n] = 2;
				exe_command = 1;
				if (cmdset->flag_withp > 0){
					if (edge == 0 && n > 0){
						plogo->flag_fall[n-1] = 2;
					}
					else if (edge == 1){
						plogo->flag_rise[n] = 2;
					}
				}
				if (cmdset->flag_withn > 0){
					if (edge == 0){
						plogo->flag_fall[n] = 2;
					}
					else if (edge == 1 && n < plogo->num_find - 1){
						plogo->flag_rise[n+1] = 2;
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "MkLogo")){
				if (num_scpos_sel >= 0 && num_scpos_end >= 0){
					if (frm_scpos_rf[num_scpos_sel] < frm_scpos_rf[num_scpos_end]){
						frm_st = plogo->frm_scpos_s[num_scpos_sel];
						frm_ed = plogo->frm_scpos_e[num_scpos_end];
					}
					else{
						frm_st = plogo->frm_scpos_s[num_scpos_end];
						frm_ed = plogo->frm_scpos_e[num_scpos_sel];
					}
					if (num_logoins_last < n){
						num_find_last = plogo->num_find;
						exe_command = insert_logo(plogo, frm_st, frm_ed,
									cmdset->flag_overlap, cmdset->flag_confirm, cmdset->flag_unit);
						// 挿入ロゴに更に挿入されることを防ぐ処理
						if (plogo->num_find > num_find_last){
							num_logoins_last = n + (plogo->num_find - num_find_last);
						}
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "DivLogo")){
				valid_force = 0;
				if (cmdset->flag_force > 0){
					valid_force =
						cmd_execute_chk_sc( plogo, cmdset, frm_scpos_rf, frm_find_c, num_sc_st, num_sc_ed );
				}
				if (num_scpos_sel >= 0 || valid_force > 0){
					if (edge == 0){			// start edge
						frm_st = frm_rf[n];
						if (num_scpos_sel >= 0){
							frm_ed = plogo->frm_scpos_s[num_scpos_sel] - 1;
						}
						else{
							frm_ed = frm_find_c - 1;
						}
						if (frm_ed >= frm_next-1 || frm_ed <= frm_st){
							frm_st = -1;
						}
					}
					else{					// end edge
						if (num_scpos_sel >= 0){
							frm_st = plogo->frm_scpos_s[num_scpos_sel];
						}
						else{
							frm_st = frm_find_c;
						}
						frm_ed = frm_rf[n];
						if (frm_st <= frm_last || frm_st >= frm_ed){
							frm_st = -1;
						}
					}
					if (num_logoins_last < n && frm_st >= 0){
						num_find_last = plogo->num_find;
						exe_command = insert_logo(plogo, frm_st, frm_ed,
									1, cmdset->flag_confirm, 1);
						// 挿入ロゴに更に挿入されることを防ぐ処理
						if (plogo->num_find > num_find_last){
							num_logoins_last = n + (plogo->num_find - num_find_last);
						}
					}
				}
			}
			else if (!stricmp(cmdset->cmdname, "Select")){
				// 最初のロゴ開始前の区切り位置確認時
				if (num_scpos_sel >= 0 && sea1st_only == 2){
					plogo->nchk_scpos_1st = num_scpos_sel;
					num_scpos_sel = -1;
				}
				// disable when check only fix point
				if (num_scpos_sel >= 0 && exefix_only >= 2){
					if (plogo->stat_scpos[num_scpos_sel] < 2 ||
						frm_rf[n] == frm_scpos_rf[num_scpos_sel]){
						num_scpos_sel = -1;
					}
				}
				if (num_scpos_sel >= 0 && exefix_only == 1){
					num_scpos_sel = -1;
				}
				if (num_scpos_sel >= 0){
					// 従来の確定位置を解除
					for(j=1; j<plogo->num_scpos - 1; j++){
						if (frm_scpos_rf[j] == frm_rf[n]){
							if (plogo->stat_scpos[j] > 0){
								plogo->stat_scpos[j] = 0;
							}
							if (plogo->nchk_scpos_1st == j){
								plogo->nchk_scpos_1st = 0;
							}
						}
					}
					// 新しい確定位置を設定
					frm_rf[n] = frm_scpos_rf[num_scpos_sel];
					plogo->stat_scpos[num_scpos_sel] = 2;
					stat_rf[n] = 2;
					if (frm_rf_l[n] > frm_rf[n]){
						frm_rf_l[n] = frm_rf[n];
					}
					if (frm_rf_r[n] < frm_rf[n]){
						frm_rf_r[n] = frm_rf[n];
					}
					exe_command = 1;
					// 最初の位置変更の場合
					if (frm_rf[n] < CnvTimeFrm(30,400) && edge == 0){
						if (plogo->nchk_scpos_1st > 0){
							plogo->nchk_scpos_1st = 0;
						}
						plogo->frm_scpos_1stsel = frm_rf[n];
					}
					// Auto系コマンドは再調整するため状態クリア
//					plogo->flag_autosetup = 0;
					// 確定する場合の処理
					if (cmdset->flag_confirm > 0){
						frm_result_rf[n] = frm_rf[n];
						flag_rf[n] = 1;
					}
				}
			}
			else{
				fprintf(stderr, "error: unknown command(%s)\n", cmdset->cmdname);
			}
			
		}
		if (exe_command > 0){
			exe_command_total = exe_command;
		}
	}
	// 前コマンド実行状態に戻すか確認 (-cont option)
	if (cmdset->flag_cont > 0){
		exe_command_total = cmdset->exe_last;
	}

	return exe_command_total;
}


//---------------------------------------------------------------------
// 最終結果にロゴ扱いを追加
//---------------------------------------------------------------------
int cmd_result_setauto_add(LAST_RESULTREC *presult, int frm_in_s, int frm_in_e){
	int j, k;
	int flag_end;
	int frm_last_ed, frm_next_st;
	int frm_cur_st, frm_cur_ed;
	int frm_tmp;
	int ncur;

	//--- ロゴ存在なく新規追加 ---
	if (presult->num <= 1){
		presult->num += 2;
		presult->frm[0] = frm_in_s;
		presult->frm[1] = frm_in_e;
	}
	//--- 通常処理 ---
	else{
		flag_end = 0;
		ncur = 0;
		frm_last_ed = 0;
		while(flag_end == 0 && ncur < presult->num - 1){
			frm_cur_st = presult->frm[ncur];
			frm_cur_ed = presult->frm[ncur+1];
			//--- 次回開始地点の処理 ---
			if (ncur+2 < presult->num){
				frm_next_st = presult->frm[ncur+2];
				// 次回開始と追加が重なっている場合
				if (frm_next_st <= frm_in_e + 2 && frm_next_st > frm_in_s + 2){
					// 現在終了と追加が重ならない場合
					if (frm_cur_ed < frm_in_s - 2){
						presult->frm[ncur+2] = frm_in_s;
						frm_next_st = presult->frm[ncur+2];
					}
					// 現在終了と追加が重なる場合、合併処理
					else{
						k = 2;
						frm_tmp = frm_next_st;
						while(frm_tmp <= frm_in_e + 2 && (ncur + k + 2) < presult->num){
							k += 2;
							frm_tmp = presult->frm[ncur + k];
						}
						if (frm_tmp > frm_in_e + 2){
							k -= 2;
						}
						// ロゴ期間合併がある場合
						if (k > 0){
							presult->frm[ncur+1] = presult->frm[ncur+k+1];
							for(j=ncur+2; j < presult->num - k - 1; j=j+2){
								presult->frm[j]   = presult->frm[j+k];
								presult->frm[j+1] = presult->frm[j+k+1];
							}
							presult->num -= k;
						}
						// 改めて現在位置設定
						frm_cur_st = presult->frm[ncur];
						frm_cur_ed = presult->frm[ncur+1];
						if (ncur+2 < presult->num){
							frm_next_st = presult->frm[ncur+2];
						}
						else{
							frm_next_st = 0;
						}
					}
				}
			}
			else{
				frm_next_st = 0;
			}
			//--- ロゴ開始部分に追加 ---
			if ((frm_last_ed < frm_in_s - 2 || ncur == 0) && frm_cur_st > frm_in_s + 2){
				// 現在開始と追加が重なる場合
				if (frm_cur_st <= frm_in_e + 2){
					presult->frm[ncur] = frm_in_s;
				}
				// 現在開始と追加が重ならない場合、ロゴセット追加
				else{
					if (presult->num < LOGO_FIND_MAX-1){
						for(j=presult->num - 1; j >= ncur; j--){
							presult->frm[j+2] = presult->frm[j];
						}
						presult->num += 2;
						presult->frm[ncur] = frm_in_s;
						presult->frm[ncur+1] = frm_in_e;
						ncur += 2;				// 現在位置もずれる
						flag_end = 1;			// 処理終了
					}
				}
			}
			//--- ロゴ終了部分に追加 ---
			if ((frm_cur_ed  < frm_in_e - 2) &&
				(frm_next_st > frm_in_e + 2 || ncur+2 >= presult->num)){
				// 現在終了と追加が重なる場合
				if (frm_cur_ed >= frm_in_s - 2){
					presult->frm[ncur+1] = frm_in_e;
				}
				// 現在終了と追加が重ならない場合、ロゴセット追加
				else{
					if (presult->num < LOGO_FIND_MAX-1){
						for(j=presult->num - 1; j >= ncur+2; j--){
							presult->frm[j+2] = presult->frm[j];
						}
						presult->num += 2;
						presult->frm[ncur+2] = frm_in_s;
						presult->frm[ncur+3] = frm_in_e;
					}
				}
			}
			//--- 次の位置設定 ---
			frm_last_ed = presult->frm[ncur+1];
			ncur += 2;
			if (frm_next_st >= frm_in_e - 2){
				flag_end = 1;
			}
		}
	}
	return 1;
}

//---------------------------------------------------------------------
// 最終結果にロゴカットを追加
//---------------------------------------------------------------------
int cmd_result_setauto_cut(LAST_RESULTREC *presult, int frm_in_s, int frm_in_e){
	int j, k;
	int flag_end;
	int frm_cur_st, frm_cur_ed;
	int frm_tmp;
	int ncur;

	//--- ロゴ存在なし ---
	if (presult->num <= 1){
	}
	//--- 通常処理 ---
	else{
		flag_end = 0;
		ncur = 0;
		while(flag_end == 0 && ncur < presult->num - 1){
			frm_cur_st = presult->frm[ncur];
			frm_cur_ed = presult->frm[ncur+1];
			//--- 全カット処理 ---
			if (frm_cur_st >= frm_in_s - 2 && frm_cur_ed <= frm_in_e + 2){
				k = 0;
				if (ncur+2 < presult->num){
					k = 2;
					frm_tmp = presult->frm[ncur + k];
					while(frm_tmp >= frm_in_s - 2 && frm_tmp <= frm_in_e + 2 &&
							(ncur + k + 2) < presult->num){
						k += 2;
						frm_tmp = presult->frm[ncur + k];
					}
					
					if (frm_tmp < frm_in_s - 2 || frm_tmp > frm_in_e + 2){
						k -= 2;
					}
				}
				k += 2;
				// ロゴ全カットで後ろにまだロゴがある場合つめる
				if (ncur + k < presult->num){
					for(j=ncur; j < presult->num - k; j++){
						presult->frm[j]   = presult->frm[j+k];
					}
				}
				presult->num -= k;
				// 改めて現在位置設定
				frm_cur_st = presult->frm[ncur];
				frm_cur_ed = presult->frm[ncur+1];
			}
			//--- 範囲外 ---
			if (frm_cur_st >= frm_in_e - 2 || frm_cur_ed <= frm_in_s + 2){
			}
			//--- 内部カットでロゴ分離 ---
			else if (frm_cur_st < frm_in_s - 2 && frm_cur_ed > frm_in_e + 2){
				if (presult->num < LOGO_FIND_MAX-1){
					for(j = presult->num - 1; j >= ncur + 1 ; j--){
						presult->frm[j+2] = presult->frm[j];
					}
					presult->frm[ncur+1] = frm_in_s - 1;
					presult->frm[ncur+2] = frm_in_e + 1;
					presult->num += 2;
				}
			}
			//--- ロゴ左側をカット ---
			else if (frm_cur_st >= frm_in_s - 2 && frm_cur_ed > frm_in_e + 2){
				presult->frm[ncur] = frm_in_e + 1;
			}
			//--- ロゴ右側をカット ---
			else if (frm_cur_st < frm_in_s - 2 && frm_cur_ed <= frm_in_e + 2){
				presult->frm[ncur+1] = frm_in_s - 1;
			}
			//--- 次の位置設定 ---
			if (presult->frm[ncur+1] >= frm_in_e - 2){
				flag_end = 1;
			}
			ncur += 2;
		}
	}
	return 1;
}

//---------------------------------------------------------------------
// 最終結果にAutoCut/AutoAdd結果を追加
//---------------------------------------------------------------------
int cmd_result_setauto(LAST_RESULTREC *presult, const LOGO_RESULTREC *plogo){
	int i;
	int frm_now_s, frm_now_e;
	int frm_last_s, frm_last_e;
	int cmdtype;
	int flag_1st;

	frm_last_s = 0;
	frm_last_e = 0;
	flag_1st = 1;
	for(i=1; i<plogo->num_scpos; i++){
		if (plogo->frm_scpos_s[i] == 0){				// 0フレームは表示しない
		}
//		else if (plogo->stat_scpos[i] > 1 || plogo->nchk_scpos_1st == i){
		else if (plogo->stat_scpos[i] > 1){
			frm_now_s = plogo->frm_scpos_s[i];
			frm_now_e = plogo->frm_scpos_e[i];
			cmdtype = 0;
			switch(plogo->arstat_sc_e[i]){
				case D_SCINT_L_TRKEEP :
				case D_SCINT_L_EC :
				case D_SCINT_L_LGADD :
				case D_SCINT_N_LGADD :
					cmdtype = 1;
					break;
				case D_SCINT_L_ECCUT:
				case D_SCINT_L_LGCUT :
				case D_SCINT_N_LGCUT :
					cmdtype = 2;
					break;
				case D_SCINT_L_TRCUT :
					if (plogo->prmvar.flag_cuttr == 1){
						cmdtype = 2;
					}
					else if (plogo->prmvar.flag_addlogo == 1){
						cmdtype = 1;
					}
					break;
				case D_SCINT_L_SP :
					cmdtype = (plogo->prmvar.flag_cutsp) + 1;
					break;
				case D_SCINT_UNKNOWN :
				case D_SCINT_N_OTHER :
					if (flag_1st == 0 && i < plogo->num_scpos - 1){
						if (plogo->prmvar.flag_adduc == 1){
							cmdtype = 1;
						}
					}
					break;
				case D_SCINT_L_UNIT :
				case D_SCINT_L_OTHER :
				case D_SCINT_L_TRRAW :
				case D_SCINT_L_MIXED :
				case D_SCINT_B_UNIT  :
				case D_SCINT_B_OTHER :
					if (plogo->prmvar.flag_addlogo == 1){
						cmdtype = 1;
					}
					break;
			}
			if (cmdtype == 1){
				cmd_result_setauto_add(presult, frm_last_s, frm_now_e);
			}
			else if (cmdtype == 2){
				cmd_result_setauto_cut(presult, frm_last_e + 1, frm_now_s - 1);
			}
			frm_last_s = frm_now_s;
			frm_last_e = frm_now_e;
			flag_1st = 0;
		}
	}
	return 1;
}


//---------------------------------------------------------------------
// コマンド結果を最終データとして格納
//---------------------------------------------------------------------
int cmd_result(LAST_RESULTREC *presult, const LOGO_RESULTREC *plogo){
	int i, num;
	int frm_new, frm_rise, frm_fall;
	int flag_lastunit;
	int first;

	first = 1;							// 一番最初だけ１
	num = 0;							// 偶数：開始エッジ待ち 奇数：終了エッジ待ち
	frm_rise = 0;						// １つ前の開始エッジ
	frm_fall = 0;						// １つ前の終了エッジ
	flag_lastunit = 0;					// １つ前の独立フレームフラグ
	for(i=0; i<plogo->num_find; i++){
		if (plogo->flag_rise[i] == 1){		// 開始エッジが決定している場合
			if (num % 2 == 0){
				frm_new = plogo->frm_result_rise[i];
				if (frm_new > frm_fall + 2 || first == 1 ||
					plogo->flag_unit[i] == 1 || flag_lastunit == 1){
					flag_lastunit = 0;
					first = 0;
					frm_rise = frm_new;
					presult->frm[num++] = frm_rise;
				}
				else if (num > 0){			// cancel last fall
					num --;
				}
			}
		}

		if (plogo->flag_fall[i] == 1){		// 終了エッジが決定している場合
			if ((num % 2 == 1) || (first == 1)){
				frm_new = plogo->frm_result_fall[i];
				if (num % 2 == 0){			// if first point is fall edge
					first = 0;
					presult->frm[num++] = frm_rise;
				}
				if (frm_rise + 2 < frm_new){	// 開始エッジから3フレーム以上離れている
					frm_fall = frm_new;
					presult->frm[num++] = frm_fall;
					if (plogo->flag_unit[i] == 1){
						flag_lastunit = 1;		// 独立フレームフラグ
					}
				}
				else{							// cancel last rise
					num --;
				}
			}
		}
	}
	if (num % 2 == 1){		// if last point is rise edge, set last frame
		presult->frm[num++] = plogo->frm_scpos_e[plogo->num_scpos - 1];
	}
	presult->num = num;

	// AutoCut/AutoAddコマンド存在時は補正
	if (plogo->flag_autosetup > 0){
		cmd_result_setauto(presult, plogo);
	}

	return num;
}


//---------------------------------------------------------------------
// ロゴ区間が極端に短い場合はロゴなしとして扱う処理
//---------------------------------------------------------------------
void cmd_preproc_minlogo(LOGO_RESULTREC *plogo, REGVARREC *regvar){
	int n;
	int sum_frm;

	sum_frm = 0;
	for(n=0; n<plogo->num_find; n++){
		sum_frm += (plogo->frm_fall[n] - plogo->frm_rise[n] + 1);
	}
	if (sum_frm < plogo->prmvar.frm_wlogo_summin){
		plogo->num_find = 1;
		plogo->frm_rise[0] = 0;
		plogo->frm_fall[0] = plogo->frm_scpos_e[plogo->num_scpos-1];
		plogo->frm_rise_l[0] = 0;
		plogo->frm_rise_r[0] = 0;
		plogo->frm_fall_l[0] = plogo->frm_fall[0];
		plogo->frm_fall_r[0] = plogo->frm_fall[0];
		plogo->fade_rise[0] = 0;
		plogo->fade_fall[0] = 0;
		plogo->intl_rise[0] = 0;
		plogo->intl_fall[0] = 0;
		plogo->stat_rise[0] = 0;
		plogo->stat_fall[0] = 0;
		plogo->flag_rise[0] = 0;
		plogo->flag_fall[0] = 0;
		// ロゴ読み込みなしに変更
		plogo->flag_nologo = 1;

		// システム変数を更新
		set_regvar(regvar, "NOLOGO", "1", 0);
	}
	return;
}


//---------------------------------------------------------------------
// If文処理
//---------------------------------------------------------------------
int cmd_start_cmdif(CMDIFREC *pcmdif, int numcmd, int flag_cond){
	int ifskip;
	int ifdepth;

	ifskip  = pcmdif->ifskip;
	ifdepth = pcmdif->ifdepth;
	if (numcmd == 1){								// If文
		if (ifdepth >= IFDEPTH_MAX){
			fprintf(stderr, "error: max nest of IF is %d.\n", IFDEPTH_MAX);
			ifskip = 1;
		}
		else if (ifskip > 0){
			ifskip = 1;
			pcmdif->ifstate[ifdepth] = -1;			// 条件：終了後
		}
		else if (flag_cond == 0){
			ifskip = 1;
			pcmdif->ifstate[ifdepth] = 0;			// 条件：未実行
		}
		else{
			ifskip = 0;
			pcmdif->ifstate[ifdepth] = 1;			// 条件：実行
		}
		ifdepth ++;
	}
	else if (numcmd == 2){							// EndIf文
		if (ifdepth == 0){
			fprintf(stderr, "error: too many EndIf.\n");
			ifskip = 0;
		}
		else{
			ifdepth --;
			if (ifdepth >= IFDEPTH_MAX){			// 未実装多重If
				ifskip = 1;
			}
			else if (ifdepth == 0){					// If処理完全終了
				ifskip = 0;
			}
			else{									// １階層終了
				if (pcmdif->ifstate[ifdepth-1] == 1){
					ifskip = 0;
				}
				else{
					ifskip = 1;
				}
			}
		}
	}
	else if (numcmd == 3){							// Else,ElsIf文
		if (ifdepth == 0){
			fprintf(stderr, "error: not exist 'If' but exist 'Else/ElsIf' .\n");
		}
		else if (ifdepth > IFDEPTH_MAX){			// 未実装多重If
			ifskip = 1;
		}
		else if (pcmdif->ifstate[ifdepth-1] < 0){	// 終了後のElseは無視
			ifskip = 1;
		}
		else if (pcmdif->ifstate[ifdepth-1] > 0){	// 実行後に出るElseは終了フラグ設定
			ifskip = 1;
			pcmdif->ifstate[ifdepth-1] = -1;		// 条件：終了後
		}
		else if (flag_cond == 0){					// 未実行で今回も条件を満たさない時
			ifskip = 1;
			pcmdif->ifstate[ifdepth-1] = 0;			// 条件：未実行
		}
		else{										// 未実行で条件を満たす時
			ifskip = 0;
			pcmdif->ifstate[ifdepth-1] = 1;			// 条件：実行
		}
	}
	pcmdif->ifskip  = ifskip;
	pcmdif->ifdepth = ifdepth;

	return 0;
}

//---------------------------------------------------------------------
// コマンド読み込み・実行開始
//---------------------------------------------------------------------
int cmd_start_file(LAST_RESULTREC *presult, LOGO_RESULTREC *plogo, REGVARREC *regvar, const char *fname, int loop){
	FILE *fp;
	int ret, exe_command;
	int len, pos;
	char buf[SIZE_BUF_MAX];
	char newname[SIZE_BUF_MAX];
	CMDSETREC      cmdset;
	CMDIFREC       cmdif;

	exe_command = 0;
	cmdif.ifdepth = 0;
	cmdif.ifskip = 0;

	fp = fopen(fname, "r");
	if (!fp){
		fprintf(stderr, "error: failed to open '%s'\n", fname);
		return 2;
	}
	while( fgets(buf, SIZE_BUF_MAX, fp) != NULL ){
		sprintf(newname, "%d", exe_command);
		set_regvar(regvar, "LASTEXE", newname, 0);		// 前回の実行状態を変数に設定
		ret = cmd_analyze(&cmdset, regvar, &(plogo->prmvar), buf, cmdif.ifskip);	// コマンド解析
		cmdset.exe_last = exe_command;					// 前コマンドの実行有無を代入
		if (ret > 0){									// IF等条件分岐文の処理
			if (ret < 10){
				cmd_start_cmdif(&cmdif, ret, cmdset.flag_cond);
//				fprintf(stderr, "%d,%d : %s", cmdif.ifdepth, cmdif.ifskip, buf);
			}
			else if (cmdif.ifskip > 0){					// If分岐で該当しない状態
			}
			else if (ret == 11){						// Call文処理
				pos = get_str(newname, buf, cmdset.pos_str);
				if (pos >= 0){							// pathを付加
					len = strlen(plogo->prmvar.path_file) + strlen(newname);
					if (len < SIZE_BUF_MAX){			// バッファオーバーチェック
						strcpy(newname, plogo->prmvar.path_file);
						len = strlen(newname);
						pos = get_str(&(newname[len]), buf, cmdset.pos_str);
					}
					else{
						pos = -1;
						fprintf(stderr, "error: too long call path\n");
					}
				}
				if (pos >= 0){							// Call実行
					loop ++;
					if (loop < 10){						// 再帰呼び出しは10回まで
						fprintf(stderr, "call:%s\n", newname);
						cmd_start_file(presult, plogo, regvar, newname, loop);
					}
					else{
						// 無限呼び出しによるバッファオーバーフロー防止のため
						fprintf(stderr, "error: many recursive call(%s)\n", buf);
					}
				}
				else{
					fprintf(stderr, "error: wrong argument(%s)\n", buf);
				}
			}
			else if (ret == 12){						// Echo文処理
			}
		}
		else if (ret >= 0){
			if (cmdif.ifskip > 0){					// If分岐で該当しない状態
			}
			else if (strcmp(cmdset.cmdname, "")){
				if (plogo->flag_exe1st > 0){		// 初回のみのチェック
					plogo->flag_exe1st = 0;
					cmd_preproc_minlogo(plogo, regvar);
				}
				exe_command = cmd_execute(plogo, &cmdset);
			}
		}
		else{
			exe_command = 0;
			if (ret == -1){
				fprintf(stderr, "error: wrong argument(%s)\n", buf);
			}
			else if (ret == -2){
				fprintf(stderr, "error: wrong command(%s)\n", buf);
			}
			else if (ret == -3){
				fprintf(stderr, "error: need Start or End(%s)\n", buf);
			}
			else if (ret == -4){
				if (cmdif.ifskip == 0){
					fprintf(stderr, "error: failed variable setting(%s)\n", buf);
				}
			}
			else if (ret == -5){
				fprintf(stderr, "error: need auto command TR/SP/EC(%s)\n", buf);
			}
		}
	}
	fclose(fp);
	if (cmdif.ifdepth > 0){
		fprintf(stderr, "error : EndIf is not found\n");
	}

	return ret;
}

//---------------------------------------------------------------------
// コマンド実行結果を初期化
//---------------------------------------------------------------------
void cmd_clear_result(LOGO_RESULTREC *plogo){
	int i;

	for(i=0; i<plogo->num_find; i++){
		plogo->flag_unit[i] = 0;
		plogo->flag_edge_rise[i] = 0;
		plogo->flag_edge_fall[i] = 0;
		plogo->flag_rise[i] = 0;
		plogo->flag_fall[i] = 0;
		plogo->frm_result_rise[i] = 0;
		plogo->frm_result_fall[i] = 0;
	}
	plogo->flag_exe1st = 1;
}

//---------------------------------------------------------------------
// ファイル名のフォルダ部分を取得
//---------------------------------------------------------------------
int cmd_getfilepath(LOGO_RESULTREC *plogo, const char *fname){
	int pos;

	pos = get_str_filepath(plogo->prmvar.path_file, fname, 0);
	return pos;
}


//---------------------------------------------------------------------
// 初期設定変数
//---------------------------------------------------------------------
void cmd_systemreg(LOGO_RESULTREC *plogo, REGVARREC *regvar){
	char strval[SIZE_BUF_MAX];

	sprintf(strval, "%d", (int)plogo->frm_totalmax);
	set_regvar(regvar, "MAXFRAME", strval, 0);

	sprintf(strval, "%d", (int)plogo->flag_nologo);
	set_regvar(regvar, "NOLOGO", strval, 0);

	set_regvar(regvar, "HEADFRAME", "-1", 0);
	set_regvar(regvar, "TAILFRAME", "-1", 0);
}


//---------------------------------------------------------------------
// コマンド開始
//---------------------------------------------------------------------
int cmd_start(LAST_RESULTREC *presult, LOGO_RESULTREC *plogo, REGVARREC *regvar, const char *fname){
	int ret;

	cmd_getfilepath(plogo, fname);
	cmd_systemreg(plogo, regvar);
	cmd_clear_result(plogo);
	ret = cmd_start_file(presult, plogo, regvar, fname, 0);
	cmd_result(presult, plogo);

	return ret;
}


//---------------------------------------------------------------------
// 範囲内にあるロゴ表示期間の秒数を取得
//---------------------------------------------------------------------
int output_component_getlogosec(LOGO_RESULTREC *plogo, int frm_s, int frm_e){
	int n;
	int tmp_s, tmp_e;
	int sum_frm;
	int nsec;

	sum_frm = 0;
	for(n=0; n<plogo->num_find; n++){
		// 範囲内にロゴ表示期間がある場合
		if (plogo->frm_rise[n] + 30 < frm_e && plogo->frm_fall[n] > frm_s + 30){
			if (plogo->frm_rise[n] < frm_s){
				tmp_s = frm_s;
			}
			else{
				tmp_s = plogo->frm_rise[n];
			}
			if (plogo->frm_fall[n] > frm_e){
				tmp_e = frm_e;
			}
			else{
				tmp_e = plogo->frm_fall[n];
			}
			// ロゴ表示期間を追加
			if (tmp_s < tmp_e){
				sum_frm += (tmp_e - tmp_s + 1);
			}
		}
	}
	// 秒数に変換してリターン
	nsec = GetSecFromFrm(sum_frm);
	return nsec;
}

//---------------------------------------------------------------------
// 調整後の構成をファイルへ出力
//---------------------------------------------------------------------
int output_component(LOGO_RESULTREC *plogo, const char *outscpfile){
	FILE *fpw;
	int i;
	int frm_last, frm_now_s, frm_now_e, frm_dif;
	int nsec, ndif, nsec_logo;
	char *pstr_arstat;

	if (outscpfile != NULL){
		fpw = fopen(outscpfile, "w");
		if (!fpw){
			fprintf(stderr, "error: failed to open '%s'\n", outscpfile);
			return 2;
		}

		frm_last = 0;
		for(i=1; i<plogo->num_scpos; i++){
			if (plogo->frm_scpos_s[i] == 0){				// 0フレームは表示しない
			}
			else if (plogo->stat_scpos[i] > 1 || plogo->nchk_scpos_1st == i){
				frm_now_s = plogo->frm_scpos_s[i];
				frm_now_e = plogo->frm_scpos_e[i];
				frm_dif = frm_now_s - frm_last;
				nsec = GetSecFromFrm(frm_dif);
				ndif = frm_dif - CnvTimeFrm(nsec, 0);
				// ロゴ期間を取得
				nsec_logo = output_component_getlogosec(plogo, frm_last, frm_now_e);
				// 種類文字列
				if (plogo->flag_autosetup == 0){
					pstr_arstat = "";
				}
				else{
					switch(plogo->arstat_sc_e[i]){
						case D_SCINT_N_UNIT :
						case D_SCINT_N_AUNIT :
						case D_SCINT_N_BUNIT :
							pstr_arstat = ":CM";
							break;
						case D_SCINT_L_TRKEEP :
							pstr_arstat = ":Trailer(add)";
							break;
						case D_SCINT_L_ECCUT :
							pstr_arstat = ":Trailer(cut)";
							break;
						case D_SCINT_L_TRRAW :
							pstr_arstat = ":Trailer";
							break;
						case D_SCINT_L_TRCUT :
							if (plogo->prmvar.flag_cuttr == 1){
								pstr_arstat = ":Trailer(cut)";
							}
							else{
								pstr_arstat = ":Trailer(cut-cancel)";
							}
							break;
						case D_SCINT_L_SP :
							if (plogo->prmvar.flag_cutsp == 1){
								pstr_arstat = ":Sponsor(cut)";
							}
							else{
								pstr_arstat = ":Sponsor(add)";
							}
							break;
						case D_SCINT_L_EC :
							pstr_arstat = ":Endcard(add)";
							break;
						case D_SCINT_L_LGCUT :
							pstr_arstat = ":L-Edge(cut)";
							break;
						case D_SCINT_L_LGADD :
							pstr_arstat = ":L-Edge(add)";
							break;
						case D_SCINT_N_LGCUT :
							pstr_arstat = ":N-Edge(cut)";
							break;
						case D_SCINT_N_LGADD :
							pstr_arstat = ":N-Edge(add)";
							break;
						case D_SCINT_N_OTHER :
							pstr_arstat = ":Nologo";
							break;
						case D_SCINT_N_TRCUT :
							pstr_arstat = ":Nologo(cut)";
							break;
						case D_SCINT_B_UNIT  :
							pstr_arstat = ":Border15s";
							break;
						case D_SCINT_B_OTHER :
							pstr_arstat = ":Border";
							break;
						case D_SCINT_L_UNIT :
						case D_SCINT_L_OTHER :
							pstr_arstat = ":L";
							break;
						case D_SCINT_L_MIXED :
							pstr_arstat = ":Mix";
							break;
						default :
							pstr_arstat = ":";
					}
				}
				// 表示
				fprintf(fpw, "%6d %6d %4d %3d %4d %s\n", frm_last, frm_now_e, nsec, ndif, nsec_logo, pstr_arstat);
				frm_last = frm_now_s;
			}
		}
		fclose(fpw);
	}
	return 0;
}


//---------------------------------------------------------------------
// 結果をファイルへ出力
//---------------------------------------------------------------------
int output_result(const LAST_RESULTREC *presult, const char *outname){
	FILE *fpw;
	int i;
	int frm_rise, frm_fall;

	fpw = fopen(outname, "w");
	if (!fpw){
		fprintf(stderr, "error: failed to create '%s'\n", outname);
		return 2;
	}

	for(i=0; i<presult->num; i+=2){
		if (i > 0){
			fprintf(fpw, " ++ ");
		}
		frm_rise = presult->frm[i];
		frm_fall = presult->frm[i+1];
		fprintf(fpw, "Trim(%d,%d)", frm_rise, frm_fall);
	}
	fprintf(fpw, "\n");
	fclose(fpw);
	return 0;
}


//---------------------------------------------------------------------
// オプションフラグを読み込み
//---------------------------------------------------------------------
int input_flags(REGVARREC *regvar, const char *strin){
	int pos;
	char buf[SIZE_BUF_MAX];

	pos = 0;
	while(pos >= 0){
		pos = get_str_word(buf, strin, pos);
		if (pos >= 0){
			set_regvar(regvar, buf, "1", 0);
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// データ初期化
//---------------------------------------------------------------------
void clear_initial_param(LOGO_RESULTREC *plogo){
	plogo->frm_cutin      = 0;
	plogo->frm_cutout     = 0;
	plogo->frm_lastcut    = 0;
	plogo->half_cutin     = 0;
	plogo->half_cutout    = 0;
	plogo->flag_autosetup = 0;
	plogo->flag_nologo    = 0;
	plogo->fix_cutin      = 0;
	plogo->fix_cutout     = 0;

	plogo->prmvar.frm_wlogo_trmax = CnvTimeFrm(120, 0);
	plogo->prmvar.frm_wcomp_trmax = CnvTimeFrm(60, 0);
	plogo->prmvar.frm_wlogo_sftmrg = CnvTimeFrm(4, 200);
	plogo->prmvar.frm_wcomp_last  = 0;
	plogo->prmvar.frm_wlogo_summin = CnvTimeFrm(20, 0);
	plogo->prmvar.sec_wcomp_spmin = 6;
	plogo->prmvar.sec_wcomp_spmax = 14;
	plogo->prmvar.flag_cuttr   = 1;
	plogo->prmvar.flag_cutsp   = 0;
	plogo->prmvar.flag_addlogo = 1;
	plogo->prmvar.flag_adduc   = 0;
	plogo->prmvar.flag_edge    = 0;

	strcpy(plogo->prmvar.path_file, "");
}


int main(int argc, const char* argv[]){
	const char* logofile = NULL;
	const char* scpfile = NULL;
	const char* cmdfile = NULL;
	const char* outfile = NULL;
	const char* outscpfile = NULL;
	int i;
	int verbose;
	REGVARREC      regvar;
	LOGO_RESULTREC logo_result;
	LAST_RESULTREC last_result;
	char* tmpstr;

	//--- initialize ---
	clear_initial_param(&logo_result);
	regvar.var_num = 0;


	//--- get argument ---
	for(i=1; i<argc; i++){
        if(argv[i][0] == '-' && argv[i][1] != 0) {
            if(!stricmp(argv[i], "-v"))
                verbose = 1;
            else if (!stricmp(argv[i], "-ver")){
				printf("join_logo_scp ver2.1\n");
				exit(1);
			}
            else if (!stricmp(argv[i], "-cutmrgin")){
				if (i > argc-2){
					fprintf(stderr, "-cutmrgin needs an argument\n");
					return 2;
				}
				logo_result.frm_cutin = atoi(argv[++i]);
				logo_result.fix_cutin = 1;
				tmpstr = strchr(argv[i], '.');
				if (tmpstr != NULL){
					if (tmpstr[1] == '5'){
						logo_result.half_cutin = 1;
					}
				}
			}
            else if (!stricmp(argv[i], "-cutmrgout")){
				if (i > argc-2){
					fprintf(stderr, "-cutmrgout needs an argument\n");
					return 2;
				}
				logo_result.frm_cutout = atoi(argv[++i]);
				logo_result.fix_cutout = 1;
				tmpstr = strchr(argv[i], '.');
				if (tmpstr != NULL){
					if (tmpstr[1] == '5'){
						logo_result.half_cutout = 1;
					}
				}
			}
            else if (!stricmp(argv[i], "-lastcut")){
				if (i > argc-2){
					fprintf(stderr, "-lastcut needs an argument\n");
					return 2;
				}
				logo_result.frm_lastcut = atoi(argv[++i]);
			}
			else if (!stricmp(argv[i], "-inlogo")){
				if (i > argc-2){
					fprintf(stderr, "-inlogo needs an argument\n");
					return 2;
				}
				logofile = argv[++i];
			}
			else if (!stricmp(argv[i], "-inscp")){
				if (i > argc-2){
					fprintf(stderr, "-inscp needs an argument\n");
					return 2;
				}
				scpfile = argv[++i];
			}
			else if (!stricmp(argv[i], "-incmd")){
				if (i > argc-2){
					fprintf(stderr, "-incmd needs an argument\n");
					return 2;
				}
				cmdfile = argv[++i];
			}
			else if (!stricmp(argv[i], "-o")){
				if (i > argc-2){
					fprintf(stderr, "-o needs an argument\n");
					return 2;
				}
				outfile = argv[++i];
			}
			else if (!stricmp(argv[i], "-oscp")){
				if (i > argc-2){
					fprintf(stderr, "-oscp needs an argument\n");
					return 2;
				}
				outscpfile = argv[++i];
			}
			else if (!stricmp(argv[i], "-flags")){
				if (i > argc-2){
					fprintf(stderr, "-flags needs an argument\n");
					return 2;
				}
				if (input_flags(&regvar, argv[++i]) < 0){
					fprintf(stderr, "-flags bad argument\n");
					return 2;
				}
			}
			else if (!stricmp(argv[i], "-set")){
				if (i > argc-3){
					fprintf(stderr, "-set needs two arguments\n");
					return 2;
				}
				if (set_regvar(&regvar, argv[i+1], argv[i+2], 0) < 0){
					fprintf(stderr, "-set bad argument\n");
					return 2;
				}
				i += 2;
			}
			else{
				fprintf(stderr, "unknown option(%s)\n", argv[i]);
				return 2;
			}
		}
		else{
			fprintf(stderr, "unknown argument(%s)\n", argv[i]);
			return 2;
		}
	}
	if (logofile == NULL){
		fprintf(stderr, "warning: not found logo file(-inlogo filename)\n");
		logo_result.flag_nologo = 1;
	}
	if (scpfile == NULL){
		fprintf(stderr, "error: need -inscp filename\n");
		return 2;
	}
	if (cmdfile == NULL){
		fprintf(stderr, "error: need -incmd filename\n");
		return 2;
	}
	if (outfile == NULL){
		fprintf(stderr, "error: need -o filename\n");
		return 2;
	}

	//--- main procedure ---
	if (logofile != NULL){
		read_logoframe(&logo_result, logofile);
	}
	read_scpos(&logo_result, scpfile);
	adjust_indata(&logo_result);
	cmd_start(&last_result, &logo_result, &regvar, cmdfile);

	//--- for debug ---
	if (verbose == 1){
		printf("logo total:%ld\n", logo_result.num_find);
		for(i=0; i<logo_result.num_find; i++){
			printf("%d : %ld %ld\n", i, logo_result.frm_rise[i], logo_result.frm_fall[i]);
		}

		printf("scptotal:%ld\n", logo_result.num_scpos);
		for(i=0; i<logo_result.num_scpos; i++){
//			printf("(%ld %ld)", logo_result.frm_scpos_e[i], logo_result.frm_scpos_s[i]);
			printf("(%ld %ld %d)", logo_result.frm_scpos_e[i], logo_result.frm_scpos_s[i], logo_result.stat_scpos[i]);
			printf("[%ld %ld] ", logo_result.frm_smute_s[i], logo_result.frm_smute_e[i]);
		}
		printf("\n");
	}

	//--- output information ---
	output_component(&logo_result, outscpfile);


	//--- output result ---
	output_result(&last_result, outfile);

	return 0;
}

