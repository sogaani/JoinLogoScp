//
// join_logo_scp 共通処理
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "join_logo_scp.h"


//---------------------------------------------------------------------
// 注意：グローバル変数使用
// 外部参照禁止（private変数扱い）
//---------------------------------------------------------------------
static int m_frate_n = 30000;
static int m_frate_d = 1001;

//---------------------------------------------------------------------
// 30フレーム/1秒に直すための補正
//---------------------------------------------------------------------
int RevFrm(int frm){
	if (m_frate_n == 30000 && m_frate_d == 1001){
		return (frm + ((frm + 140) / 1000));
	}
	else{
		return ((frm * m_frate_d * 30 + (m_frate_n/2)) / m_frate_n);
	}
}

//---------------------------------------------------------------------
// 時間をフレーム数に変換
//---------------------------------------------------------------------
int CnvTimeFrm(int sec, int d){
	return (((sec * m_frate_n) + (d * m_frate_n / 1000) + (m_frate_d/2)) / m_frate_d);
}

//---------------------------------------------------------------------
// フレーム数に対応する秒数を取得
//---------------------------------------------------------------------
int GetSecFromFrm(int frm){
	return ((RevFrm(frm) + 15) / 30);
}

//---------------------------------------------------------------------
// フレームレート変更関数（未使用）
//---------------------------------------------------------------------
int ChangeFrameRate(int n, int d){
	m_frate_n = n;
	m_frate_d = d;
	return 1;
}


//---------------------------------------------------------------------
// 文字列から１単語を読み込み
// src文字列の位置posから１単語を読み込みdstに出力
// 読み終わった位置を返り値（失敗時は-1）
//---------------------------------------------------------------------
int get_str(char *dst, const char *src, int pos){
	int len;
	int flag_quote;

	if (pos < 0) return pos;

	flag_quote = 0;
	len = 0;
	while( src[pos] == ' ' ){
		pos ++;
	}
	if (src[pos] == '\"'){
		flag_quote ++;
		pos ++;
	}
	while( src[pos] != '\0' && src[pos] != ' ' && flag_quote < 2 && len < SIZE_BUF_MAX){
		if ( src[pos] > ' ' || src[pos] < 0){
			if (src[pos] == '\"' && flag_quote > 0){
				flag_quote = 2;
			}
			else{
				dst[len++] = src[pos];
			}
		}
		pos ++;
	}
	if (len >= SIZE_BUF_MAX || len == 0){
		pos = -1;
	}
	else{
		dst[len] = '\0';
	}
	return pos;
}

//---------------------------------------------------------------------
// 文字列から１単語を読み込み（","を区切りとして認識）
// src文字列の位置posから１単語を読み込みdstに出力
// 読み終わった位置を返り値（失敗時は-1）
//---------------------------------------------------------------------
int get_str_word(char *dst, const char *src, int pos){
	int len;

	if (pos < 0) return pos;

	len = 0;
	while( src[pos] == ' ' ){
		pos ++;
	}
	while( src[pos] != '\0' && src[pos] != ' ' && src[pos] != ',' && len < SIZE_BUF_MAX){
		if ( src[pos] > ' ' || src[pos] < 0 ){
			dst[len++] = src[pos];
		}
		pos ++;
	}
	if (len >= SIZE_BUF_MAX || len == 0){
		pos = -1;
	}
	else{
		dst[len] = '\0';
		if (src[pos] == ','){
			pos ++;
		}
	}
	return pos;
}

//---------------------------------------------------------------------
// 文字列から１単語を読み込み数値として格納（数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_num(int *result, const char *src, int pos){
	int len;
	int err;
	int val, total_val;
	int val_sign;

	if (pos < 0) return pos;

	len = 0;
	err = 0;
	val = 0;
	total_val = 0;
	val_sign = 1;
	while( src[pos] == ' ' ){
		pos ++;
	}
	while( (src[pos] != '\0' && src[pos] != ' ') && len < SIZE_BUF_MAX){
		if ( src[pos] > ' ' ){
			if ( (src[pos] < '0'  || src[pos] > '9') &&
				 (src[pos] != '+' && src[pos] != '-')){
				err = 1;
			}
			if (err == 0){
				if (src[pos] == '+'){
					total_val += val * val_sign;
					val_sign = 1;
					val = 0;
				}
				else if (src[pos] == '-'){
					total_val += val * val_sign;
					val_sign = -1;
					val = 0;
				}
				else{
					val = val * 10 + (src[pos] - '0');
				}
				len ++;
			}
		}
		pos ++;
	}
	total_val += val * val_sign;
	if (len >= SIZE_BUF_MAX || len == 0){
		pos = -1;
	}

	if (err == 0){
		*result = total_val;
	}
	else{
		pos = -1;
	}
	return pos;
}

//---------------------------------------------------------------------
// 文字列から１単語を読み込みlong数値として格納（数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_numl(long *result, const char *src, int pos){
	int tmp_result;
	int ret;

	tmp_result = *result;
	ret = get_str_num(&tmp_result, src, pos);
	if (ret >= 0){
		*result = (long) tmp_result;
	}
	return ret;
}


//---------------------------------------------------------------------
// 文字列から時間またはフレーム数を読む（時間・数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//  type : 時間指定時の変換方法  0=フレーム数  1:秒数
//---------------------------------------------------------------------
int get_str_sub_time(int *result, const char *src, int pos, int type){
	int len;
	int err;
	int flag_time;
	int flag_minus;
	int val, val_decimal, tmp_val, val_total;
	int mul;

	if (pos < 0) return pos;

	len = 0;
	err = 0;
	val_total = 0;
	while( src[pos] == ' ' ){
		pos ++;
	}
	while((src[pos] >= '0' && src[pos] <= '9') || (src[pos] == ':') ||
			(src[pos] == '+') || (src[pos] == '-')){
		flag_time = 0;
		flag_minus = 0;
		val = 0;
		val_decimal = 0;
		tmp_val = 0;
		if (src[pos] == '-'){
			pos ++;
			flag_minus = 1;
		}
		else if (src[pos] == '+'){
			pos ++;
		}
		while( (src[pos] >= '0' && src[pos] <= '9') || (src[pos] == ':') ){
			if (src[pos] == ':'){
				flag_time = 1;
				val = (val + tmp_val) * 60;
				tmp_val = 0;
			}
			else if (src[pos] >= '0' && src[pos] <= '9'){
				tmp_val = tmp_val * 10 + (src[pos] - '0');
				len ++;
			}
			pos ++;
		}
		val = val + tmp_val;

		// 小数以下取得
		if (src[pos] == '.'){
			flag_time = 1;
			pos ++;
			mul = 100;
			while(src[pos] >= '0' && src[pos] <= '9'){
				val_decimal += (src[pos] - '0') * mul;
				if (mul >= 10){
					mul = mul / 10;
				}
				else{
					mul = 0;
				}
				pos ++;
			}
		}

		// 時間形式だった場合は値を変換
		if (flag_time == 1){
			if (type == 0){				// フレーム数に変換
				val = CnvTimeFrm(val, val_decimal);
			}
			else{						// 秒数に変換
				val = val + ((val_decimal + 500) / 1000);
			}
		}
		// 符号追加
		if (flag_minus == 1){
			val = val * -1;
		}
		// 合計
		val_total += val;
	}

	// 時間・数値以外があれば読み込み失敗を返す
	while(src[pos] > ' '){
		err = 1;
		pos ++;
	}


	if (len == 0){
		err = 1;
		pos = -1;
	}

	if (err == 0){
		*result = val_total;
	}
	else{
		pos = -1;
	}
	return pos;
}

//---------------------------------------------------------------------
// 文字列から時間またはフレーム数を読む（時間・数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込みt、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_frm(int *result, const char *src, int pos){
	int ret;

	ret = get_str_sub_time(result, src, pos, 0);
	return ret;
}

//---------------------------------------------------------------------
// 文字列から時間またはフレーム数を読む（時間・数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_frml(long *result, const char *src, int pos){
	int tmp_result;
	int ret;

	tmp_result = *result;
	ret = get_str_frm(&tmp_result, src, pos);
	if (ret >= 0){
		*result = (long) tmp_result;
	}
	return ret;
}

//---------------------------------------------------------------------
// 文字列から時間または秒数を読む（時間・数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_sec(int *result, const char *src, int pos){
	int ret;

	ret = get_str_sub_time(result, src, pos, 1);
	return ret;
}

//---------------------------------------------------------------------
// 文字列から時間または秒数を読む（時間・数値以外があれば読み込み失敗を返す）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_secl(long *result, const char *src, int pos){
	int tmp_result;
	int ret;

	tmp_result = *result;
	ret = get_str_sec(&tmp_result, src, pos);
	if (ret >= 0){
		*result = (long) tmp_result;
	}
	return ret;
}

//---------------------------------------------------------------------
// 文字列から１単語を読み込み数値として格納（数値以外があればそこで終了）
// src文字列の位置posから１単語を読み込み、数値をresultに出力
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_numhead(int *result, const char *src, int pos){
	int len;
	int err;
	int val;
	int val_sign;

	if (pos < 0) return pos;

	len = 0;
	err = 0;
	val = 0;
	val_sign = 1;
	while( src[pos] == ' ' ){
		pos ++;
	}
	while( ((src[pos] >= '0' && src[pos] <= '9') ||
			(src[pos] == '-' && len == 0)) &&
		   len < SIZE_BUF_MAX ){
		if (src[pos] == '-'){
			val_sign = -1;
		}
		else{
			val = val * 10 + (src[pos] - '0');
		}
		len ++;
		pos ++;
	}
	if (len >= SIZE_BUF_MAX || len == 0){
		err = 1;
		pos = -1;
	}

	if (err == 0){
		*result = val * val_sign;
	}
	else{
		pos = -1;
	}
	return pos;
}



//---------------------------------------------------------------------
// Shift-JISの２バイト文字チェック
// 2バイト文字だった時は1を返り値とする（1バイト文字は0）
//---------------------------------------------------------------------
int get_str_sjis_multibyte(const char *str){
	unsigned char code;

	code = (unsigned char)*str;
	if ((code >= 0x81 && code <= 0x9F) ||
		(code >= 0xE0 && code <= 0xFC)){		// Shift-JIS 1st-byte
		code = (unsigned char)*(str+1);
		if ((code >= 0x40 && code <= 0x7E) ||
			(code >= 0x80 && code <= 0xFC)){	// Shift-JIS 2nd-byte
			return 1;
		}
	}
	return 0;
}

//---------------------------------------------------------------------
// 文字列からファイルパス部分を取得
// 読み終わった位置を返り値とする（失敗時は-1）
//---------------------------------------------------------------------
int get_str_filepath(char *dst, const char *src, int pos){
	int i;
	int pos_ed, pos_tmp;
	int flag_quote;
	const char *str;

	strcpy(dst, "");
	if (pos < 0) return pos;

	while( src[pos] == ' ' ){
		pos ++;
	}
	// 最初にダブルクォートの有無確認
	flag_quote = 0;
	if (src[pos] == '\"'){
		pos ++;
		flag_quote ++;
	}
	// 開始位置から順番にパス区切り確認
	pos_ed = -1;
	pos_tmp = pos;
	str = &(src[pos_tmp]);
	while( *str != '\0' && flag_quote < 2 && pos_tmp < SIZE_BUF_MAX ){
		if (get_str_sjis_multibyte(str) > 0){	// ２バイト文字確認
			pos_tmp ++;
		}
		else if (*str == '\\' || *str == '/'){
			pos_ed = pos_tmp;				// 終了位置変更
		}
		else if (*str == '\"'){				// ダブルクォートで終了
			flag_quote = 2;
		}
		pos_tmp ++;
		str = &(src[pos_tmp]);
	}
	if (pos_ed < 0){				// ファイルパス部分が見つからない場合
		return -1;
	}
	if (pos_ed - pos >= SIZE_BUF_MAX){		// 文字列長確認
		return -1;
	}
	// ファイルパス部分まで結果書き込み
	for(i=pos; i<=pos_ed; i++){
		*dst = src[i];
		dst ++;
	}
	*dst = '\0';

	return pos_ed + 1;
}


//---------------------------------------------------------------------
// ２フレーム間の秒数を取得
// 入力：
//   frm_src  : 比較元フレーム番号
//   frm_dst  : 比較先フレーム番号
// 出力：
//   ncal_sgn : 比較フレーム大小
//   ncal_sec : フレーム間秒数
//   ncal_dis : フレーム間秒数の誤差
// 返り値：
//   1 : ３秒以上かつ秒単位で誤差が少ない場合（１５秒単位では誤差甘め）
//   0 : それ以外
//---------------------------------------------------------------------
int adjust_calcdif(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst){
	int frm_dif_org, frm_dif_abs, frm_dif;
	int frm_sec, frm_dis;
	int flag;

	frm_dif_org = frm_dst - frm_src;
	frm_dif_abs = abs(frm_dif_org);
	frm_dif     = RevFrm(frm_dif_abs);							// 30フレーム/1秒に補正
	frm_sec     = (frm_dif + 30/2) / 30;
	frm_dis     = abs(frm_sec * 30 - frm_dif);
	if ((frm_dis <= 5 && frm_sec >= 3) ||						// 3秒以上で秒単位切り替えあり
		(frm_dis <= 10 && frm_sec == 10) ||						// 10秒
		(frm_dis <= 10 && frm_sec >= 15 && frm_sec % 15 == 0)){	// 15秒単位切り替え
		if (frm_sec <= 120){									// 2分以内
			flag = 1;
		}
		else{
			flag = 0;
		}
	}
	else{
		flag = 0;
	}
	// 結果格納
	if (frm_dif_org < 0){
		*ncal_sgn = -1;
	}
	else if (frm_dif_org > 0){
		*ncal_sgn = 1;
	}
	else{
		*ncal_sgn = 0;
	}
	*ncal_sec = frm_sec;
	*ncal_dis = frm_dis;

	return flag;
}


//---------------------------------------------------------------------
// ２フレーム間の秒数を取得
// 入力：
//   frm_src  : 比較元フレーム番号
//   frm_dst  : 比較先フレーム番号
// 出力：
//   ncal_sgn : 比較フレーム大小
//   ncal_sec : フレーム間秒数
//   ncal_dis : フレーム間秒数の誤差
// 返り値：
//   2 : 10,15,30,45,60,90,120秒
//   1 : ３秒以上２５秒以下かつ秒単位で誤差が少ない場合
//   0 : それ以外
//---------------------------------------------------------------------
int adjust_calcdif_select(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst){
	int frm_dif_org, frm_dif_abs, frm_dif;
	int frm_sec, frm_dis;
	int flag;

	frm_dif_org = frm_dst - frm_src;
	frm_dif_abs = abs(frm_dif_org);
	frm_dif     = RevFrm(frm_dif_abs);							// 30フレーム/1秒に補正
	frm_sec     = (frm_dif + 30/2) / 30;
	frm_dis     = abs(frm_sec * 30 - frm_dif);
	if (frm_dis <= 10 &&
		(frm_sec == 10 || frm_sec == 15 || frm_sec == 30 || frm_sec == 45 ||
		 frm_sec == 60 || frm_sec == 90 || frm_sec == 120)){
		flag = 2;
	}
	else if (frm_dis <= 5 && (frm_sec >= 3 && frm_sec <= 25)){
		flag = 1;
	}
	else{
		flag = 0;
	}
	// 結果格納
	if (frm_dif_org < 0){
		*ncal_sgn = -1;
	}
	else if (frm_dif_org > 0){
		*ncal_sgn = 1;
	}
	else{
		*ncal_sgn = 0;
	}
	*ncal_sec = frm_sec;
	*ncal_dis = frm_dis;

	return flag;
}


//---------------------------------------------------------------------
// ２フレーム間の秒数を取得（誤差少ない前提）
// 入力：
//   frm_src  : 比較元フレーム番号
//   frm_dst  : 比較先フレーム番号
// 出力：
//   ncal_sgn : 比較フレーム大小
//   ncal_sec : フレーム間秒数
//   ncal_dis : フレーム間秒数の誤差
// 返り値：
//   2 : 10,15,30,45,60,90,120秒
//   1 : ３秒以上２５秒以下かつ秒単位で誤差が少ない場合
//   0 : それ以外
//---------------------------------------------------------------------
int adjust_calcdif_exact(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst){
	int frm_dif_org, frm_dif_abs, frm_dif;
	int frm_sec, frm_dis;
	int flag;

	frm_dif_org = frm_dst - frm_src;
	frm_dif_abs = abs(frm_dif_org);
	frm_dif     = RevFrm(frm_dif_abs);							// 30フレーム/1秒に補正
	frm_sec     = (frm_dif + 30/2) / 30;
	frm_dis     = abs(frm_sec * 30 - frm_dif);
	if (frm_dis <= 5 &&
		(frm_sec == 10 || frm_sec == 15 || frm_sec == 30 || frm_sec == 45 ||
		 frm_sec == 60 || frm_sec == 90 || frm_sec == 120)){
		flag = 2;
	}
	else if (frm_dis <= 4 && (frm_sec >= 3 && frm_sec <= 25)){
		flag = 1;
	}
	else{
		flag = 0;
	}
	// 結果格納
	if (frm_dif_org < 0){
		*ncal_sgn = -1;
	}
	else if (frm_dif_org > 0){
		*ncal_sgn = 1;
	}
	else{
		*ncal_sgn = 0;
	}
	*ncal_sec = frm_sec;
	*ncal_dis = frm_dis;

	return flag;
}


//---------------------------------------------------------------------
// シーンチェンジを挿入
// 返り値：挿入した場所番号
//---------------------------------------------------------------------
int insert_scpos(LOGO_RESULTREC *plogo, int frm_dst_s, int frm_dst_e,
				 int frm_dmute_s, int frm_dmute_e, int stat_scpos_dst, int overwrite){
	int i, num_ins;
	int flag_ins;

	if (plogo->num_scpos >= SCPOS_FIND_MAX){
		return -1;
	}
	if (plogo->num_scpos <= 1){		// シーンチェンジを読み込みできてない場合
		return -1;
	}

	// 挿入場所を検索
	num_ins = 1;
	while(plogo->frm_scpos_s[num_ins] < frm_dst_s && num_ins < plogo->num_scpos - 1){
		num_ins ++;
	}

	// 挿入場所を確保
	flag_ins = 0;
	if (plogo->frm_scpos_s[num_ins] != frm_dst_s || num_ins == plogo->num_scpos - 1){
		flag_ins = 1;
		for(i=plogo->num_scpos - 1; i >= num_ins; i--){
			plogo->frm_scpos_s[i+1] = plogo->frm_scpos_s[i];
			plogo->frm_scpos_e[i+1] = plogo->frm_scpos_e[i];
			plogo->frm_smute_s[i+1] = plogo->frm_smute_s[i];
			plogo->frm_smute_e[i+1] = plogo->frm_smute_e[i];
			plogo->still_scpos[i+1] = plogo->still_scpos[i];
			plogo->arstat_sc_e[i+1] = plogo->arstat_sc_e[i];
			plogo->stat_scpos[i+1]  = plogo->stat_scpos[i];
		}
		plogo->num_scpos ++;
	}
	// 書き込み
	if (flag_ins > 0 || overwrite > 0){
		plogo->frm_scpos_s[num_ins] = frm_dst_s;
		plogo->frm_scpos_e[num_ins] = frm_dst_e;
		plogo->frm_smute_s[num_ins] = frm_dmute_s;
		plogo->frm_smute_e[num_ins] = frm_dmute_e;
		plogo->still_scpos[num_ins] = 0;
		plogo->arstat_sc_e[num_ins] = D_SCINT_UNKNOWN;
	}
	// 状態は上書き時以外でも変更
	plogo->stat_scpos[num_ins]  = stat_scpos_dst;

	return num_ins;
}


