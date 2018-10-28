//
// join_logo_scp Auto系コマンド処理
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "join_logo_scp.h"
#include "join_logo_scp_auto.h"
#include "join_logo_scp_extern.h"


//---------------------------------------------------------------------
// フレーム位置に対応するシーンチェンジ番号を取得
// 出力：
//  返り値  : フレーム位置に対応するシーンチェンジ番号（-1の時該当なし）
//---------------------------------------------------------------------
int auto_getnsc(
	LOGO_RESULTREC *plogo,
	int frm_src,		// 対象とするフレーム位置
	int frm_difmax		// 位置ずれ最大許容のフレーム数
){
	int j;
	int frm_cur;
	int nsc_dst;
	int frmdif_dst, frmdif_tmp;

	nsc_dst = -1;
	frmdif_dst = 0;
	for(j=1; j<plogo->num_scpos-1; j++){
		// 番組構成として認識されているシーンチェンジのみ検索
//		if (plogo->stat_scpos[j] == 2 || plogo->nchk_scpos_1st == j){
		if (plogo->stat_scpos[j] == 2){
			frm_cur = plogo->frm_scpos_s[j];
			// ロゴからの最短距離
			frmdif_tmp = abs(frm_cur - frm_src);
			if (frmdif_tmp <= frm_difmax && (frmdif_tmp < frmdif_dst || nsc_dst < 0)){
				nsc_dst = j;
				frmdif_dst = frmdif_tmp;
			}
		}
	}
	return nsc_dst;
}


//---------------------------------------------------------------------
// ロゴに対応する推測構成切り替わり位置を取得
// 出力：
//   返り値：ロゴ切り替わりに対応するシーンチェンジ番号
//---------------------------------------------------------------------
int auto_getedge_nsc(LOGO_RESULTREC *plogo, int frm_logo, int nedge, int frm_mgn){
	int i;
	int nsc_lrise, nsc_lfall;
	int nsc_rrise, nsc_rfall;
	int nsc_last;
	int nsc_baselogo;
	int frm_cur, frm_last, frm_baselogo;
	int arstat_cur;
	int flag_logo_cur, flag_logo_last;
	int flag_invalid_cur, flag_invalid_last;

	//--- ロゴ前後の切り替わり位置取得 ---
	nsc_lrise = -1;
	nsc_lfall = -1;
	nsc_rrise = -1;
	nsc_rfall = -1;
	nsc_last = 0;
	frm_last = 0;
	flag_logo_last = 0;
	for(i=1; i<plogo->num_scpos; i++){
		if (plogo->stat_scpos[i] == 2){
			frm_cur = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
			if (arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH){
				flag_logo_cur = 1;
			}
			else{
				flag_logo_cur = 0;
			}
			flag_invalid_cur = 0;
			if ((arstat_cur == D_SCINT_L_TRKEEP) ||
				(arstat_cur == D_SCINT_L_TRCUT) ||
				(arstat_cur == D_SCINT_L_TRRAW) ||
				(arstat_cur == D_SCINT_L_ECCUT) ||
				(arstat_cur == D_SCINT_L_EC) ||
				(arstat_cur == D_SCINT_L_SP)){
				flag_invalid_cur = 1;
			}
			if (flag_invalid_cur == 1 || flag_invalid_last == 1){
			}
			else if (flag_logo_last == 0 && flag_logo_cur == 1){		// ロゴ立ち上がり
				if (frm_last <= frm_logo){
					nsc_lrise = nsc_last;
				}
				else{
					if (nsc_rrise < 0){
						nsc_rrise = nsc_last;
					}
				}
			}
			else if (flag_logo_last == 1 && flag_logo_cur == 0){		// ロゴ立ち下がり
				if (frm_last <= frm_logo){
					nsc_lfall = nsc_last;
				}
				else{
					if (nsc_rfall < 0){
						nsc_rfall = nsc_last;
					}
				}
			}
			nsc_last = i;
			frm_last = frm_cur;
			flag_logo_last = flag_logo_cur;
			flag_invalid_last = flag_invalid_cur;
		}
	}
	//--- 一番近いロゴ切り替わり位置取得 ---
	if (nedge == 0){		// ロゴ開始部分
		if (nsc_lrise < 0){
			nsc_baselogo = nsc_rrise;
		}
		else if (nsc_rrise < 0){
			nsc_baselogo = nsc_lrise;
		}
		else if (abs(plogo->frm_scpos_s[nsc_lrise] - frm_logo) <=
				 abs(plogo->frm_scpos_s[nsc_rrise] - frm_logo)){
			nsc_baselogo = nsc_lrise;
		}
		else{
			nsc_baselogo = nsc_rrise;
		}
		if (nsc_baselogo >= 0){
			frm_baselogo = plogo->frm_scpos_s[nsc_baselogo];
			if (abs(frm_logo - frm_baselogo) >= frm_mgn){
				nsc_baselogo = -1;
			}
		}
	}
	else{					// ロゴ終了部分
		if (nsc_lfall < 0){
			nsc_baselogo = nsc_rfall;
		}
		else if (nsc_rfall < 0){
			nsc_baselogo = nsc_lfall;
		}
		else if (abs(plogo->frm_scpos_e[nsc_lfall] - frm_logo) <=
				 abs(plogo->frm_scpos_e[nsc_rfall] - frm_logo)){
			nsc_baselogo = nsc_lfall;
		}
		else{
			nsc_baselogo = nsc_rfall;
		}
		if (nsc_baselogo >= 0){
			frm_baselogo = plogo->frm_scpos_e[nsc_baselogo];
			if (abs(frm_logo - frm_baselogo) >= frm_mgn){
				nsc_baselogo = -1;
			}
		}
	}
	return nsc_baselogo;
}


//---------------------------------------------------------------------
// 指定位置がロゴあり構成か判断
// 出力：
//  返り値  : 0:ロゴなし  1:ロゴあり
//---------------------------------------------------------------------
int auto_is_logoarea(
	LOGO_RESULTREC *plogo,
	int nsc_target
){
	int i;
	int nlg;
	int inlogo;
	int frm_target, frm_last;
	int arstat_target;
	int nsc_last;
	int frm_rise, frm_fall;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	inlogo = 0;
	frm_target = plogo->frm_scpos_s[nsc_target];
	arstat_target = plogo->arstat_sc_e[nsc_target];
	//--- 構成情報だけではロゴ有無を判断できない場合 ---
	if ((arstat_target == D_SCINT_L_TRKEEP) ||
		(arstat_target == D_SCINT_L_TRRAW) ||
		(arstat_target == D_SCINT_L_EC) ||
		(arstat_target == D_SCINT_L_SP)){
		// 1つ前の構成位置を取得
		nsc_last = -1;
		i = nsc_target - 1;
		while(nsc_last < 0 && i > 0){
			if (plogo->stat_scpos[i] == 2){
				nsc_last = i;
			}
			i --;
		}
		// 構成取得時
		if (nsc_last > 0){
			frm_last = plogo->frm_scpos_s[nsc_last];
			nlg = 0;
			frm_rise = 0;
			while(nlg < plogo->num_find && frm_rise < frm_target - frm_spc){
				frm_rise = plogo->frm_rise[nlg];
				frm_fall = plogo->frm_fall[nlg];
				// ロゴが構成内に存在するか確認
				if (frm_rise < frm_target - frm_spc &&
					frm_fall > frm_last + frm_spc){
					inlogo = 1;
				}
				nlg ++;
			}
		}
	}
	//--- 構成情報がロゴ内の場合 ---
	else if (arstat_target >= D_SCINTR_L_LOW &&
			 arstat_target <= D_SCINTR_L_HIGH &&
			 arstat_target != D_SCINT_L_LGCUT){
		inlogo = 1;
	}
	return inlogo;
}


//---------------------------------------------------------------------
// 最初のロゴかつCM候補がとりにくい場合、別の候補がないか判断
// 出力：
//  返り値  : 変更後のロゴ立ち上がりフレーム
//  plogo->frm_scpos_s[] : 先頭立ち上がり部分の候補状態を変更
//  plogo->arstat_sc_e[] : 先頭立ち上がり部分の配置状態を変更
//---------------------------------------------------------------------
int auto_arrange_logo_rise1rev(
	LOGO_RESULTREC *plogo,
	int frm_logo_rise,				// ロゴ開始フレーム
	int frm_logo_fall,				// ロゴ終了フレーム
	int frm_lim						// 検索範囲フレーム
){
	int j1, j2;
	int nsc_1st;
	int val_1st;
	int frm_j1, frm_j2;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int logofix, nsc_logo;
	int frmdif_logo, frmdif_tmp;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	// ロゴ位置が確定候補か確認
	logofix = 0;
	if (auto_getnsc(plogo, frm_logo_rise, frm_spc) >= 0){
		logofix = 1;
	}

	nsc_1st = -1;
	val_1st = 0;
	nsc_logo = -1;
	frmdif_logo = 0;
	j1 = 1;
	frm_j1 = plogo->frm_scpos_s[j1];
	while((frm_j1 < frm_lim) && (frm_j1 < frm_logo_fall - frm_spc) && (j1 < plogo->num_scpos)){
		if (plogo->stat_scpos[j1] >= 0 && plogo->still_scpos[j1] == 0){
			// 最初の候補がロゴ開始位置であれば最小限の候補に残す
			if (j1 == plogo->nchk_scpos_1st && abs(frm_j1 - frm_logo_rise) <= 2 && val_1st < 1){
				nsc_1st = j1;
				val_1st = 1;
				nsc_logo = j1;
			}
			// 次の位置と間隔チェック
			j2 = j1 + 1;
			frm_j2 = plogo->frm_scpos_s[j2];
			while(frm_j2 < frm_lim * 2 && j2 < plogo->num_scpos){
				if (plogo->stat_scpos[j2] >= 0 && plogo->still_scpos[j2] == 0){
					flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
								frm_j1, frm_j2);
					if (flag1 > 0 && ncal_sec1 <= 30){
						// ロゴ開始位置から構成作成できるシーンチェンジ間がある場合
						if (abs(frm_j1 - frm_logo_rise) <= 2 &&
							(val_1st < 4 || plogo->stat_scpos[j1] >= 2)){
							nsc_1st = j1;
							val_1st = 4;
							nsc_logo = j1;
						}
						else if (abs(frm_j2 - frm_logo_rise) <= 2 &&
								 (val_1st < 4 || plogo->stat_scpos[j2] >= 2)){
							nsc_1st = j2;
							val_1st = 4;
							nsc_logo = j1;
						}
						// 最初の候補から構成作成できるシーンチェンジ間がある場合
						else if (j1 == plogo->nchk_scpos_1st && val_1st < 3){
							nsc_1st = j1;
							val_1st = 3;
							nsc_logo = j1;
						}
						// 構成作成できるシーンチェンジ間がある場合
						else if (val_1st < 2){
							nsc_1st = j1;
							val_1st = 2;
						}
						// ロゴ位置に一番近いか確認
						frmdif_tmp = abs(frm_j1 - frm_logo_rise);
						if (frmdif_tmp < abs(frm_j2 - frm_logo_rise)){
							if (frmdif_tmp < frmdif_logo || nsc_logo < 0){
								nsc_logo = j1;
								frmdif_logo = frmdif_tmp;
							}
						}
					}
				}
				j2 ++;
				frm_j2 = plogo->frm_scpos_s[j2];
			}
		}
		j1 ++;
		frm_j1 = plogo->frm_scpos_s[j1];
	}
	// 候補位置を差し替え
	if (val_1st > 0){		// 候補のランクで選択可
		if (plogo->stat_scpos[nsc_1st] <= 1){
			plogo->stat_scpos[nsc_1st] = 2;
			plogo->arstat_sc_e[nsc_1st] = D_SCINT_N_OTHER;
		}
		if (logofix == 0 && nsc_1st == nsc_logo){		// ロゴ位置変更
			frm_logo_rise = plogo->frm_scpos_s[nsc_1st];
		}
		// 既存の最初候補がある場合
		if (plogo->nchk_scpos_1st > 0){
			if (plogo->nchk_scpos_1st == nsc_1st){
				if (val_1st > 1){			// 確定場所なら候補を消す
					plogo->nchk_scpos_1st = 0;
				}
			}
			else{							// 違う位置なら最初候補は無効化
				plogo->stat_scpos[plogo->nchk_scpos_1st] = 0;
			}
		}
	}
	return frm_logo_rise;
}


//---------------------------------------------------------------------
// 最後のロゴが最終フレームまで続く場合、手前の構成区切りを設定
// 出力：
//  返り値  : ロゴ立ち下がりフレーム
//  plogo->frm_scpos_s[] : 先頭立ち上がり部分の候補状態を変更
//  plogo->arstat_sc_e[] : 先頭立ち上がり部分の配置状態を変更
//---------------------------------------------------------------------
int auto_arrange_logo_falllast(
	LOGO_RESULTREC *plogo,
	int frm_logo_rise,				// ロゴ開始フレーム
	int frm_logo_fall,				// ロゴ終了フレーム
	int frm_lim						// 検索範囲フレーム
){
	int j1, j2;
	int frm_j1, frm_j2;
	int frm_last;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int nsc_keep, nsc_tmpstat, nsc_subkeep;
	int frm_spc;
	int val_dis, val_dislast, nsc_dislast, nsc_disup1, nsc_disup2;

	frm_spc = plogo->frm_spc;

	j1 = plogo->num_scpos - 1;
	frm_last = plogo->frm_scpos_s[j1];
	j1 --;
	frm_j1 = plogo->frm_scpos_s[j1];
	nsc_tmpstat = -1;
	nsc_dislast = -1;
	nsc_disup1  = -1;
	nsc_disup2  = -1;
	val_dislast = 0;
	while((frm_j1 > frm_last - frm_lim) && (frm_j1 > frm_logo_rise + frm_spc) && (j1 > 1)){
		nsc_keep = -1;
		if (plogo->stat_scpos[j1] >= 0 && plogo->still_scpos[j1] == 0){
			// 次の位置と間隔チェック
			j2 = j1 - 1;
			frm_j2 = plogo->frm_scpos_s[j2];
			while(frm_j2 > frm_last - (frm_lim*4) && j2 > 0 && nsc_keep < 0){
				if (plogo->stat_scpos[j2] >= 0 && plogo->still_scpos[j2] == 0){
					flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
								frm_j2, frm_j1);
					if (flag1 > 0 && ncal_sec1 <= 30){
						// 構成作成できるシーンチェンジ間がある場合
						val_dis = ncal_dis1;
						if (ncal_sec1 % 15 != 0){
							val_dis += 10;
						}
						// 最適構成だった場合
						if (nsc_dislast < 0 || val_dislast > val_dis){
							// 前回の構成を破棄
							if (nsc_dislast >= 0){
								if (nsc_disup1 >= 0){
									plogo->stat_scpos[nsc_disup1] = 0;
								}
								if (nsc_disup2 >= 0){
									plogo->stat_scpos[nsc_disup2] = 0;
									nsc_tmpstat = -1;
								}
							}
							// 構成追加
							nsc_dislast = j2;
							val_dislast = val_dis;
							nsc_disup1 = -1;
							nsc_disup2 = -1;
							if (plogo->stat_scpos[j1] < 2){
								nsc_disup1 = j1;
							}
							if (plogo->stat_scpos[j2] < 2){
								nsc_disup1 = j2;
							}
							if (plogo->stat_scpos[j1] < 2 || nsc_tmpstat == j1){
								plogo->stat_scpos[j1] = 2;
								if (ncal_sec1 % 15 == 0){
									plogo->arstat_sc_e[j1] = D_SCINT_L_UNIT;
								}
								else{
									plogo->arstat_sc_e[j1] = D_SCINT_L_OTHER;
								}
							}
							if (plogo->stat_scpos[j2] < 2){
								plogo->stat_scpos[j2] = 2;
								plogo->arstat_sc_e[j2] = D_SCINT_L_OTHER;
								nsc_tmpstat = j2;				// 一時的書き込み
							}
							nsc_keep = j2;
						}
					}
				}
				j2 --;
				frm_j2 = plogo->frm_scpos_s[j2];
			}
		}
		j1 --;
		// 
		if (nsc_keep >= 0 && j1 > nsc_keep){
			j1 = nsc_keep;
		}
		frm_j1 = plogo->frm_scpos_s[j1];
	}
	// 最後までロゴが続く場合のカット
	if ((plogo->arstat_sc_e[plogo->num_scpos - 1] == D_SCINT_L_OTHER) ||
		(plogo->arstat_sc_e[plogo->num_scpos - 1] == D_SCINT_L_UNIT ) ||
		(plogo->arstat_sc_e[plogo->num_scpos - 1] == D_SCINT_L_MIXED)){
		nsc_keep = -1;
		nsc_subkeep = -1;
		j1 = plogo->num_scpos - 2;
		frm_j1 = plogo->frm_scpos_s[j1];
		while(frm_j1 > frm_last - plogo->prmvar.frm_wcomp_last && j1 > 0 && nsc_keep < 0){
			if (plogo->stat_scpos[j1] >= 2){
				nsc_keep = j1;
			}
			if (nsc_subkeep < 0){
				nsc_subkeep = j1;
			}
			j1 --;
			frm_j1 = plogo->frm_scpos_s[j1];
		}
		if (nsc_keep >= 0){			// 構成位置がある場合
			plogo->arstat_sc_e[plogo->num_scpos - 1] = D_SCINT_N_OTHER;
		}
		else if (nsc_subkeep > 0){	// 構成位置がないが無音シーンチェンジはある場合
			while((frm_j1 > frm_last - plogo->prmvar.frm_wcomp_last - CnvTimeFrm(60,0)) &&
					j1 > 0 && nsc_keep < 0){
				if (plogo->stat_scpos[j1] >= 2){
					nsc_keep = j1;
				}
				j1 --;
				frm_j1 = plogo->frm_scpos_s[j1];
			}
			if (nsc_keep < 0){		// 近くに構成確定がなければ構成追加で最後カット
				plogo->stat_scpos[nsc_subkeep] = 2;
				plogo->arstat_sc_e[nsc_subkeep] = plogo->arstat_sc_e[plogo->num_scpos - 1];
				plogo->arstat_sc_e[plogo->num_scpos - 1] = D_SCINT_N_OTHER;
			}
		}
	}

	return frm_logo_fall;
}


//---------------------------------------------------------------------
// ２構成合わせてCMになる位置を確認
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void auto_arrange_logo_aunit(
	LOGO_RESULTREC *plogo,
	int frm_logo_lastfall, int frm_logo_rise, int frm_logo_fall, int frm_logo_nextrise,
	int nsc_rise, int nsc_fall
){
	int j;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int flag_unit;
	int ncount;
	int nsc_from, nsc_next, nsc_steady;
	int nsc_p0unit, nsc_p1unit, nsc_p2unit, nsc_p3unit;
	int nsc_cur;
	int arstat_cur, arstat_next;
	int frm_cur;
	int frm_from;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	//--- シーンチェンジを順番に候補となるか確認 ---
	ncount     = 0;			// 合併候補の取得数
	nsc_steady = 0;			// 15秒単位となる手前の確定場所記憶
	for(j=1; j<plogo->num_scpos-1; j++){
		nsc_cur = j;
		frm_cur = plogo->frm_scpos_s[j];
		arstat_cur = plogo->arstat_sc_e[j];
		//--- 対象ロゴ計算範囲内で処理 ---
		if (frm_cur > frm_logo_lastfall - frm_spc && frm_cur < frm_logo_nextrise + frm_spc){
			// 確定場所を確認
			if (plogo->stat_scpos[nsc_cur] == 2){
				flag_unit = 0;
				// 確定単位構成時
				if ((arstat_cur == D_SCINT_N_UNIT ) ||
					(arstat_cur == D_SCINT_B_UNIT )){
					if (ncount > 0){
						frm_from = plogo->frm_scpos_s[nsc_p0unit];
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_from, frm_cur);
						if (flag1 > 0 && (ncal_sec1 % 15 == 0)){
							ncount = 0;
						}
					}
					if (ncount == 0){
						nsc_steady = nsc_cur;
					}
				}
				// ロゴ外部
				if ((nsc_cur <= nsc_rise && nsc_rise >= 0) ||
					(nsc_cur >  nsc_fall && nsc_fall >= 0) ||
					(nsc_rise < 0 && nsc_fall < 0)){
					// 確定位置から順番に構成位置を記憶
					if (ncount >= 3){
						nsc_p3unit = nsc_p2unit;
					}
					if (ncount >= 2){
						nsc_p2unit = nsc_p1unit;
					}
					if (ncount >= 1){
						nsc_p1unit = nsc_p0unit;
					}
					nsc_p0unit = nsc_cur;
					if (ncount <= 3){
						ncount ++;
					}
					// 2構成または3構成の時合併15秒単位を調べる
					if (ncount >= 3){
						nsc_from = nsc_p2unit;
						frm_from = plogo->frm_scpos_s[nsc_from];
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_from, frm_cur);
						if (flag1 > 0 && (ncal_sec1 % 15 == 0)){
							flag_unit = 1;
							if (ncount > 3){
								ncount = 3;
							}
						}
					}
					if (ncount >= 4 && flag_unit == 0){
						nsc_from = nsc_p3unit;
						frm_from = plogo->frm_scpos_s[nsc_from];
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_from, frm_cur);
						if (flag1 > 0 && (ncal_sec1 % 15 == 0)){
							flag_unit = 1;
						}
					}
//printf("(A:%d,%d,%d,%d,%d)", nsc_p0unit, (int)plogo->frm_scpos_s[nsc_p0unit],ncount,flag1,arstat_cur);
					// 合併処理
					if (flag_unit > 0){
						// 手前または次の位置が15秒単位確定時のみ合併処理する
						if (nsc_steady == nsc_from ||
							(nsc_cur == nsc_rise && nsc_rise >= 0)){
							flag_unit = 2;
						}
						if (flag_unit < 2){
							nsc_next = nsc_cur + 1;
							while(plogo->stat_scpos[nsc_next] < 2 && nsc_next < plogo->num_scpos-1){
								nsc_next ++;
							}
							if (nsc_next < plogo->num_scpos-1){
								arstat_next = plogo->arstat_sc_e[nsc_next];
								if ((arstat_next == D_SCINT_N_UNIT) ||
									(arstat_next == D_SCINT_B_UNIT)){
									flag_unit = 2;
								}
							}
						}
						if (flag_unit >= 2){
							plogo->arstat_sc_e[nsc_p1unit] = D_SCINT_N_BUNIT;	// 合併
							plogo->arstat_sc_e[nsc_p0unit] = D_SCINT_N_AUNIT;	// 合併
							if (ncount >= 4){	// 3構成時
								plogo->arstat_sc_e[nsc_p2unit] = D_SCINT_N_BUNIT;	// 合併
								plogo->arstat_sc_e[nsc_p1unit] = D_SCINT_N_AUNIT;	// 合併
							}
						}
					}
				}
				// ロゴ内部
				else{
					ncount = 1;
					nsc_p0unit = nsc_cur;
					nsc_steady = nsc_cur;
				}
			}
		}
	}
}


//---------------------------------------------------------------------
// １ロゴ範囲内で変化のない無音シーンチェンジ部分を構成から外す処理
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void auto_arrange_logo_delchg(
	LOGO_RESULTREC *plogo,
	int frm_logo_lastfall, int frm_logo_rise, int frm_logo_fall, int frm_logo_nextrise,
	int nsc_rise, int nsc_fall,
	int frm_wlogo_edge,				// 多少のずれ許容するロゴ切り替わりからのフレーム数
	int frm_wlogo_sftmrg			// ロゴ切り替わりのずれを許すフレーム数
){
	int j, k;
	int nsc_cur;
	int frm_cur;
	int arstat_cur;
	int nsc_tmp1, nsc_tmp2;
	int frm_tmp1, frm_tmp2;
	int nsc_ins;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int flag_tmp2;
	int nsc_lastfall, nsc_nextrise;
	int dist_sfts, dist_sfte;
	int frm_tmps, frm_tmpe;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	//--- 変化ない所は外す処理 ---
	for(j=1; j<plogo->num_scpos-1; j++){
		nsc_cur = j;
		frm_cur = plogo->frm_scpos_s[j];
		arstat_cur = plogo->arstat_sc_e[j];
		// 対象ロゴ計算範囲内で処理
		if (frm_cur > frm_logo_lastfall + frm_spc && frm_cur < frm_logo_nextrise - frm_spc){
//printf("(C:%d,%d,%d)", frm_cur, plogo->stat_scpos[nsc_cur], plogo->still_scpos[nsc_cur]);
			// 確定場所を確認
			if (plogo->stat_scpos[nsc_cur] == 2){
				if (plogo->still_scpos[nsc_cur] > 0){	// 変化ない時
					// ロゴ切り替わり位置を除く
					if ((frm_cur <  frm_logo_rise - frm_spc) ||
						(frm_cur >  frm_logo_rise + frm_spc &&
						 frm_cur <  frm_logo_fall - frm_spc) ||
						(frm_cur >  frm_logo_fall + frm_spc)){
						// 根幹ではない場所であれば外す
						if ((arstat_cur == D_SCINT_N_OTHER) ||
							(arstat_cur == D_SCINT_N_AUNIT) ||
							(arstat_cur == D_SCINT_L_UNIT ) ||
							(arstat_cur == D_SCINT_L_OTHER)){
							plogo->stat_scpos[nsc_cur] = 0;
						}
					}
				}
			}
		}
	}
	//--- 基準ロゴに対応するシーンチェンジ番号を取得 ---
	nsc_lastfall = auto_getnsc(plogo, frm_logo_lastfall, frm_spc);
	nsc_nextrise = auto_getnsc(plogo, frm_logo_nextrise, frm_spc);
	//--- 変化ない所を外したことによる補正 ---
	nsc_tmp1 = -1;
	nsc_tmp2 = -1;
	nsc_ins  = -1;
	for(j=1; j<plogo->num_scpos-1; j++){
		nsc_cur = j;
		frm_cur = plogo->frm_scpos_s[j];
		arstat_cur = plogo->arstat_sc_e[j];
		// 対象ロゴ計算範囲内で処理
		if (frm_cur > frm_logo_lastfall + frm_spc && frm_cur < frm_logo_nextrise - frm_spc){
			// 確定場所を確認
			if (plogo->stat_scpos[nsc_cur] == 2){
				if ((nsc_cur < nsc_rise && nsc_rise >= 0) ||		// ロゴ開始より前
					(nsc_cur > nsc_fall && nsc_fall >= 0)){			// ロゴ終了より後
					// 15秒単位になっているか確認と不要部分合併
					if ((plogo->arstat_sc_e[nsc_cur] == D_SCINT_UNKNOWN) ||
						(plogo->arstat_sc_e[nsc_cur] == D_SCINT_N_OTHER)){
						if (frm_tmp1 >= 0){
							flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp1, frm_cur);
							if (flag1 > 0 && ncal_sec1 % 15 == 0){	// 15秒単位は合併
								plogo->arstat_sc_e[nsc_cur] = D_SCINT_N_UNIT;
								if (nsc_ins >= 0){					// 途中で分離されていたら合併
									for(k=nsc_tmp1+1; k<nsc_cur; k++){
										if (plogo->stat_scpos[k] == 2){
											plogo->stat_scpos[k] = 0;
										}
									}
								}
							}
						}
					}
					// 15秒単位の位置確認
					if ((plogo->arstat_sc_e[nsc_cur] != D_SCINT_UNKNOWN) &&
						(plogo->arstat_sc_e[nsc_cur] != D_SCINT_N_OTHER)){
						nsc_tmp1 = nsc_cur;
						frm_tmp1 = plogo->frm_scpos_s[nsc_tmp1];
						nsc_ins = -1;
					}
					else{										// 単位構成でなければ合併候補
						nsc_ins = nsc_cur;
					}
					// ファイル先頭位置
					if (frm_logo_lastfall <= 0 && nsc_tmp1 < 0){
						nsc_tmp1 = nsc_cur;
						frm_tmp1 = plogo->frm_scpos_s[nsc_tmp1];
						nsc_ins = -1;
					}
				}
				else if (nsc_cur == nsc_rise){					// ロゴ開始地点
					if (nsc_tmp1 >= 0 && arstat_cur != D_SCINT_N_UNIT){
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_tmp1, frm_cur);
						if (flag1 > 0 && ncal_sec1 % 15 == 0){	// 直前の15秒単位は合併
							plogo->arstat_sc_e[nsc_cur] = D_SCINT_N_UNIT;
							if (nsc_ins >= 0){					// 途中で分離されていたら合併
								for(k=nsc_tmp1+1; k<nsc_cur; k++){
									if (plogo->stat_scpos[k] == 2){
										plogo->stat_scpos[k] = 0;
									}
								}
							}
						}
					}
					nsc_tmp1 = nsc_cur;
					frm_tmp1 = frm_cur;
					nsc_ins  = -1;
				}
				else{
					//--- ロゴ付近か検出 ---
					dist_sfts = 0;
					dist_sfte = 0;
					if (abs(frm_tmp2 - frm_logo_rise) <= frm_wlogo_sftmrg && nsc_rise < 0){
						dist_sfts = 1;
					}
					if (abs(frm_logo_fall - frm_tmp2) <= frm_wlogo_sftmrg && nsc_fall < 0){
						dist_sfte = 1;
					}
					//--- 外す候補がある場合の処理 ---
					if (nsc_tmp2 >= 0){
						flag_tmp2 = 0;
						if (nsc_rise >= 0 || nsc_lastfall < 0){
							frm_tmps = frm_logo_rise;
						}
						else{
							frm_tmps = frm_logo_lastfall;
						}
						if (nsc_fall >= 0 || nsc_nextrise < 0){
							frm_tmpe = frm_logo_fall;
						}
						else{
							frm_tmpe = frm_logo_nextrise;
						}
						//--- 先頭位置と候補位置から判断 ---
						// 対象場所がロゴ切り替わりから離れていたら厳しく取る
						if (abs(frm_tmp2 - frm_tmps) > frm_wlogo_edge || nsc_rise < 0){
							flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmps, frm_tmp2);
							if (flag1 == 1){
								flag1 = 0;
							}
						}
						else{
							flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmps, frm_tmp2);
						}
						if (flag1 > 0 || dist_sfts > 0){
							flag_tmp2 = 1;
							if (nsc_rise >= 0 || nsc_lastfall >= 0 ||
								(dist_sfts > 0 && frm_tmp2 - frm_logo_lastfall > frm_wlogo_edge)){
								nsc_tmp1 = nsc_tmp2;
								frm_tmp1 = frm_tmp2;
							}
						}
						//--- 終了位置と候補位置から判断 ---
						// 対象場所がロゴ切り替わりから離れていたら厳しく取る
						if (abs(frm_tmpe - frm_tmp2) > frm_wlogo_edge || nsc_fall < 0){
							flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp2, frm_tmpe);
							if (flag1 == 1){
								flag1 = 0;
							}
						}
						else{
							flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp2, frm_tmpe);
						}
						if (flag1 > 0 || dist_sfte > 0){
							flag_tmp2 = 1;
						}
						//--- 現位置と候補位置から判断 ---
						// 対象場所がロゴ切り替わりからの候補でなければ厳しく取る
						if (flag_tmp2 == 0){
							flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp2, frm_cur);
							if (flag1 == 1){
								flag1 = 0;
							}
						}
						else{
							flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp2, frm_cur);
						}
						if (flag1 == 0){		// 外す処理
							if (flag_tmp2 == 0){	// 候補位置を外す
								// 混合情報があれば移す
								if (plogo->arstat_sc_e[nsc_tmp2] == D_SCINT_L_MIXED){
									if (plogo->arstat_sc_e[nsc_cur] == D_SCINT_L_UNIT ||
										plogo->arstat_sc_e[nsc_cur] == D_SCINT_L_OTHER){
										plogo->arstat_sc_e[nsc_cur] = D_SCINT_L_MIXED;
									}
								}
								plogo->stat_scpos[nsc_tmp2] = 0;
							}
							// 現位置を候補位置に変更
							nsc_tmp2 = nsc_cur;
							frm_tmp2 = frm_cur;
						}
						else{					// 次検索の基準位置として残す
							nsc_tmp1 = nsc_cur;
							frm_tmp1 = frm_cur;
							nsc_tmp2 = -1;
						}
					}
					else if (nsc_tmp1 >= 0){			// 基準となる確定位置がある場合
						if ((frm_tmp1 - frm_logo_rise > frm_wlogo_edge || nsc_rise < 0) &&
							(frm_logo_fall - frm_tmp1 > frm_wlogo_edge || nsc_fall < 0)){
							// 対象場所がロゴ切り替わりから離れていたら厳しく取る
							flag1 = adjust_calcdif_exact(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp1, frm_cur);
							if (flag1 == 1){
								flag1 = 0;
							}
						}
						else{
							flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
										frm_tmp1, frm_cur);
						}
						if (flag1 == 0){
							nsc_tmp2 = nsc_cur;
							frm_tmp2 = frm_cur;
						}
						else{
							nsc_tmp1 = nsc_cur;
							frm_tmp1 = frm_cur;
							nsc_tmp2 = -1;
						}
					}
					else{								// 基準位置がない場合
						nsc_tmp2 = nsc_cur;
						frm_tmp2 = frm_cur;
					}
//					printf("(A:%d,%d,%d,%d,%d,%d,%d)", frm_logo_rise, nsc_tmp2, frm_tmp2, nsc_tmp1, frm_tmp1, flag1, frm_cur);
					// ロゴ終了位置なら次の設定
					if (nsc_cur == nsc_fall){
						nsc_tmp1 = nsc_cur;
						frm_tmp1 = frm_cur;
						nsc_tmp2 = -1;
					}
				}
			}
		}
	}
}


//---------------------------------------------------------------------
// ロゴ周辺の確定２点間に秒数単位となる構成があれば追加
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void auto_arrange_logo_peri_set(
	LOGO_RESULTREC *plogo,
	int nsc_from,		// 開始シーンチェンジ番号
	int nsc_to,			// 終了シーンチェンジ番号
	int inlogo,			// 0:ロゴ外  1:ロゴ内
	int ncntr,			// 1:開始側から確認  2:終了側から確認  3:両側から確認
	int sweep			// 0:通常 1:多少のずれを許容して追加する
){
	int j;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int flag2, ncal_sgn2, ncal_sec2, ncal_dis2;
	int sec_logo_from, sec_logo_to;
	int nsc_cur;
	int frm_cur;
	int frm_from, frm_to, frm_mid, frm_base;
	int nsc_fixfrom;
	int nsc_keep, nsc_base;
	int keep_sec1, keep_dis, keep_type;
	int flag_update;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	frm_from = plogo->frm_scpos_s[nsc_from];
	frm_to   = plogo->frm_scpos_s[nsc_to];
	nsc_fixfrom = nsc_from;
	//--- 開始側からの検索終了地点 ---
	if (ncntr == 3){			// 両側から検索する時は中間地点まで
		frm_mid  = (frm_from + frm_to) / 2;
	}
	else{
		frm_mid = frm_to;
	}
	//--- 開始位置から終了位置まで検索 ---
	nsc_base = nsc_from;
	frm_base = frm_from;
	nsc_keep = -1;
	keep_sec1 = 0;
	keep_dis  = 0;
	keep_type = 0;
	for(j = nsc_from + 1; j <= nsc_to; j++){
		if (plogo->stat_scpos[j] >= -1 &&			// 候補状態
			plogo->still_scpos[j] == 0){			// 画像変化あること必須
			nsc_cur = j;
			frm_cur = plogo->frm_scpos_s[j];
			flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
						frm_base, frm_cur);
			sec_logo_to   = GetSecFromFrm(frm_to - frm_cur);
			sec_logo_from = GetSecFromFrm(frm_cur - frm_from);
			// 前位置確定（番組構成追加）
			if (nsc_keep >= 0){
				if (abs(ncal_sec1 - keep_sec1) > 2 || j == nsc_to){
					if (keep_type == 2 || j == nsc_to){
						flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
									plogo->frm_scpos_s[nsc_keep], frm_cur);
					}
					else{
						flag2 = 0;
					}
					if (keep_type == 2 && (flag1 == 0 || flag2 == 0)){	// 候補のずれがある場合は今回のずれ確認
					}
					else if (j == nsc_to && ncal_sec2 < keep_sec1 && flag2 == 0){
					}
					else{
						plogo->stat_scpos[nsc_keep] = 2;
						if (inlogo > 0){
							plogo->arstat_sc_e[nsc_keep] = D_SCINT_L_OTHER;
						}
						else{
							plogo->arstat_sc_e[nsc_keep] = D_SCINT_N_OTHER;
						}
						frm_base = plogo->frm_scpos_s[nsc_keep];
						nsc_base = nsc_keep;
						nsc_keep = -1;
						nsc_fixfrom = nsc_base;			// 確定位置更新
						// baseが変化するので取直し
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_base, frm_cur);
					}
				}
			}
			// 候補追加
			if ((ncal_sec1 > 2 || nsc_from == 0) && sec_logo_to > 2){	// 両側から離れていること前提
				if (frm_cur < frm_mid + frm_spc || nsc_keep >= 0){		// 検索終了地点まで
					if ((sec_logo_from <= 30) || flag1 > 1){			// 端に近いか特定秒数の時
						flag_update = 0;
						flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
									frm_cur, frm_to);
						// 終了側にも近く両側検索する場合は両側から確認
						if (sec_logo_to <= 30 && ncntr == 3){
							if ((keep_dis > ncal_dis1 || nsc_keep < 0) && flag2 > 0){
								flag_update = 1;
							}
						}
						// 開始側に候補ある場合
						else if ((ncntr & 0x1)!=0 && flag1 > 0){
							if (keep_dis > ncal_dis1 || nsc_keep < 0){
								flag_update = 1;
							}
						}
						// ロゴなし開始側検索で終了側から15秒単位の場合
						else if ((ncntr & 0x1)!=0 && (ncal_sec2 % 15 == 0) && (ncal_sec2 > 0) &&
								 (inlogo == 0) && (nsc_to < plogo->num_scpos-1)){
							if (keep_dis > ncal_dis1 || nsc_keep < 0){
								flag_update = 1;
							}
						}
						// 開始側に候補ある場合（ずれが多少ある場合）
						else if ((ncntr & 0x1)!=0 && nsc_keep < 0 && sweep > 0 &&
								 ncal_sec1 <= 15 && ncal_dis1 <= 10){
							flag_update = 2;
						}
						// 候補を更新
						if (flag_update > 0){
							nsc_keep  = nsc_cur;
							keep_sec1 = ncal_sec1;
							keep_dis  = ncal_dis1;
							keep_type = flag_update;
						}
					}
				}
			}
		}
	}
	//--- 終了位置から開始位置（確定箇所あれば確定位置）まで検索 ---
	nsc_base = nsc_to;
	frm_base = frm_to;
	nsc_keep = -1;
	keep_sec1 = 0;
	keep_dis  = 0;
	keep_type = 0;
	for(j = nsc_to - 1; j >= nsc_fixfrom; j--){
		if (plogo->stat_scpos[j] >= -1 &&			// 候補状態
			plogo->still_scpos[j] == 0){			// 画像変化あること必須
			nsc_cur = j;
			frm_cur = plogo->frm_scpos_s[j];
			flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
						frm_cur, frm_base);
			sec_logo_to   = GetSecFromFrm(frm_to - frm_cur);
			sec_logo_from = GetSecFromFrm(frm_cur - frm_from);
			// 前位置確定（番組構成追加）
			if (nsc_keep >= 0){
				if (abs(ncal_sec1 - keep_sec1) > 2 || j == nsc_fixfrom){
					if (keep_type == 2 || j == nsc_fixfrom){
						flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
									frm_cur, plogo->frm_scpos_s[nsc_keep]);
					}
					else{
						flag2 = 0;
					}
					if (keep_type == 2 && (flag1 == 0 || flag2 == 0)){	// 候補のずれがある場合は今回のずれ確認
					}
					else if (j == nsc_fixfrom && ncal_sec2 < keep_sec1 && flag2 == 0){	// 最終の確定候補からのずれ
					}
					else{
						plogo->stat_scpos[nsc_keep] = 2;
						if (inlogo > 0){
							plogo->arstat_sc_e[nsc_keep] = plogo->arstat_sc_e[nsc_base];
							plogo->arstat_sc_e[nsc_base] = D_SCINT_L_OTHER;
						}
						else{
							plogo->arstat_sc_e[nsc_keep] = plogo->arstat_sc_e[nsc_base];
							plogo->arstat_sc_e[nsc_base] = D_SCINT_N_OTHER;
						}
						frm_base = plogo->frm_scpos_s[nsc_keep];
						nsc_base = nsc_keep;
						nsc_keep = -1;
						// baseが変化するので取直し
						flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
									frm_cur, frm_base);
					}
				}
			}
			// 候補追加
			if ((ncal_sec1 > 2) && (sec_logo_from > 2 || nsc_from == 0)){	// 両側から離れていること前提
				if ((sec_logo_to <= 30) || flag1 > 1){						// 端に近いか特定秒数の時
					// 終了側に候補ある場合
					if (nsc_to < plogo->num_scpos-1){
						flag_update = 0;
						flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
									frm_from, frm_cur);
						// 終了側に候補ある場合
						if ((ncntr & 0x2)!=0 && flag1 > 0){
							if (keep_dis > ncal_dis1 || nsc_keep < 0){
								flag_update = 1;
							}
						}
						// ロゴなし終了側検索で開始側から15秒単位の場合
						else if ((ncntr & 0x2)!=0 && (ncal_sec2 % 15 == 0) && (ncal_sec2 > 0) &&
								 (inlogo == 0) && (nsc_fixfrom == nsc_from) && (nsc_from > 0)){
							if (keep_dis > ncal_dis1 || nsc_keep < 0){
								flag_update = 1;
							}
						}
						// 終了側に候補ある場合（ずれが多少ある場合）
						else if ((ncntr & 0x2)!=0 && nsc_keep < 0 && sweep > 0 &&
								 ncal_sec1 <= 15 && ncal_dis1 <= 10){
							flag_update = 2;
						}
						// 候補を更新
						if (flag_update > 0){
							nsc_keep  = nsc_cur;
							keep_sec1 = ncal_sec1;
							keep_dis  = ncal_dis1;
							keep_type = flag_update;
						}
					}
				}
			}
		}
	}
}


//---------------------------------------------------------------------
// ロゴ周辺であらためて構成を確認して追加
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void auto_arrange_logo_peri(
	LOGO_RESULTREC *plogo,
	int frm_logo_lastfall, int frm_logo_rise, int frm_logo_fall, int frm_logo_nextrise,
	int nsc_rise, int nsc_fall,
	int frm_wlogo_edge				// 多少のずれ許容するロゴ切り替わりからのフレーム数
){
	int j;
	int nsc_from;
	int nsc_cur;
	int frm_cur;
	int frm_from;
	int arstat_cur;
	int frm_spc;
	int sweep;

	frm_spc = plogo->frm_spc;

	//--- シーンチェンジを順番に候補となるか確認 ---
	nsc_from = 0;
	for(j=1; j<plogo->num_scpos-1; j++){
		nsc_cur = j;
		frm_cur = plogo->frm_scpos_s[j];
		arstat_cur = plogo->arstat_sc_e[j];
		//--- 対象ロゴ以前は開始場所として検索 ---
		if (frm_cur <= frm_logo_lastfall + frm_spc){
			if (plogo->stat_scpos[nsc_cur] == 2){
				nsc_from = nsc_cur;
			}
		}
		//--- 対象ロゴ計算範囲内で処理 ---
		if (frm_cur > frm_logo_lastfall + frm_spc && frm_cur < frm_logo_nextrise - frm_spc){
			// 確定場所を確認
			if (plogo->stat_scpos[nsc_cur] == 2){
				//--- ロゴ付近か確認 ---
				frm_from = plogo->frm_scpos_s[nsc_from];
				if ((abs(frm_cur  - frm_logo_rise) <= frm_wlogo_edge &&
					 abs(frm_from - frm_logo_rise) <= frm_wlogo_edge) ||
					(abs(frm_cur  - frm_logo_fall) <= frm_wlogo_edge &&
					 abs(frm_from - frm_logo_fall) <= frm_wlogo_edge)){
					sweep = 1;			// 多少のずれがあっても検索する
				}
				else{
					sweep = 0;
				}
				//--- ロゴ開始以前の処理 ---
				if (nsc_cur <= nsc_rise && nsc_rise >= 0){
					if ((arstat_cur == D_SCINT_N_UNIT ) ||		// 単位構成時は検出しない
						(arstat_cur == D_SCINT_N_AUNIT) ||
						(arstat_cur == D_SCINT_N_BUNIT) ||
						(arstat_cur == D_SCINT_B_UNIT )){
						nsc_from = nsc_cur;
					}
					if (nsc_cur == nsc_rise){					// ロゴ開始地点
						if (nsc_from < nsc_rise){
							auto_arrange_logo_peri_set(plogo, nsc_from, nsc_cur, 0, 2, 0);
						}
					}
				}
				//--- ロゴ終了以前の処理 ---
				else if (nsc_cur <= nsc_fall && nsc_fall >= 0){
					if (nsc_from == nsc_rise && nsc_cur == nsc_fall){
						auto_arrange_logo_peri_set(plogo, nsc_from, nsc_cur, 1, 3, sweep);
					}
					else if (nsc_from == nsc_rise){
						auto_arrange_logo_peri_set(plogo, nsc_from, nsc_cur, 1, 1, sweep);
					}
					else if (nsc_cur == nsc_fall){
						auto_arrange_logo_peri_set(plogo, nsc_from, nsc_cur, 1, 2, sweep);
					}
				}
				//--- ロゴ終了より後 ---
				else if (nsc_cur > nsc_fall && nsc_from == nsc_fall){
					if ((arstat_cur != D_SCINT_N_UNIT ) &&		// 単位構成時は検出しない
						(arstat_cur != D_SCINT_N_AUNIT) &&
						(arstat_cur != D_SCINT_N_BUNIT) &&
						(arstat_cur != D_SCINT_B_UNIT )){
						auto_arrange_logo_peri_set(plogo, nsc_from, nsc_cur, 0, 1, 0);
					}
				}
				nsc_from = nsc_cur;
			}
		}
	}
}


//---------------------------------------------------------------------
// ロゴ切り替わり位置がずれている場合の位置補正
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->frm_rise[]       : ロゴ開始位置を変更
//  plogo->frm_fall[]       : ロゴ終了位置を変更
//---------------------------------------------------------------------
void auto_arrange_logo_shift(
	LOGO_RESULTREC *plogo,
	int frm_logo_rise, int frm_logo_fall,
	int nsc_rise, int nsc_fall,
	int nlg,
	int frm_wlogo_sftmrg				// ロゴ切り替わりのずれを許すフレーム数
){
	int j, k;
	int arstat_cur;
	int frm_cur;
	int frm_sec15s, frm_sec15x;
	int frm_dif, frm_dst, frm_nearest;
	int nsc_dst, nsc_ins;
	int frm_wlogo_ovmrg;

	frm_wlogo_ovmrg = CnvTimeFrm(2, 800);

	//--- ロゴ立ち上がり位置の補正 ---
	j = nsc_rise;
	if (j > 1){
		arstat_cur = plogo->arstat_sc_e[j];
		if (arstat_cur == D_SCINT_B_OTHER){		// ロゴ地点が無音シーンチェンジでなかった場合
			j --;
			while(plogo->stat_scpos[j] < 2 && j > 0){
				j --;
			}
			if (j > 0){							// 隣接構成位置
				frm_cur = plogo->frm_scpos_s[j];
				frm_dif = abs(frm_logo_rise - frm_cur);
				// 従来のロゴ位置から範囲内であればロゴ位置を変更
				if (frm_dif <= frm_wlogo_sftmrg){
					// ロゴ位置を変更
					plogo->frm_rise[nlg] = frm_cur;
					// 旧ロゴ位置を消去
					plogo->stat_scpos[nsc_rise] = 0;
					plogo->arstat_sc_e[nsc_rise] = D_SCINT_UNKNOWN;
				}
				else{
					// ロゴがCMに少しだけ重なっている場合の無効化
					frm_sec15s = CnvTimeFrm(15, 0);
					frm_sec15x = ((frm_dif + frm_sec15s - 1) / frm_sec15s) * frm_sec15s;
					if (frm_sec15x - frm_dif < frm_wlogo_ovmrg){
						frm_dst = frm_cur + frm_sec15x;
						frm_nearest = CnvTimeFrm(0, 500);
						nsc_dst = -1;
						nsc_ins = -1;
						for(k=nsc_rise; k < plogo->num_scpos-1; k++){
							if (plogo->frm_smute_s[k] <= frm_dst &&
								plogo->frm_smute_e[k] >= frm_dst){
								if (nsc_dst <= 0){
									nsc_dst = k;
								}
							}
							if (abs(plogo->frm_scpos_s[k] - frm_dst) < frm_nearest){
								nsc_dst = k;
								nsc_ins = k;
								frm_nearest = abs(plogo->frm_scpos_s[k] - frm_dst);
							}
						}
						if (nsc_dst > 0){
							if (nsc_ins <= 0){
								// 無音シーンチェンジ挿入
								nsc_ins = insert_scpos(plogo, frm_dst, frm_dst-1,
										plogo->frm_smute_s[nsc_dst], plogo->frm_smute_e[nsc_dst], 2, 0);
							}
							if (nsc_ins > 0){
								// 旧ロゴ位置を消去
								plogo->stat_scpos[nsc_rise] = 0;
								plogo->arstat_sc_e[nsc_rise] = D_SCINT_UNKNOWN;
								// 新しいロゴ位置設定
								plogo->stat_scpos[nsc_ins] = 2;
								plogo->arstat_sc_e[nsc_ins] = D_SCINT_N_UNIT;
								// 元位置がロゴ範囲内だった場合ロゴ位置を変更
								if (plogo->frm_smute_s[nsc_dst] <= frm_logo_rise &&
									plogo->frm_smute_e[nsc_dst] >= frm_logo_rise){
									plogo->frm_rise[nlg] = plogo->frm_scpos_s[j];
								}
							}
						}
					}
				}
			}
		}
	}

	//--- ロゴ立ち下がり位置の補正 ---
	j = nsc_fall;
	if (j > 1 && j < plogo->num_scpos-2){
		j ++;
		while(plogo->stat_scpos[j] < 2 && j < plogo->num_scpos-1){
			j ++;
		}
		if (j < plogo->num_scpos-1){
			arstat_cur = plogo->arstat_sc_e[j];
			if (arstat_cur == D_SCINT_B_OTHER){		// ロゴ地点が無音シーンチェンジでなかった場合
				frm_cur = plogo->frm_scpos_s[j];
				frm_dif = abs(frm_cur - frm_logo_fall);
				// 従来のロゴ位置から範囲内であればロゴ位置を変更
				if (frm_dif <= frm_wlogo_sftmrg){
					// ロゴ位置を変更
					plogo->frm_fall[nlg] = plogo->frm_scpos_e[j];
					// ロゴ位置の配置情報を変更
					plogo->arstat_sc_e[j] = D_SCINT_L_OTHER;
					// 旧ロゴ位置を消去
					plogo->stat_scpos[nsc_fall] = 0;
					plogo->arstat_sc_e[nsc_fall] = D_SCINT_UNKNOWN;
				}
				else{
					// ロゴがCMに少しだけ重なっている場合の無効化
					frm_sec15s = CnvTimeFrm(15, 0);
					frm_sec15x = ((frm_dif + frm_sec15s - 1) / frm_sec15s) * frm_sec15s;
					if (frm_sec15x - frm_dif < frm_wlogo_ovmrg && frm_cur > frm_sec15x){
						frm_dst = frm_cur - frm_sec15x;
						frm_nearest = CnvTimeFrm(0, 500);
						nsc_dst = -1;
						nsc_ins = -1;
						for(k=nsc_fall; k > 0; k--){
							if (plogo->frm_smute_s[k] <= frm_dst &&
								plogo->frm_smute_e[k] >= frm_dst){
								if (nsc_dst <= 0){
									nsc_dst = k;
								}
							}
							if (abs(plogo->frm_scpos_s[k] - frm_dst) < frm_nearest){
								nsc_dst = k;
								nsc_ins = k;
								frm_nearest = abs(plogo->frm_scpos_s[k] - frm_dst);
							}
						}
						if (nsc_dst > 0){
							if (nsc_ins <= 0){
								// 無音シーンチェンジ挿入
								nsc_ins = insert_scpos(plogo, frm_dst, frm_dst-1,
										plogo->frm_smute_s[nsc_dst], plogo->frm_smute_e[nsc_dst], 2, 0);
							}
							if (nsc_ins > 0){
								// 旧ロゴ位置を消去
								plogo->stat_scpos[nsc_fall] = 0;
								plogo->arstat_sc_e[nsc_fall] = D_SCINT_UNKNOWN;
								// 新しいロゴ位置設定
								plogo->stat_scpos[nsc_ins] = 2;
								plogo->arstat_sc_e[nsc_ins] = D_SCINT_L_OTHER;
								plogo->arstat_sc_e[j] = D_SCINT_N_UNIT;
								// 元位置がロゴ範囲内だった場合ロゴ位置を変更
								if (plogo->frm_smute_s[nsc_dst] <= frm_logo_fall &&
									plogo->frm_smute_e[nsc_dst] >= frm_logo_fall){
									plogo->frm_fall[nlg] = plogo->frm_scpos_e[j];
								}
							}
						}
					}
				}
			}
		}
	}
}


//---------------------------------------------------------------------
// 最初のロゴ開始前の最初の構成区切り位置をチェック
// 出力：
//  返り値  : 0:変更なし 1:変更あり
//  plogo->frm_scpos_s[] : 先頭部分の候補状態を変更
//  plogo->arstat_sc_e[] : 先頭部分の配置状態を変更
//---------------------------------------------------------------------
int auto_arrange_logo_scpos1rev(
	LOGO_RESULTREC *plogo,
	int frm_logo_rise				// ロゴ開始フレーム
){
	int ret;
	int j;
	int frm_pos1st;
	int frm_cur;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	// 候補がない場合は終了
	ret = 1;
	if (plogo->nchk_scpos_1st <= 0){
		ret = 0;
	}
	// 候補取得
	if (ret > 0){
		frm_pos1st = plogo->frm_scpos_s[plogo->nchk_scpos_1st];
		// 候補がロゴ開始以降の時は終了
		if (frm_pos1st >= frm_logo_rise){
			ret = 0;
		}
	}
//printf("(%d,%d,%d)", (int)plogo->nchk_scpos_1st, frm_pos1st, frm_logo_rise);
	// 確定位置を確認
	if (ret > 0){
		j = 1;
		while(j < plogo->num_scpos-1){
			if (plogo->stat_scpos[j] == 2 && j != plogo->nchk_scpos_1st){
				frm_cur = plogo->frm_scpos_s[j];
				if (frm_cur < frm_logo_rise - frm_spc){				// ロゴ立ち上がりより前
					if (abs(frm_pos1st - frm_cur) < CnvTimeFrm(0, 800)){	// 位置が重なり
						plogo->stat_scpos[plogo->nchk_scpos_1st] = 0;
						plogo->arstat_sc_e[plogo->nchk_scpos_1st] = D_SCINT_UNKNOWN;
					}
				}
			}
			j ++;
		}
	}

	return ret;
}


//---------------------------------------------------------------------
// ロゴ切り替わり付近のシーンチェンジを再認識
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->frm_rise[]       : ロゴ開始位置を変更（auto_arrange_logo_shiftのみ）
//  plogo->frm_fall[]       : ロゴ終了位置を変更（auto_arrange_logo_shiftのみ）
//---------------------------------------------------------------------
void auto_arrange_logo(LOGO_RESULTREC *plogo){
	int frm_logo_rise, frm_logo_fall;
	int frm_logo_lastfall, frm_logo_nextrise;
	int nsc_rise, nsc_fall;
	int nlg;
	int frm_spc, frm_lim, frm_wlogo_edge, frm_wlogo_sftmrg;

	frm_spc = plogo->frm_spc;
	frm_lim = CnvTimeFrm(30, 0) + frm_spc;			// 検索対象内とするフレーム
	frm_wlogo_edge = CnvTimeFrm(120, 0) + frm_spc;	// ロゴ切り替わり付近検出用
	frm_wlogo_sftmrg = plogo->prmvar.frm_wlogo_sftmrg;	// ロゴ位置ずれを許容するフレーム数

	for(nlg = 0; nlg < plogo->num_find; nlg++){
		//--- ロゴのフレーム位置取得 ---
		frm_logo_rise = plogo->frm_rise[nlg];
		frm_logo_fall = plogo->frm_fall[nlg]+1;
		if (nlg == 0){
			frm_logo_lastfall = -frm_spc;
		}
		else{
			frm_logo_lastfall = plogo->frm_fall[nlg-1]+1;
		}
		if (nlg == plogo->num_find - 1){
			frm_logo_nextrise = plogo->frm_totalmax;
		}
		else{
			frm_logo_nextrise = plogo->frm_rise[nlg+1];
		}

		//--- 最初のロゴかつCM候補がとりにくい場合、別の候補がないか判断 ---
		if (nlg == 0 && frm_logo_rise < frm_lim){
			frm_logo_rise = auto_arrange_logo_rise1rev(plogo, frm_logo_rise, frm_logo_fall, frm_lim);
		}

		//--- 最後のロゴが最終フレームまでかかっている時の判断 ---
		if (nlg == plogo->num_find - 1 && frm_logo_fall > plogo->frm_totalmax - frm_lim){
			auto_arrange_logo_falllast(plogo, frm_logo_rise, frm_logo_fall, frm_lim);
		}

		//--- 基準ロゴに対応するシーンチェンジ番号を取得 ---
		nsc_rise = auto_getnsc(plogo, frm_logo_rise, frm_spc);
		nsc_fall = auto_getnsc(plogo, frm_logo_fall, frm_spc);

//printf("(%d,%d,%d)", nlg, (int)plogo->frm_scpos_s[nsc_fall], (int)plogo->arstat_sc_e[nsc_fall]);
		//--- ２構成合わせてCMになる位置を確認 ---
		auto_arrange_logo_aunit(plogo,
			frm_logo_lastfall, frm_logo_rise, frm_logo_fall, frm_logo_nextrise, nsc_rise, nsc_fall);

		//--- 対象ロゴ範囲内で変化のない無音シーンチェンジ部分を構成から外す処理 ---
		auto_arrange_logo_delchg(plogo,
			frm_logo_lastfall, frm_logo_rise, frm_logo_fall, frm_logo_nextrise, nsc_rise, nsc_fall,
			frm_wlogo_edge, frm_wlogo_sftmrg);

		//--- 対象ロゴ周辺であらためて構成追加 ---
		auto_arrange_logo_peri(plogo,
			frm_logo_lastfall, frm_logo_rise, frm_logo_fall, frm_logo_nextrise, nsc_rise, nsc_fall,
			frm_wlogo_edge);

		//--- ロゴ切り替わり位置がずれている場合の位置補正 ---
		auto_arrange_logo_shift(plogo,
			frm_logo_rise, frm_logo_fall,
			nsc_rise, nsc_fall,
			nlg,
			frm_wlogo_sftmrg);

		//--- 最初のロゴの時に手前の確定位置がないか判断 ---
		if (nlg == 0){
			auto_arrange_logo_scpos1rev(plogo, frm_logo_rise);
		}
	}
}

//---------------------------------------------------------------------
// 配置状態を最初に検出・変更
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->*_scpos*         : シーンチェンジ挿入あり
//  plogo->frm_rise[]       : ロゴ開始位置を変更（auto_arrange_logo_shiftのみ）
//  plogo->frm_fall[]       : ロゴ終了位置を変更（auto_arrange_logo_shiftのみ）
//---------------------------------------------------------------------
int auto_arrange(LOGO_RESULTREC *plogo){
	int i;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int flag2, ncal_sgn2, ncal_sec2, ncal_dis2;
	int flag3, ncal_sgn3, ncal_sec3, ncal_dis3;
	int flag_sc;
	int frm_logo_lastfall, frm_logo_rise, frm_logo_fall, frm_mid;
	int frm_nearest, frm_tmp;
	int flag_dislogo;
	int nsc_last, nsc_cur, nsc_ins;
	int frm_last, frm_cur;
	int frm_spc;
	int nlg = 0;
	int stat_inlogo = 0;
	int stat_stdlast = 0;
	int frm_stdlast = 0;

	frm_spc = CnvTimeFrm(1, 100);			// 1.1秒分のフレームをマージン
	plogo->frm_spc = frm_spc;

	nsc_last = -1;
	frm_last = 0;
	frm_nearest = -1000;
	frm_logo_lastfall = 0;
	frm_logo_rise = plogo->frm_rise[nlg];
	frm_logo_fall = plogo->frm_fall[nlg]+1;
	flag_dislogo = (frm_logo_fall - frm_logo_rise < CnvTimeFrm(1, 500))? 1 : 0;
	// 初期化
	for(i=0; i<plogo->num_scpos; i++){
		plogo->arstat_sc_e[i] = D_SCINT_UNKNOWN;
	}
	//--- 各シーンチェンジ位置確認 ---
	i = 0;
	while(++i < plogo->num_scpos){
		frm_cur = plogo->frm_scpos_s[i];
		nsc_cur = i;
		// 構成切り替わり強制挿入時の無音シーンチェンジフレーム位置候補
		if (stat_inlogo == 1){				// 前回ロゴ内
			frm_tmp = frm_logo_fall;		// 基準を次のロゴ終了
		}
		else if (stat_inlogo == 2){			// 前回ロゴ内でロゴ位置シフト後
			frm_tmp = frm_logo_lastfall;	// 基準を次のロゴ終了
		}
		else{								// 前回ロゴ外
			frm_tmp = frm_logo_rise;		// 基準を次のロゴ開始
		}
		if (abs(frm_nearest - frm_tmp) > abs(frm_cur - frm_tmp)){
			frm_nearest = frm_cur;			// 一番基準に近い無音シーンチェンジを設定
		}
		//--- CM認識中のロゴ開始時にシーンチェンジ区切りがなかった場合 ---
		if (stat_stdlast == 2 && frm_cur > frm_logo_rise + frm_spc && flag_dislogo == 0){
			frm_mid = frm_logo_rise;
			flag_sc = 0;
			if (abs(frm_nearest - frm_mid) <= frm_spc){	// 近くに無音区間があったらそこに設定
				frm_mid = frm_nearest;
				flag_sc = 1;
			}
			flag2 = adjust_calcdif(&ncal_sgn2, &ncal_sec2, &ncal_dis2, frm_last, frm_mid);
			flag3 = adjust_calcdif(&ncal_sgn3, &ncal_sec3, &ncal_dis3, frm_stdlast, frm_mid);
			// 無音シーンチェンジ挿入、現在位置ずれたら修正
			nsc_ins = insert_scpos(plogo, frm_mid, frm_mid-1, frm_mid, frm_mid, 2, 0);
			if (nsc_cur >= nsc_ins && plogo->frm_scpos_s[nsc_cur+1] == frm_cur){
				nsc_cur = nsc_cur + 1;
				i = i + 1;
			}
			// 配置状態設定
			if ((ncal_sec2 % 15 != 0 || flag2 == 0) && ncal_sec3 % 15 == 0 && flag3 > 0){
				if (flag_sc > 0){
					plogo->arstat_sc_e[nsc_ins]     = D_SCINT_N_UNIT;
				}
				else{
					plogo->arstat_sc_e[nsc_ins]     = D_SCINT_B_UNIT;
				}
				plogo->stat_scpos[nsc_last]  = 0;
			}
			else if (ncal_sec2 % 15 == 0 && flag2 > 0){
				if (flag_sc > 0){
					plogo->arstat_sc_e[nsc_ins]  = D_SCINT_N_UNIT;
				}
				else{
					plogo->arstat_sc_e[nsc_ins]  = D_SCINT_B_UNIT;
				}
			}
			else{
				if (flag_sc > 0){
					plogo->arstat_sc_e[nsc_ins]  = D_SCINT_N_OTHER;
				}
				else{
					plogo->arstat_sc_e[nsc_ins]  = D_SCINT_B_OTHER;
				}
			}
			nsc_last = nsc_ins;
			frm_last = frm_mid;
			stat_stdlast = 0;
		}
		//--- ロゴ位置を確認、次のロゴ位置に移動 ---
		while(frm_cur > plogo->frm_fall[nlg]+1 + frm_spc && nlg < plogo->num_find-1){
			nlg ++;
			frm_logo_lastfall = frm_logo_fall;
			frm_logo_rise = plogo->frm_rise[nlg];
			frm_logo_fall = plogo->frm_fall[nlg]+1;
			flag_dislogo = (frm_logo_fall - frm_logo_rise < CnvTimeFrm(1, 500))? 1 : 0;
			if (stat_inlogo == 1){			// 前位置がロゴ内部、今回ロゴ外でロゴ位置シフトする場合
				stat_inlogo = 2;
			}
			else if (stat_inlogo == 2){		// ロゴ位置シフトからさらに次のロゴに移ったら内部保持中止
				stat_inlogo = 0;
			}
		}
		//--- 前回ロゴ消えからCM認識なしで次のロゴになったら内部保持中止 ---
		if (stat_inlogo == 2 && frm_cur > frm_logo_rise + frm_spc){
			stat_inlogo = 0;
		}

		//--- 認識箇所の状態を再認識 ---
		if (plogo->nchk_scpos_1st == nsc_cur && stat_inlogo == 0){
			plogo->stat_scpos[nsc_cur] = 2;
		}
		//--- 認識箇所の状態を再認識 ---
		if (plogo->stat_scpos[nsc_cur] == 2){
			flag1 = adjust_calcdif(&ncal_sgn1, &ncal_sec1, &ncal_dis1, frm_last, frm_cur);

			//--- 前回ロゴ内部、今回ロゴ外部の場合、分離 ---
			if (stat_inlogo > 0){
				if (frm_cur <= frm_logo_rise + frm_spc || frm_cur > frm_logo_fall + frm_spc){	// 今回ロゴ外
					if (stat_inlogo == 1){			// ロゴ位置シフトがない時
						frm_mid = frm_logo_fall;
					}
					else{							// ロゴ位置シフト時
						frm_mid = frm_logo_lastfall;
					}
					flag_sc = 0;
					if (abs(frm_nearest - frm_mid) <= frm_spc){	// 近くに無音区間があったらそこに設定
						frm_mid = frm_nearest;
						flag_sc = 1;
					}
					flag2 = adjust_calcdif(&ncal_sgn2, &ncal_sec2, &ncal_dis2, frm_last, frm_mid);
					// 無音シーンチェンジ挿入、現在位置ずれたら修正
					nsc_ins = insert_scpos(plogo, frm_mid, frm_mid-1, frm_mid, frm_mid, 2, 0);
					if (nsc_cur >= nsc_ins && plogo->frm_scpos_s[nsc_cur+1] == frm_cur){
						nsc_cur = nsc_cur + 1;
						i = i + 1;
					}
					// 配置状態設定
					if (ncal_sec2 % 15 == 0 && flag2 > 0){
						plogo->arstat_sc_e[nsc_ins]  = D_SCINT_L_UNIT;
					}
					else{
						plogo->arstat_sc_e[nsc_ins]  = D_SCINT_L_OTHER;
					}
					nsc_last = nsc_ins;
					frm_last = frm_mid;
					stat_inlogo = 3;			// 設定完了
					// 変更後設定で再取得
					flag1 = adjust_calcdif(&ncal_sgn1, &ncal_sec1, &ncal_dis1, frm_last, frm_cur);
				}
			}

			//--- ロゴ内の処理 ---
			if (frm_last >= frm_logo_rise - frm_spc && frm_cur <= frm_logo_fall + frm_spc){
				if (nsc_last < 0){
					plogo->arstat_sc_e[nsc_cur]  = D_SCINT_L_OTHER;
				}
				else if (flag1 > 0 && (ncal_sec1 % 15 == 0)){
					plogo->arstat_sc_e[nsc_cur]  = D_SCINT_L_UNIT;
				}
				else{
					plogo->arstat_sc_e[nsc_cur]  = D_SCINT_L_OTHER;
				}
			}
			//--- ロゴ外の処理 ---
			else if ((frm_last >= frm_logo_lastfall - frm_spc && frm_cur <= frm_logo_rise + frm_spc) ||
						 frm_last >= frm_logo_fall - frm_spc ||
						 stat_inlogo == 3){
				if (nsc_last < 0){
					plogo->arstat_sc_e[nsc_cur]  = D_SCINT_N_OTHER;
					// CM認識設定
					stat_stdlast = 2;
					frm_stdlast  = frm_cur;
				}
				else if (flag1 > 0 && (ncal_sec1 % 15 == 0)){
					if (stat_inlogo > 0 &&
						(stat_inlogo != 3 || flag_sc == 0)){
						plogo->arstat_sc_e[nsc_cur]  = D_SCINT_B_UNIT;
					}
					else{
						plogo->arstat_sc_e[nsc_cur]  = D_SCINT_N_UNIT;
						// CM認識設定
						stat_stdlast = 2;
						frm_stdlast  = frm_cur;
					}
				}
				else{
					if (stat_inlogo > 0 &&
						(stat_inlogo != 3 || flag_sc == 0)){
						plogo->arstat_sc_e[nsc_cur]  = D_SCINT_B_OTHER;
					}
					else{
						plogo->arstat_sc_e[nsc_cur]  = D_SCINT_N_OTHER;
					}
				}
				stat_inlogo = 0;
			}
			//--- ロゴ有無混合時の処理 ---
			else{
				plogo->arstat_sc_e[nsc_cur]  = D_SCINT_L_MIXED;
			}
			//--- ロゴ開始以降で未終了の場合、次はロゴ内状態として設定 ---
			if (frm_cur >= frm_logo_rise - frm_spc && frm_cur < frm_logo_fall - frm_spc){
				stat_inlogo = 1;
			}
			else{
				stat_inlogo = 0;
			}
			//--- ロゴ開始場所以降であればCM検出認定状態は解除 ---
			if (frm_cur >= frm_logo_rise - frm_spc){
				stat_stdlast = 0;
			}
			//--- 現在の位置を前回位置として記憶 ---
			nsc_last = nsc_cur;
			frm_last = frm_cur;
		}
	}

	//--- ロゴ切り替わり付近のシーンチェンジを再認識 ---
	auto_arrange_logo(plogo);

//	for(i=0; i<plogo->num_scpos; i++){
//		printf("(%d:%d,%d,%d)", i, (int)plogo->frm_scpos_s[i], plogo->stat_scpos[i], plogo->arstat_sc_e[i]);
//	}

	//--- 予告検出なしの検出（初期値設定） ---
	plogo->frm_notfound_tr = -1;

	return 0;
}

//---------------------------------------------------------------------
// 合併15秒単位を検索
// 出力：
//  返り値：合併15秒単位となる最後のシーンチェンジ番号（なければ現在のシーンチェンジ番号）
//---------------------------------------------------------------------
int autocm_detect_unit(LOGO_RESULTREC *plogo, int nsc_cur, int nsc_from){
	int i, n;
	int nsc_unit;
	int frm_from, frm_cur;
	int secdif_cur;
	int flag_end;

	if (nsc_from < 0 || nsc_from >= plogo->num_scpos){
		return nsc_cur;
	}

	flag_end = 0;
	frm_from = plogo->frm_scpos_s[nsc_from];
	nsc_unit = -1;
	n = 0;
	i = nsc_cur;
	while(i < plogo->num_scpos-1 && flag_end == 0){
		if (plogo->stat_scpos[i] == 2){
			frm_cur = plogo->frm_scpos_s[i];
			secdif_cur = GetSecFromFrm(frm_cur - frm_from);
			if (secdif_cur == 15 || secdif_cur == 30){
				nsc_unit = i;				// 合併15秒単位を設定
				flag_end = 1;
			}
			else if (secdif_cur > 30){
				flag_end = 1;
			}
			n ++;
			if (n >= 3){					// 最大３構成まで
				flag_end = 1;
			}
		}
		i ++;
	}
//printf("@%d,%d,%d",frm_from, n, secdif_cur);

	return nsc_unit;
}


//---------------------------------------------------------------------
// 指定期間をCM扱いに変更（nsc_fromの次からnsc_toまで）
// 出力：
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void autocm_detect_chgcm(LOGO_RESULTREC *plogo, int nsc_from, int nsc_to){
	int i;
	int arstat;
	int nsc_last;
	int nsc_unitrest;

	nsc_last = nsc_from;
	nsc_unitrest = 0;
	for(i=nsc_from+1; i<=nsc_to; i++){
		if (plogo->stat_scpos[i] == 2){
			//--- CM構成化 ---
			arstat = plogo->arstat_sc_e[i];
			if (arstat == D_SCINT_L_UNIT){
				plogo->arstat_sc_e[i] = D_SCINT_N_UNIT;
			}
			else{
				plogo->arstat_sc_e[i] = D_SCINT_N_OTHER;
			}
			//--- 合併による15秒単位計算用 ---
			if (nsc_unitrest >= i){					// 合併の途中構成
				plogo->arstat_sc_e[i]      = D_SCINT_N_AUNIT;
			}
			else{
				nsc_unitrest = autocm_detect_unit(plogo, i, nsc_last);
				if (nsc_unitrest > nsc_to){			// 合併あり
					nsc_unitrest = i;
				}
				if (nsc_unitrest > i){				// 合併の先頭位置
					plogo->arstat_sc_e[i]      = D_SCINT_N_BUNIT;
				}
				else if (nsc_unitrest == i){		// 単体CMを念のため再設定
					plogo->arstat_sc_e[i] = D_SCINT_N_UNIT;
				}
			}
			nsc_last = i;
		}
	}
}

//---------------------------------------------------------------------
// カウント状態を変更
// 出力：
//  cntset  : CM構成候補検索用のカウント状態
//---------------------------------------------------------------------
void autocm_cmdet_cnt(int cntset[D_AUTOCM_NUM_SMAX][D_AUTOCM_NUM_CMAX], int type, int st, int ed){
	int i, j;

	if (type == D_AUTOCM_T_INIT){
		//--- 初期化処理 ---
		for(i=st; i<=ed; i++){
			for(j=0; j<D_AUTOCM_NUM_CMAX; j++){
				cntset[i][j] = 0;
			}
		}
	}
	else if (type == D_AUTOCM_T_SFT){
		//--- シフト処理 ---
		for(i=st; i<D_AUTOCM_NUM_SMAX; i++){
			if (i < D_AUTOCM_NUM_SMAX+st-ed){
				for(j=0; j<D_AUTOCM_NUM_CMAX; j++){
					cntset[i][j] = cntset[i+ed-st][j];
				}
			}
			else{
				for(j=0; j<D_AUTOCM_NUM_CMAX; j++){
					cntset[i][j] = 0;
				}
			}
		}
	}
	else if (type == D_AUTOCM_T_MRG){
		//--- 合併処理 ---
		for(i=st+1; i<=ed; i++){
			for(j=2; j<D_AUTOCM_NUM_CMAX; j++){		// シーンチェンジ番号以外
				cntset[st][j] += cntset[i][j];
			}
		}
		cntset[st][D_AUTOCM_C_NSCED] = cntset[ed][D_AUTOCM_C_NSCED];
		//--- 残りをシフト ---
		for(i=st+1; i<D_AUTOCM_NUM_SMAX; i++){
			if (i < D_AUTOCM_NUM_SMAX+st-ed){
				for(j=0; j<D_AUTOCM_NUM_CMAX; j++){
					cntset[i][j] = cntset[i+ed-st][j];
				}
			}
			else{
				for(j=0; j<D_AUTOCM_NUM_CMAX; j++){
					cntset[i][j] = 0;
				}
			}
		}
	}
}


//---------------------------------------------------------------------
// ロゴデータ情報を書き換え
// 出力：
//  plogo->num_find    : ロゴ合計数
//  plogo->frm_rise*   : ロゴ開始位置
//  plogo->frm_fall*   : ロゴ終了位置
//---------------------------------------------------------------------
void autocm_cmdet_mklogo(LOGO_RESULTREC *plogo){
	int i;
	int flag_logo, flag_update;
	int num_find;
	int arstat_cur;
	int frm_cur, frm_last, frm_logo_st, frm_logo_ed;

	num_find = 0;
	flag_logo = 0;
	frm_last = 0;
	frm_logo_st = 0;
	frm_logo_ed = 0;
	for(i=1; i < plogo->num_scpos; i++){
		flag_update = 0;
		if (plogo->stat_scpos[i] >= 2 || i==plogo->num_scpos-1){
			frm_cur = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
//printf("[%d,%d]",frm_cur,arstat_cur);
			//--- ロゴあり構成検出時 ---
			if (arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH){
				if (flag_logo == 0){				// ロゴ開始
					frm_logo_st = frm_last;
					flag_logo = 1;
				}
				if (i==plogo->num_scpos-1){			// 最後までロゴ
					frm_logo_ed = frm_cur;
					flag_update = 1;
				}
			}
			//--- ロゴなし構成検出時 ---
			else{
				if (flag_logo == 1){				// ロゴ終了
					frm_logo_ed = frm_last;
					flag_logo = 0;
					flag_update = 1;
				}
			}
			//--- ロゴ情報追加 ---
			if (flag_update > 0){
				plogo->frm_rise[num_find]   = frm_logo_st;
				plogo->frm_rise_l[num_find] = frm_logo_st;
				plogo->frm_rise_r[num_find] = frm_logo_st;
				plogo->fade_rise[num_find]  = 0;
				plogo->intl_rise[num_find]  = 0;
				plogo->frm_fall[num_find]   = frm_logo_ed;
				plogo->frm_fall_l[num_find] = frm_logo_ed;
				plogo->frm_fall_r[num_find] = frm_logo_ed;
				plogo->fade_fall[num_find]  = 0;
				plogo->intl_fall[num_find]  = 0;
				num_find ++;
			}
			//--- 前位置として記憶 ---
			frm_last = frm_cur;
		}
	}
	plogo->num_find = num_find;
}


//---------------------------------------------------------------------
// 先頭部分のCM扱いを検出して変更
// 出力：
//  返り値  : 次の検出シーンチェンジ番号
//  *r_flag_headdet         : 先頭直後カット方針（0:緩め 1:厳しめ 2:なし）
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
int autocm_cmdet_headproc(LOGO_RESULTREC *plogo, int *r_flag_headdet){
	int i;
	int flag_headdet;
	int nsc_head1, nsc_last;

	//--- 先頭部分検索 ---
	flag_headdet = 0;
	nsc_last = 0;
	nsc_head1 = -1;
	i = 1;
	while((plogo->stat_scpos[i] < 2 || plogo->frm_scpos_s[i] == 0) && i < plogo->num_scpos){
		if (nsc_head1 < 0 && plogo->frm_scpos_s[i] > 0){
			nsc_head1 = i;			// 最初の無音シーンチェンジ
		}
		i ++;
	}
	if (i < plogo->num_scpos){
		if (plogo->arstat_sc_e[i] == D_SCINT_L_OTHER ||
			plogo->arstat_sc_e[i] == D_SCINT_L_UNIT){
			if (plogo->frm_scpos_1stsel < 0){
				if (plogo->frm_scpos_s[i] < CnvTimeFrm(30, 400)){	// 先頭CM扱い
					if (plogo->arstat_sc_e[i] == D_SCINT_L_UNIT){
						plogo->arstat_sc_e[i] = D_SCINT_N_UNIT;
					}
					else{
						plogo->arstat_sc_e[i] = D_SCINT_N_OTHER;
					}
				}
				else if (nsc_head1 > 0){
					if (plogo->frm_scpos_s[nsc_head1] < CnvTimeFrm(15, 0)){
						plogo->stat_scpos[nsc_head1] = 2;
						plogo->arstat_sc_e[nsc_head1] = D_SCINT_N_OTHER;
						i = nsc_head1;
					}
				}
				else{
					flag_headdet = 2;		// 先頭カットなし
				}
			}
			else if (plogo->frm_scpos_1stsel > 0 &&
					 plogo->frm_scpos_1stsel < CnvTimeFrm(30, 400)){
				flag_headdet = 1;			// 先頭カット厳しめに見る
			}
			else{
				flag_headdet = 2;			// 先頭カットなし
			}
		}
		else if (plogo->frm_scpos_1stsel > 0){
			if (plogo->frm_scpos_1stsel > 0 &&
				 plogo->frm_scpos_1stsel < CnvTimeFrm(30, 400)){
				flag_headdet = 1;			// 先頭カット厳しめに見る
			}
			else{
				flag_headdet = 2;			// 先頭カットなし
			}
		}
		nsc_last = i;
	}
	*r_flag_headdet = flag_headdet;

	return nsc_last;
}


//---------------------------------------------------------------------
// ロゴ周辺であらためて構成追加
// （auto_arrange_logo_peri を呼び出し）
// 出力：
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void autocm_cmdet_logoperi(LOGO_RESULTREC *plogo){
	int frm_logo_rise, frm_logo_fall;
	int frm_logo_lastfall, frm_logo_nextrise;
	int nsc_rise, nsc_fall;
	int nlg;
	int frm_spc, frm_wlogo_edge;

	frm_spc = plogo->frm_spc;
	frm_wlogo_edge = CnvTimeFrm(120, 0) + frm_spc;		// ロゴ切り替わり付近検出用

	for(nlg = 0; nlg < plogo->num_find; nlg++){
		//--- ロゴのフレーム位置取得 ---
		frm_logo_rise = plogo->frm_rise[nlg];
		frm_logo_fall = plogo->frm_fall[nlg]+1;
		if (nlg == 0){
			frm_logo_lastfall = -frm_spc;
		}
		else{
			frm_logo_lastfall = plogo->frm_fall[nlg-1]+1;
		}
		if (nlg == plogo->num_find - 1){
			frm_logo_nextrise = plogo->frm_totalmax;
		}
		else{
			frm_logo_nextrise = plogo->frm_rise[nlg+1];
		}
		//--- 基準ロゴに対応するシーンチェンジ番号を取得 ---
		nsc_rise = auto_getnsc(plogo, frm_logo_rise, frm_spc);
		nsc_fall = auto_getnsc(plogo, frm_logo_fall, frm_spc);

		//--- 対象ロゴ周辺であらためて構成追加 ---
		auto_arrange_logo_peri(plogo,
			frm_logo_lastfall, frm_logo_rise, frm_logo_fall, frm_logo_nextrise, nsc_rise, nsc_fall,
			frm_wlogo_edge);

	}
}

//---------------------------------------------------------------------
// 指定範囲内の60/90/120秒をCM化
// 出力：
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void autocm_cmtail_sub(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_tlong_st, int frm_tlong_ed){
	int i, j;
	int frm_last, frm_cur, frm_tmplast, frm_tmpcur;
	int nsc_lgstart, nsc_last, nsc_dov15;
	int arstat_cur, arstat_tmpcur;
	int secdif_cur, secdif_tmpcur;
	int flag_dov15, flag_logoetc, flag_chk;

	nsc_lgstart = -1;
	nsc_dov15  = -1;
	nsc_last   = 0;
	flag_dov15 = 0;
	for(i=1; i<plogo->num_scpos; i++){
		if (plogo->stat_scpos[i] == 2){
			//--- 構成状態確認 ---
			frm_last = plogo->frm_scpos_s[nsc_last];
			frm_cur = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
			secdif_cur = GetSecFromFrm(frm_cur - frm_last);
			flag_logoetc = 0;
			if (arstat_cur == D_SCINT_L_UNIT || arstat_cur == D_SCINT_L_OTHER){
				flag_logoetc = 1;
			}
			//--- CM化チェック ---
			if (frm_tlong_st < frm_cur && frm_last < frm_tlong_ed){
				if (flag_logoetc > 0){
					if (secdif_cur >= 60 && secdif_cur <= 120 && (secdif_cur % 30 == 0)){
						//--- CM化 ---
						plogo->arstat_sc_e[i] = D_SCINT_N_UNIT;
						flag_logoetc = 0;
						//-- 前側のCM化 ---
						if (nsc_lgstart > 0 || nsc_dov15 > 0){
							if (nsc_lgstart < 0 || nsc_dov15 < nsc_lgstart){
								nsc_lgstart = nsc_dov15;
							}
							for(j=nsc_lgstart; j < i; j++){
								if (plogo->stat_scpos[j] == 2){
									if (arstat_cur == D_SCINT_L_OTHER){
										plogo->arstat_sc_e[j] = D_SCINT_N_OTHER;
									}
									else if (arstat_cur == D_SCINT_L_UNIT){
										plogo->arstat_sc_e[j] = D_SCINT_N_UNIT;
									}
								}
							}
						}
						//--- 後側のCM化 ---
						flag_chk = 0;
						frm_tmplast = plogo->frm_scpos_s[i];
						j = i+1;
						while(flag_chk == 0 && j < plogo->num_scpos){
							if (plogo->stat_scpos[j] == 2){
								frm_tmpcur = plogo->frm_scpos_s[j];
								arstat_tmpcur = plogo->arstat_sc_e[j];
								secdif_tmpcur = GetSecFromFrm(frm_tmpcur - frm_tmplast);
								if (frm_tmplast > frm_tlong_ed){
									flag_chk = 1;
								}
								else if (arstat_tmpcur == D_SCINT_L_UNIT  && secdif_tmpcur < 60){
									plogo->arstat_sc_e[j] = D_SCINT_N_UNIT;
								}
								else if (arstat_tmpcur == D_SCINT_L_OTHER && secdif_tmpcur < 15){
									plogo->arstat_sc_e[j] = D_SCINT_N_OTHER;
								}
								else{
									flag_chk = 1;
								}
								frm_tmplast = frm_tmpcur;
							}
							j ++;
						}
					}
					else if (nsc_lgstart < 0){
						nsc_lgstart = i;
					}
				}
			}
			if (flag_logoetc == 0 || secdif_cur > 60){
				nsc_lgstart = -1;
				nsc_dov15  = -1;
				flag_dov15 = 0;
			}
			else{
				if (secdif_cur >= 15 || arstat_cur != D_SCINT_L_OTHER){
					nsc_dov15  = -1;
					flag_dov15 = 1;
				}
				else{
					if (flag_dov15 == 0 && nsc_dov15 < 0){
						nsc_dov15 = i;
					}
				}
			}
			nsc_last = i;
		}
	}
}


//---------------------------------------------------------------------
// 最後部分の60/90/120秒をCM化
// 出力：
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//---------------------------------------------------------------------
void autocm_cmtail(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int sectmp;
	int frm_baseend, frm_totalmax, frm_width;
	int frm_tlong_st, frm_tlong_ed;

	//--- パラメータ取得 ---
	sectmp       = cmdset->autop_scopex;
	frm_width    = CnvTimeFrm(sectmp, 0);
	frm_baseend  = cmdset->select_frm_max;
	frm_totalmax = plogo->frm_totalmax;
	//--- 設定なければ終了 ---
	if (sectmp <= 0 || frm_baseend <= 0){
		return;
	}

	//--- 検出 ---
	frm_tlong_ed = frm_baseend;					// 60/90/120秒をCM化する範囲最後
	frm_tlong_st = frm_tlong_ed - frm_width;	// 60/90/120秒をCM化する範囲先頭
	autocm_cmtail_sub(plogo, cmdset, frm_tlong_st, frm_tlong_ed);

	//--- 複数回検出 ---
	frm_tlong_ed += frm_baseend;
	frm_tlong_st  = frm_tlong_ed - frm_width;
	while(frm_tlong_st < frm_totalmax){
		autocm_cmtail_sub(plogo, cmdset, frm_tlong_st, frm_tlong_ed);
		frm_tlong_ed += frm_baseend;
		frm_tlong_st  = frm_tlong_ed - frm_width;
	}
}


//---------------------------------------------------------------------
// ロゴ扱い部分からCM構成になる所を検出して変更
// 出力：
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->num_find    : ロゴ合計数
//  plogo->frm_rise*   : ロゴ開始位置
//  plogo->frm_fall*   : ロゴ終了位置
//---------------------------------------------------------------------
int autocm_cmdet(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int i, iunit;
	int state;
	int level_cmdet, cutlevel;
	int flag_headdet, flag_tailcm;
	int flag_cm;
	int change, dettype;
	int secdif_cur, sectmp, secmax_cm;
	int nsc_last;
	int frm_cur, frm_last;
	int frm_finaldet, frm_wlogo_trmax;
	int cntset[D_AUTOCM_NUM_SMAX][D_AUTOCM_NUM_CMAX];

	//--- パラメータ取得 ---
	level_cmdet = cmdset->autop_code % 100;
	if (level_cmdet == 0){
		return 0;
	}
	sectmp = cmdset->autop_scope;
	frm_finaldet = 0;
	if (sectmp > 0){
		frm_finaldet = plogo->frm_totalmax - CnvTimeFrm(sectmp, 0);
	}
	secmax_cm   = cmdset->autop_maxprd;
	if (secmax_cm <= 0){
		secmax_cm = 60;			// 未設定時の初期値
	}
	frm_wlogo_trmax = plogo->prmvar.frm_wlogo_trmax;

	//--- 初期化 ---
	autocm_cmdet_cnt(cntset, D_AUTOCM_T_INIT, 0, D_AUTOCM_NUM_SMAX-1);

	//--- 先頭部分検索 ---
	nsc_last = autocm_cmdet_headproc(plogo, &flag_headdet);

	//--- CM扱いを検索・追加 ---
	state = 0;
	cntset[0][D_AUTOCM_C_NSCST] = nsc_last;
	i = nsc_last;
	while(++i < plogo->num_scpos){
		if (plogo->stat_scpos[i] == 2){
			//--- 合併15秒単位検索 ---
			iunit = autocm_detect_unit(plogo, i, nsc_last);
			if (i < iunit){
				i = iunit;
			}
			//--- 構成状態確認 ---
			frm_last = plogo->frm_scpos_s[nsc_last];
			frm_cur = plogo->frm_scpos_s[i];
			secdif_cur = GetSecFromFrm(frm_cur - frm_last);
			change = 0;
			cutlevel = level_cmdet;
			flag_cm = 0;
			if (((secdif_cur % 15)==0 && secdif_cur <= secmax_cm) ||
				((secdif_cur % 30)==0 && secdif_cur <= 120 && cutlevel >= 10)){
				flag_cm = 1;
			}
			flag_tailcm = 0;
			if (frm_finaldet > 0 && frm_finaldet < frm_cur &&
				(frm_cur - frm_last <= frm_wlogo_trmax) &&
				((state % 2)==0 || cntset[state][D_AUTOCM_C_DOV15] == 0)){
				flag_tailcm = 1;
			}
			if ((state % 2) == 0){			// CM候補検索
				if (flag_cm > 0 || flag_tailcm > 0){
					cntset[state][D_AUTOCM_C_NSCED] = i;
				}
				else{
					change = 1;
				}
			}
			else if ((state % 2) == 1){		// ロゴ期間検索
				if (flag_cm > 0 || flag_tailcm > 0){
					change = 1;
				}
				else{
					cntset[state][D_AUTOCM_C_NSCED] = i;
				}
			}
			if (i == plogo->num_scpos-1){	// 最後は強制設定
				change = 1;
			}
			//--- 変化があった時に判断 ---
			if (change > 0){
				//--- CM扱い部分完了時 ---
				if ((state % 2)==0){
					//--- 先頭位置カット部分の補正 ---
					if (state == 0){
						if (flag_headdet == 1){			// 先頭カット厳しめ
							if (cutlevel > 3){
								cutlevel -= 3;
							}
							else if (cutlevel > 1){
								cutlevel = 1;
							}
						}
						else if (flag_headdet == 2){	// 先頭カットなし
							cutlevel = 0;
						}
						else if (flag_headdet == 0){	// 先頭カット条件緩く
							if (cutlevel > 0){
								cutlevel += 3;
							}
						}
					}
					//--- 最終地点の補正 ---
					if (i == plogo->num_scpos-1){
						cutlevel = 10;
					}
					//--- 不確定先送り地点の判断 ---
					if (state == 4){
						if (cntset[1][D_AUTOCM_C_DET]+cntset[3][D_AUTOCM_C_DET] <= 4 &&
							cntset[1][D_AUTOCM_C_SEC]+cntset[3][D_AUTOCM_C_SEC] >= 120){
							//--- 対象部分をCM扱いに変更 ---
							autocm_detect_chgcm(plogo,
								cntset[2][D_AUTOCM_C_NSCST],
								cntset[2][D_AUTOCM_C_NSCED]);
							//--- 手前ロゴ部分がCM扱いでないか判断後シフト ---
							if (cntset[1][D_AUTOCM_C_DOV15] == 0){
								autocm_detect_chgcm(plogo,
									cntset[1][D_AUTOCM_C_NSCST],
									cntset[1][D_AUTOCM_C_NSCED]);
							}
							autocm_cmdet_cnt(cntset, D_AUTOCM_T_SFT, 0, 2);
						}
						else{
							//--- ロゴ扱い部分として継続 ---
							autocm_cmdet_cnt(cntset, D_AUTOCM_T_MRG, 1, 3);
						}
						state = 2;
					}
					//--- 対象地点をロゴなし扱いにするか判断 ---
					// cutlevel 0 : カットなし
					//          1 : 4構成以上CM
					//          2 : 4構成以上CM、3構成の一部（周囲構成少）
					//          3 : 4構成以上CM、3構成の一部
					//          4 : 3構成以上CM
					//          5 : 3構成以上CM、2構成の一部（周囲構成少）
					//          6 : 3構成以上CM、2構成の一部
					//          7 : 2構成以上CM
					//          8 : 2構成以上CM、1構成の一部（周囲構成少）
					//          9 : 1構成以上CM
					//          10 : 1構成以上CM、120秒構成まで許可
					dettype = 0;
					if ((cntset[state][D_AUTOCM_C_DET] > 3 && cutlevel > 0) ||
						(cntset[state][D_AUTOCM_C_DET] > 2 && cutlevel > 3) ||
						(cntset[state][D_AUTOCM_C_DET] > 1 && cutlevel > 6) ||
						(cntset[state][D_AUTOCM_C_DET] > 0 && cutlevel > 8)){
						dettype = 1;		// 確定
					}
					else if ((cntset[state][D_AUTOCM_C_DET] == 2 &&
							  (cutlevel == 5 || cutlevel == 6) &&
							  (cntset[state][D_AUTOCM_C_D15] >= 2 || cntset[state][D_AUTOCM_C_D30] >= 2)) ||
							 (cntset[state][D_AUTOCM_C_DET] == 3 &&
							  (cutlevel == 2 || cutlevel == 3) &&
							  (cntset[state][D_AUTOCM_C_D15] + cntset[state][D_AUTOCM_C_D30] >= 3)) ||
							 (cntset[state][D_AUTOCM_C_DET] == 1 &&
							  (cutlevel == 8) &&
							  (cntset[state][D_AUTOCM_C_D15] + cntset[state][D_AUTOCM_C_D30] >= 1))){
						if (state == 0 || cutlevel == 3 || cutlevel == 6){
							dettype = 1;		// 先頭位置、周囲関係なくだったら確定
						}
						else{
							dettype = 2;		// 微妙先送り
						}
					}
//printf("A%d,%d,%dA", cutlevel, frm_cur,cntset[state][D_AUTOCM_C_DET]);
					//--- ロゴなし扱いにする変更処理 ---
					if ((dettype == 1) || (dettype == 2 && state == 0)){
						//--- 対象部分をCM扱いに変更 ---
						autocm_detect_chgcm(plogo,
							cntset[state][D_AUTOCM_C_NSCST],
							cntset[state][D_AUTOCM_C_NSCED]);
						//--- 手前ロゴ部分がCM扱いでないか判断後シフト ---
						if (state > 0){
							if (cntset[1][D_AUTOCM_C_DOV15] == 0){
								autocm_detect_chgcm(plogo,
									cntset[1][D_AUTOCM_C_NSCST],
									cntset[1][D_AUTOCM_C_NSCED]);
							}
							autocm_cmdet_cnt(cntset, D_AUTOCM_T_SFT, 0, 2);
						}
						state = 1;
						cntset[state][D_AUTOCM_C_NSCST] = nsc_last;
						cntset[state][D_AUTOCM_C_NSCED] = i;
					}
					else if (dettype == 2){
						//--- 判断は先送り ---
						state = 3;
						cntset[state][D_AUTOCM_C_NSCST] = nsc_last;
						cntset[state][D_AUTOCM_C_NSCED] = i;
					}
					else{
						//--- ロゴ扱い部分として継続 ---
						autocm_cmdet_cnt(cntset, D_AUTOCM_T_MRG, 1, 2);
						state = 1;
						cntset[state][D_AUTOCM_C_NSCST] = nsc_last;
						cntset[state][D_AUTOCM_C_NSCED] = i;
					}
				}
				//--- ロゴ扱い部分完了時 ---
				else{
					if (i == plogo->num_scpos-1){	// 最後のみ
						//--- 手前ロゴ部分がCM扱いでないか判断後シフト ---
						if (cntset[state][D_AUTOCM_C_DOV15] == 0){
							autocm_detect_chgcm(plogo,
								cntset[state][D_AUTOCM_C_NSCST],
								cntset[state][D_AUTOCM_C_NSCED]);
							autocm_cmdet_cnt(cntset, D_AUTOCM_T_SFT, 0, state);
						}
					}
					state ++;
					cntset[state][D_AUTOCM_C_NSCST] = nsc_last;
					cntset[state][D_AUTOCM_C_NSCED] = i;
				}
			}
			//--- カウント ---
			cntset[state][D_AUTOCM_C_DET] ++;
			cntset[state][D_AUTOCM_C_SEC] += secdif_cur;
			if (secdif_cur == 15){
				cntset[state][D_AUTOCM_C_D15] ++;
			}
			else if (secdif_cur == 30){
				cntset[state][D_AUTOCM_C_D30] ++;
			}
			else if (secdif_cur >= 15){
				cntset[state][D_AUTOCM_C_DOV15] ++;
			}
			nsc_last = i;
		}
	}
	//--- 最後部分の60/90/120秒をCM化する場合の処理 ---
	autocm_cmtail(plogo, cmdset);
	//--- ロゴデータ書き換え ---
	autocm_cmdet_mklogo(plogo);
	//--- ロゴ周辺であらためて構成追加 ---
	autocm_cmdet_logoperi(plogo);

	return 1;
}

//---------------------------------------------------------------------
// 予告開始位置を取得
// 出力：
//  返り値  : 予告開始位置となるシーンチェンジ番号（-1の時該当なし）
//---------------------------------------------------------------------
int autocut_cuttr_getlocst(LOGO_RESULTREC *plogo, 
	int frm_cut_s, int frm_cut_e,
	int prm_c_from, int prm_c_lgpre, int prm_trscope, int prm_tr1stprd,
	int prm_frm_wlogo_trmax, int prm_frm_wcomp_trmax
){
	int j;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int frmdifsec;
	int nlg, nlg_fall;
	int frm_logo_rise, frm_logo_fall, frm_logo_lastfall;
	int nsc_logo_rise, nsc_logo_fall, nsc_logo_lastfall;
	int nsc_last;
	int flag_cand;
	int nsc_dst_fall;
	int frm_dst_fall;
	int frm_cur, frm_last;
	int arstat_cur;
	int longlogo;
	int shortlogo;
	int nsc_cand;
	int frm_spc;
	int nsc_tmp1, arstat_tmp1, difsec_tmp1;
	int frm_tmp1;
	int nsc_tmp2, arstat_tmp2, difsec_tmp2;
	int frm_tmp2;
	int prm_wcomp_spmin, prm_wcomp_spmax;

	frm_spc = plogo->frm_spc;
	prm_wcomp_spmin = plogo->prmvar.sec_wcomp_spmin;
	prm_wcomp_spmax = plogo->prmvar.sec_wcomp_spmax;

	nsc_cand = -1;
	flag_cand = 0;
	for(nlg=0; nlg < plogo->num_find; nlg++){
		//--- ロゴのフレーム位置、該当シーンチェンジ位置を取得 ---
		frm_logo_rise = plogo->frm_rise[nlg];
		frm_logo_fall = plogo->frm_fall[nlg]+1;
		nsc_logo_rise = auto_getnsc(plogo, frm_logo_rise, frm_spc);
		nsc_logo_fall = auto_getnsc(plogo, frm_logo_fall, frm_spc);
		if (nlg == 0){
			frm_logo_lastfall = 0;
			nsc_logo_lastfall = 0;
		}
		else{
			frm_logo_lastfall = plogo->frm_fall[nlg-1]+1;
			nsc_logo_lastfall = auto_getnsc(plogo, frm_logo_lastfall, frm_spc);
		}
		//--- ロゴ終了が見つからない時は次のロゴ終了位置を取得 ---
		nlg_fall = nlg;
		if (nsc_logo_rise >= 0 && nsc_logo_fall < 0){
			while(nsc_logo_fall < 0 && nlg_fall < plogo->num_find - 1){
				nlg_fall ++;
				frm_logo_fall = plogo->frm_fall[nlg_fall]+1;
				nsc_logo_fall = auto_getnsc(plogo, frm_logo_fall, frm_spc);
			}
		}
		if (frm_logo_fall > frm_cut_s + frm_spc && frm_logo_fall < frm_cut_e + frm_spc){
			//--- ロゴ位置に対応するシーンチェンジがある場合 ---
			if (nsc_logo_rise >= 0 && nsc_logo_fall >= 0){
				//--- ロゴ期間が予告用の長さか確認 ---
				longlogo = 0;
				if (frm_logo_rise < frm_cut_s - frm_spc){
					longlogo = 1;
				}
				else if (frm_logo_fall - frm_logo_rise > prm_frm_wlogo_trmax + frm_spc){
					longlogo = 1;
				}
				//--- ロゴ終了地点を検索（ロゴエッジ処理している所は除く） ---
				nsc_dst_fall = nsc_logo_fall;
				arstat_cur = plogo->arstat_sc_e[nsc_dst_fall];
				while(((arstat_cur == D_SCINT_L_LGCUT) ||
					   (arstat_cur == D_SCINT_L_LGADD)) &&
					  nsc_dst_fall > nsc_logo_rise){
					nsc_dst_fall --;
					arstat_cur = plogo->arstat_sc_e[nsc_dst_fall];
				}
				frm_dst_fall = plogo->frm_scpos_s[nsc_dst_fall];

				//--- ロゴ開始直後にすぐ終わるかチェック ---
				j = nsc_logo_rise + 1;
				if (j > nsc_dst_fall){
					j = nsc_dst_fall;
				}
				while(plogo->stat_scpos[j] < 2 && j < nsc_dst_fall){
					j ++;
				}
				shortlogo = 0;
				if (j != nsc_dst_fall){
				}
				else if (GetSecFromFrm(frm_dst_fall - frm_logo_rise) < 15){
					shortlogo = 1;
				}
				//--- ロゴ手前でも位置確定がまだで予告候補があればチェック ---
				if (nsc_logo_rise < nsc_dst_fall && flag_cand == 0 && shortlogo > 0 &&
					prm_c_lgpre == 1){
					arstat_cur = plogo->arstat_sc_e[nsc_logo_rise];
					// ロゴ手前がCM15秒単位検出ではない時
					if ((arstat_cur != D_SCINT_N_UNIT) &&
						(arstat_cur != D_SCINT_N_BUNIT)){
						// １つ前の構成を取得
						j = nsc_logo_rise - 1;
						while(plogo->stat_scpos[j] < 2 && j > nsc_logo_lastfall){
							j --;
						}
						nsc_tmp1 = j;
						frm_tmp1 = plogo->frm_scpos_s[nsc_tmp1];
						arstat_tmp1 = plogo->arstat_sc_e[nsc_tmp1];
						difsec_tmp1 = GetSecFromFrm(frm_logo_rise - frm_tmp1);
						// ２つ前の構成を取得
						if (j > nsc_logo_lastfall){
							j --;
						}
						while(plogo->stat_scpos[j] < 2 && j > nsc_logo_lastfall){
							j --;
						}
						nsc_tmp2 = j;
						frm_tmp2 = plogo->frm_scpos_s[nsc_tmp2];
						arstat_tmp2 = plogo->arstat_sc_e[nsc_tmp2];
						difsec_tmp2 = GetSecFromFrm(frm_tmp1 - frm_tmp2);
						// ２つ前はCM15秒単位だった場合
						if (nsc_tmp1 > nsc_logo_lastfall &&
							(arstat_tmp1 == D_SCINT_N_UNIT ||
							 arstat_tmp1 == D_SCINT_N_BUNIT)){
							// １つ前の構成が番組提供の構成外であれば予告に追加
							if ((difsec_tmp1 < prm_wcomp_spmin || difsec_tmp1 > prm_wcomp_spmax) &&
								(difsec_tmp1 < prm_tr1stprd || prm_tr1stprd == 0)){
								nsc_cand = nsc_tmp1;
								flag_cand = 1;
							}
						}
						// ２つ前もCM15秒単位検出ではなく、３つ前がCM15秒単位だった場合
						else if (nsc_tmp2 > nsc_logo_lastfall &&
								 (arstat_tmp2 == D_SCINT_N_UNIT ||
								  arstat_tmp2 == D_SCINT_N_BUNIT)){
							// １つ前の構成が番組提供の秒数であれば２つ前の構成を予告に追加
							if ((difsec_tmp1 >= prm_wcomp_spmin && difsec_tmp1 <= prm_wcomp_spmax) &&
								(difsec_tmp2 < prm_tr1stprd || prm_tr1stprd == 0)){
								nsc_cand = nsc_tmp2;
								flag_cand = 1;
							}
						}
					}
				}
				//--- 自動判別でCM明けのロゴがあれば優先させる場合 ---
				if (nsc_logo_rise < nsc_dst_fall && flag_cand == 0 && nsc_cand >= 0){
					arstat_cur = plogo->arstat_sc_e[nsc_logo_rise];
					// ロゴ手前がCM検出ではない時はCM明けから
					if ((arstat_cur != D_SCINT_N_UNIT) &&
						(arstat_cur != D_SCINT_N_BUNIT)){
						nsc_cand = -1;
					}
					else{
						if (shortlogo == 0){
							nsc_cand = -1;
						}
					}
					if (nsc_cand >= 0){
						// ロゴ期間終了位置から次の構成位置を取得
						j = nsc_dst_fall + 1;
						while(plogo->stat_scpos[j] < 2 && j < plogo->num_find - 1){
							j ++;
						}
						// ロゴ直後がCM検出ではない時はCM明けから
						arstat_cur = plogo->arstat_sc_e[j];
						if ((arstat_cur != D_SCINT_N_UNIT) &&
							(arstat_cur != D_SCINT_N_AUNIT) &&
							(arstat_cur != D_SCINT_N_BUNIT)){
							nsc_cand = -1;
						}
					}
				}
				//--- ロゴ期間内のシーンチェンジ確認 ---
				nsc_last = -1;
				for(j=nsc_logo_rise; j <= nsc_dst_fall; j++){
					frm_cur = plogo->frm_scpos_s[j];
					arstat_cur = plogo->arstat_sc_e[j];
					if (frm_cur >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc &&
						nsc_last >=0 &&
						arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH &&
						((arstat_cur != D_SCINT_L_LGCUT) &&	// エッジ処理済み部分は除く
						 (arstat_cur != D_SCINT_L_LGADD))){
						//--- 前回からの期間を取得 ---
						flag1 = adjust_calcdif(&ncal_sgn1, &ncal_sec1, &ncal_dis1, frm_last, frm_cur);
						frmdifsec = GetSecFromFrm(frm_dst_fall - frm_cur);
						//--- 構成が予告期間以上あったら本編と認識 ---
						if (frm_cur - frm_last > prm_frm_wcomp_trmax + frm_spc){
							longlogo = 1;
							nsc_cand = -1;
						}
						//--- 最初の予告位置設定 ---
						else if (nsc_cand < 0){
							// 構成期間が所定以下なら設定
							if (ncal_sec1 <= prm_tr1stprd || prm_tr1stprd == 0){
								nsc_cand = nsc_last;
							}
						}
						//--- 本編後の同ロゴ内位置だった場合条件を満たすか確認 ---
						if (longlogo > 0){
							//--- CM明けから数える場合は本編ロゴ中は無効化 ---
							if (prm_c_from == 1){
								 nsc_cand = -1;
							}
							else if ((((flag1 == 0) || (ncal_sec1 % 15) != 0) && prm_c_from == 5) ||
									 (((flag1 == 0) || (ncal_sec1 % 5)  != 0) && prm_c_from == 4) ||
									 (frmdifsec > prm_trscope)){
									 nsc_cand = -1;
							}
						}
					}
					//--- 確定するロゴ期間の場合はフラグ設定 ---
					if (longlogo > 0 && prm_c_from == 2){
						flag_cand = 0;
					}
					else{
						if (nsc_cand >= 0){
							flag_cand = 1;
						}
					}
					//--- 確定位置だったら前回位置を今回の位置に変更 ---
					if (plogo->stat_scpos[j] >= 2){
						nsc_last = j;
						frm_last = plogo->frm_scpos_s[nsc_last];
					}
				}
			}
		}
	}
//printf("(%d,%d)",nsc_cand, (int)plogo->frm_scpos_s[nsc_cand]);
	return nsc_cand;
}


//---------------------------------------------------------------------
// AutoCut TR 予告を残し番宣をカットする処理
// 出力：
//  返り値  : カット処理実行 0=未実行 1=実行
//  plogo->arstat_sc_e[] : 予告以降の配置状態を変更
//---------------------------------------------------------------------
int autocut_cuttr(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int i;
	int ret;
	int prm_limit, prm_trscope, prm_trsumprd;
	int prm_trscope_tmp, prm_trsumprd_tmp;
	int prm_tr1stprd;
	int prm_c_from, prm_c_cutst, prm_c_lgpre;
	int prm_c_exe;
	int prm_frm_wlogo_trmax;
	int prm_frm_wcomp_trmax;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int nlg, nlg_start;
	int nsc_trstart;
	int nsc_last;
	int frm_logo_rise, frm_logo_fall;
	int frm_cur, frm_last;
	int state_cut;
	int arstat_cur;
	int ncut_rest;
	int sec_tr_total;
	int frm_spc;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	//--- パラメータ取得 ---
	prm_limit = cmdset->autop_limit;
	prm_trscope_tmp  = cmdset->autop_trscope;
	prm_trsumprd_tmp = cmdset->autop_trsumprd;
	prm_tr1stprd  = cmdset->autop_tr1stprd;
	prm_c_from  = cmdset->autop_code % 10;
	prm_c_cutst = (cmdset->autop_code / 10) % 10;
	prm_c_lgpre = (cmdset->autop_code / 100) % 10;
	prm_noedge  = 1;
	prm_frm_wlogo_trmax = plogo->prmvar.frm_wlogo_trmax;
	prm_frm_wcomp_trmax = plogo->prmvar.frm_wcomp_trmax;
	//--- 実行判断 ---
	prm_c_exe = (prm_c_from != 0)? 1 : 0;
	//--- デフォルト値付き設定 ---
	prm_trscope  = (prm_trscope_tmp == 0)? 30 : prm_trscope_tmp;
	prm_trsumprd = (prm_trsumprd_tmp == 0)?  3 : prm_trsumprd_tmp;

	//--- 初期状態設定 ---
	frm_spc = plogo->frm_spc;
	//--- 実行有無確認 ---
	if (prm_c_exe == 0){
		return 0;
	}

	//--- 予告開始位置を取得 ---
	nsc_trstart = autocut_cuttr_getlocst(plogo, 
		frm_cut_s, frm_cut_e,
		prm_c_from, prm_c_lgpre, prm_trscope, prm_tr1stprd, prm_frm_wlogo_trmax, prm_frm_wcomp_trmax
	);
//printf("(%d,%d,%d,%d,%d,%d,%d,%d,%d)", nsc_trstart, frm_cut_s, frm_cut_e,prm_c_from, prm_c_lgpre, prm_trscope, prm_tr1stprd, prm_frm_wlogo_trmax, prm_frm_wcomp_trmax);
	if (nsc_trstart < 0){	// 予告候補が見つからない場合
		//--- 予告検出なしの検出 ---
		plogo->frm_notfound_tr = frm_cut_s;
		return 0;
	}

	//--- 予告位置を設定 ---
	ret = 0;
	state_cut = 0;
	sec_tr_total = 0;
	ncut_rest = prm_limit;
	nsc_last = -1;
	nlg = 0;
	nlg_start = 0;
	frm_logo_rise = plogo->frm_rise[nlg];
	frm_logo_fall = plogo->frm_fall[nlg]+1;
	i = nsc_trstart - 1;
	while(++i < plogo->num_scpos - prm_noedge){
		frm_cur  = plogo->frm_scpos_s[i];
		arstat_cur = plogo->arstat_sc_e[i];
//printf("%d,%d,%d,%d,%d,%d,%d,%d  ", frm_cur, arstat_cur, i, nsc_last, frm_logo_rise, frm_logo_fall, state_cut, ncut_rest);
		if (plogo->stat_scpos[i] < 2){
		}
		//--- 範囲内でロゴ有の配置位置だった場合のみ実行 ---
		else if (frm_cur >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc &&
			nsc_last >=0 &&
			((arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH) ||
			 (nsc_last == nsc_trstart && plogo->stat_scpos[i] >= 2)) &&			// ロゴ部分または予告開始部分
			((arstat_cur != D_SCINT_L_LGCUT) &&		// エッジ処理済み部分は除く
			 (arstat_cur != D_SCINT_L_LGADD))){

			//--- シーンチェンジに対応するロゴ位置取得 ---
			while(frm_cur > frm_logo_fall + frm_spc && nlg < plogo->num_find-1){
				nlg ++;
				frm_logo_rise = plogo->frm_rise[nlg];
				frm_logo_fall = plogo->frm_fall[nlg]+1;
			}
			//--- 前フレームからの期間 ---
			flag1 = adjust_calcdif(&ncal_sgn1, &ncal_sec1, &ncal_dis1, frm_last, frm_cur);

			//--- 初回チェック ---
			if (state_cut == 0){
				nlg_start = nlg;
				state_cut = 1;
			}
			//--- カット開始条件判断 ---
			if (state_cut == 1 && ncut_rest == 0 && sec_tr_total >= prm_trsumprd){
				if (((flag1 > 0 && ncal_sec1 % 15 == 0) && prm_c_cutst == 0) ||
					((flag1 > 0 && ncal_sec1 >= 15) && prm_c_cutst == 1) ||
					(prm_c_cutst == 2)){
					state_cut = 2;
				}
			}
			//--- カット開始後の処理 ---
			if (state_cut == 2){
				if ((frm_logo_fall - frm_logo_rise <= prm_frm_wlogo_trmax + frm_spc ||
					 nlg == nlg_start) &&											// ロゴが２分以内か開始ロゴ
					frm_logo_fall - frm_cur <= prm_frm_wlogo_trmax + frm_spc &&		// ロゴ終了まで２分以内
					frm_cur - frm_last <= prm_frm_wcomp_trmax + frm_spc &&			// 構成が60秒以内
					frm_logo_fall <= frm_cut_e + frm_spc){							// ロゴ終了地点が範囲内
					if (ncal_sec1 >= 15){
						plogo->arstat_sc_e[i] = D_SCINT_L_TRCUT;
					}
					else{
						plogo->arstat_sc_e[i] = D_SCINT_L_ECCUT;
					}
					ret = 1;
				}
				else{
					state_cut = 3;					// 完全終了
				}
			}
			//--- カット開始まで残す処理 ---
			if (state_cut == 1){
				if (ncut_rest > 0){
					plogo->arstat_sc_e[i] = D_SCINT_L_TRKEEP;
					ncut_rest --;
					sec_tr_total += ncal_sec1;
				}
				else{								// エンドカード判断待ち
					plogo->arstat_sc_e[i] = D_SCINT_L_TRRAW;
				}
			}
		}
		else if (frm_cur >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc &&
					nsc_last >=0 &&
					(arstat_cur == D_SCINT_N_OTHER)){	// ロゴなし不明構成
			if (state_cut == 1 || state_cut == 2){		// カット期間中
				plogo->arstat_sc_e[i] = D_SCINT_N_TRCUT;
			}
		}
		//--- 確定位置だったら前回位置を今回の位置に変更 ---
		if (plogo->stat_scpos[i] >= 2){
			nsc_last = i;
			frm_last = plogo->frm_scpos_s[i];
		}
	}
	return ret;
}


//---------------------------------------------------------------------
// 入力秒数がエンドカード検索範囲内かチェック
// 出力：
//  返り値  : 0=エンドカード検索範囲外 1=エンドカード検索範囲内
//---------------------------------------------------------------------
int autocut_cutec_getscope(int frmdifsec, int prm_c_sel, int prm_period, int prm_maxprd){

	if ((frmdifsec < 15 && prm_c_sel == 1) ||
		(frmdifsec <= prm_period && prm_c_sel == 3) ||
		(frmdifsec >= prm_period && frmdifsec < 15 && prm_c_sel == 4) ||
		(frmdifsec >= prm_period && frmdifsec < prm_maxprd && prm_c_sel == 5) ||
		(frmdifsec == prm_period && prm_c_sel == 2)){
		return 1;
	}
	else{
		return 0;
	}
}

//---------------------------------------------------------------------
// 開始位置・情報を取得
// 出力：
//  返り値  : エンドカードのカット開始位置となるシーンチェンジ番号（-1の時該当なし）
//  local_cntcut[] : 各シーンチェンジ番号のエンドカード候補を１から順番に番号付け
//  r_ovw_force    : 0:既存情報の上書きなし 1-:既存の番組提供・エンドカード設定を上書き開始する番号
//---------------------------------------------------------------------
int autocut_cutec_getlocst(
	int *local_cntcut,			// カット対象構成を１から順番に番号記載（出力）
	int *r_ovw_force,
	LOGO_RESULTREC *plogo, 
	int frm_cut_s, int frm_cut_e,
	int prm_limit, int prm_period, int prm_maxprd, int prm_c_sel, int prm_c_cutsp,
	int prm_c_cutla, int prm_c_cutlp, int prm_c_cut30
){
	int i;
	int nsc_last;
	int nsc_cand;
	int frm_cur, frm_last;
	int frmdifsec_sc, frmdifsec_lastsc;
	int arstat_cur;
	int flag_cand, flag_existec;
	int flag_end, flag_valid, flag_cont;
	int n_cutmax, n_cutdst;
	int ovw_force;
	int frm_spc;
	int prmflag_cutlast;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	//--- 初期状態設定 ---
	for(i=0; i<plogo->num_scpos; i++){
		local_cntcut[i] = 0;
	}
	if (prm_limit < 0 || prm_c_cutla > 0 || prm_c_cutlp > 0){
		prmflag_cutlast = 1;
	}
	else{
		prmflag_cutlast = 0;
	}
	frm_spc = plogo->frm_spc;

	//--- 先頭位置を検索 ---
	flag_existec = 0;
	flag_end = 0;
	flag_cand = 0;
	nsc_cand = -1;
	i = 0;
	if (prm_noedge > 0){
		nsc_last = -1;
	}
	while(++i < plogo->num_scpos - prm_noedge && flag_end == 0){
		frm_cur  = plogo->frm_scpos_s[i];
		arstat_cur = plogo->arstat_sc_e[i];
		//--- 範囲内でロゴ有の配置位置だった場合のみ実行 ---
		if (frm_cur >= frm_cut_s && frm_cur <= frm_cut_e + frm_spc && nsc_last >= 0){
//printf("(%d,%d,%d,%d)",arstat_cur,nsc_cand,flag_cand,frm_cur);
			//--- エンドカードとして有効な所を検索する ---
			if (arstat_cur == D_SCINT_L_TRRAW){
				if (flag_cand == 0){
					nsc_cand = nsc_last;
					flag_cand = 1;
				}
			}
			else if (arstat_cur == D_SCINT_L_EC){
				if (flag_cand == 0 || flag_existec == 0){
					nsc_cand = nsc_last;
					flag_cand = 1;
					flag_existec = 1;
				}
			}
			else if (arstat_cur == D_SCINT_L_SP){
				if (prmflag_cutlast > 0 || prm_c_cutsp > 0){	// カット対象とする場合
					if (nsc_cand < 0){
						nsc_cand = nsc_last;
					}
				}
				else{
					nsc_cand = -1;
					flag_cand = 0;
				}
			}
			else if (arstat_cur == D_SCINT_L_TRCUT ||
					 arstat_cur == D_SCINT_L_ECCUT){
				flag_end = 1;
			}
			else if (arstat_cur == D_SCINT_L_TRKEEP){
				nsc_cand = -1;
				flag_cand = 0;
				frm_last = plogo->frm_scpos_s[nsc_last];
				frmdifsec_sc = GetSecFromFrm(frm_cur - frm_last);
				if (prm_c_cut30 > 0){
					if (frmdifsec_sc == 30){
						prm_limit = 0;			// エンドカードなし
					}
				}
			}
			else if (arstat_cur >= D_SCINTR_L_LOW &&
					 arstat_cur <= D_SCINTR_L_HIGH &&
					 arstat_cur != D_SCINT_L_LGCUT &&
					 arstat_cur != D_SCINT_L_LGADD){
				if (flag_cand == 0){
					frm_last = plogo->frm_scpos_s[nsc_last];
					frmdifsec_sc = GetSecFromFrm(frm_cur - frm_last);
					// 検索範囲内かチェック
					if (autocut_cutec_getscope(frmdifsec_sc, prm_c_sel, prm_period, prm_maxprd) > 0){
						if (prmflag_cutlast > 0 && nsc_cand < 0){
							nsc_cand = nsc_last;
						}
					}
					else{
						if (flag_cand == 0 && nsc_cand >= 0){
							nsc_cand = -1;
						}
					}
				}
			}
		}
		else if (frm_cur > frm_cut_e){
			flag_end = 1;
		}
		//--- 確定位置だったら前回位置を今回の位置に変更 ---
		if (plogo->stat_scpos[i] >= 2){
			nsc_last = i;
		}
	}

	//--- 各シーンチェンジ位置順番付け ---
	n_cutmax = 0;
	if (nsc_cand >= 0){
		flag_end = 0;
		flag_cont = 0;
		i = nsc_cand;
		nsc_last = nsc_cand;
		while(++i < plogo->num_scpos - prm_noedge && flag_end == 0){
			frm_cur  = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
			//--- 範囲内でロゴ有の配置位置だった場合のみ実行 ---
			if (frm_cur >= frm_cut_s && frm_cur <= frm_cut_e + frm_spc && nsc_last >= 0){
				flag_valid = 0;
				//--- エンドカードとして有効な所を検索する ---
				if (arstat_cur == D_SCINT_L_TRRAW){
					flag_valid = 1;
				}
				else if (arstat_cur == D_SCINT_L_EC){
					flag_valid = 1;
				}
				else if (arstat_cur == D_SCINT_L_SP){
					flag_cont = 1;									// 継続フラグを立てる
					if (prmflag_cutlast > 0 || prm_c_cutsp > 0){	// カット対象とする場合
						flag_valid = 1;
					}
				}
				else if (arstat_cur == D_SCINT_L_TRCUT ||	// 既存カットがあれば終了
						 arstat_cur == D_SCINT_L_ECCUT){
					flag_end = 1;
				}
				else if (arstat_cur >= D_SCINTR_L_LOW &&
						 arstat_cur <= D_SCINTR_L_HIGH &&
						 arstat_cur != D_SCINT_L_LGCUT &&
						 arstat_cur != D_SCINT_L_LGADD){
					frm_last = plogo->frm_scpos_s[nsc_last];
					frmdifsec_sc = GetSecFromFrm(frm_cur - frm_last);
					// 検索範囲内かチェック
					if (autocut_cutec_getscope(frmdifsec_sc, prm_c_sel, prm_period, prm_maxprd) > 0){
						if (prmflag_cutlast > 0 || flag_cont > 0){
							flag_valid = 1;
						}
					}
					else{
						flag_cont = 0;						// 継続は終了
					}
				}
				if (flag_valid > 0){
					//--- 開始位置取得 ---
					frm_last = plogo->frm_scpos_s[nsc_last];
					frmdifsec_sc = GetSecFromFrm(frm_cur - frm_last);
					if (autocut_cutec_getscope(frmdifsec_sc, prm_c_sel, prm_period, prm_maxprd) > 0){
						n_cutmax ++;
						local_cntcut[i] = n_cutmax;
						frmdifsec_lastsc = frmdifsec_sc;	// 最後の期間を記憶
					}
				}
			}
			else if (frm_cur > frm_cut_e){
				flag_end = 1;
			}
			//--- 確定位置だったら前回位置を今回の位置に変更 ---
			if (plogo->stat_scpos[i] >= 2){
				nsc_last = i;
			}
		}
	}
	//--- カット位置の番号を決定 ---
	ovw_force = 0;
	if (prm_limit >= 0){
		n_cutdst = prm_limit + 1;
		if (n_cutmax > 0 && prmflag_cutlast > 0){	// 最後をカットする場合
			if (prm_c_cutla > 0){
				ovw_force = n_cutmax;
				if (n_cutdst > n_cutmax){
					n_cutdst = n_cutmax;
				}
			}
			else if (prm_c_cutlp > 0){
				if (autocut_cutec_getscope(frmdifsec_lastsc, 2, prm_period, prm_maxprd) > 0){
					ovw_force = n_cutmax;
					if (n_cutdst > n_cutmax){
						n_cutdst = n_cutmax;
					}
				}
			}
		}
	}
	else{
		n_cutdst = n_cutmax + prm_limit + 1;
		ovw_force = n_cutdst;
		if (prm_c_cutlp > 0){						// 期間条件がある場合
			if (autocut_cutec_getscope(frmdifsec_lastsc, 2, prm_period, prm_maxprd) == 0){
				n_cutdst = n_cutmax + 100;			// カット無効化
				ovw_force = 0;
			}
		}
	}
	*r_ovw_force = ovw_force;
	return n_cutdst;
}


//---------------------------------------------------------------------
// AutoCut EC エンドカード部分カット処理
// 出力：
//  返り値  : カット処理実行 0=未実行 1=実行
//  plogo->arstat_sc_e[] : エンドカード部分の配置状態を変更
//---------------------------------------------------------------------
int autocut_cutec(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int i;
	int ret;
	int prm_limit, prm_c_sel;
	int prm_c_cutla, prm_c_cutlp, prm_c_cut30;
	int prm_c_cutsp;
	int prm_period, prm_period_tmp;
	int prm_maxprd, prm_maxprd_tmp;
	int nsc_last;
	int frm_cur;
	int arstat_cur;
	int flag_end;
	int n_cutdst;
	int state_cut;
	int frm_spc;
	int ovw_force;
	int local_cntcut[SCPOS_FIND_MAX];
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	//--- パラメータ取得 ---
	prm_limit = cmdset->autop_limit;
	prm_period_tmp = cmdset->autop_period;
	prm_maxprd_tmp = cmdset->autop_maxprd;
	prm_c_sel  = cmdset->autop_code % 10;
	prm_c_cutla  = ((((cmdset->autop_code / 10) % 10) & 0x1) != 0)? 1 : 0;
	prm_c_cutlp  = ((((cmdset->autop_code / 10) % 10) & 0x2) != 0)? 1 : 0;
	prm_c_cut30  = 0;
	prm_c_cutsp  = ((((cmdset->autop_code / 100) % 10) & 0x1) != 0)? 1 : 0;
	//--- デフォルト値付き設定 ---
	prm_period  = (prm_period_tmp == 0)?   5 : prm_period_tmp;
	prm_maxprd  = (prm_maxprd_tmp == 0)?  14 : prm_maxprd_tmp;


	//--- 初期状態設定 ---
	frm_spc = plogo->frm_spc;
	ret = 0;

	//--- 位置情報を取得 ---
	n_cutdst = autocut_cutec_getlocst(local_cntcut, &ovw_force, plogo,
							frm_cut_s, frm_cut_e, 
							prm_limit, prm_period, prm_maxprd, prm_c_sel, prm_c_cutsp,
							prm_c_cutla, prm_c_cutlp, prm_c_cut30);

	//--- 確認した位置でカット処理 ---
	flag_end = 0;
	i = 0;
	state_cut = 0;
	nsc_last = 0;
	if (prm_noedge > 0){
		nsc_last = -1;
	}
	while(++i < plogo->num_scpos - prm_noedge && flag_end == 0){
		frm_cur  = plogo->frm_scpos_s[i];
		arstat_cur = plogo->arstat_sc_e[i];
		//--- 範囲内でロゴ有の配置位置だった場合のみ実行 ---
		if (frm_cur >= frm_cut_s && frm_cur <= frm_cut_e + frm_spc && nsc_last >= 0){
			if (state_cut == 0){
				if (local_cntcut[i] == n_cutdst){
					state_cut = 1;
				}
				else if (local_cntcut[i] > 0){
					if (prm_limit > 0){
						plogo->arstat_sc_e[i] = D_SCINT_L_EC;
					}
				}
			}
			if (state_cut == 1){
				if (local_cntcut[i] > 0){
					if ((arstat_cur != D_SCINT_L_EC &&
						 arstat_cur != D_SCINT_L_SP) ||
						(ovw_force > 0 && ovw_force <= local_cntcut[i])){
						plogo->arstat_sc_e[i] = D_SCINT_L_ECCUT;
						ret = 1;
					}
				}
				else if (arstat_cur == D_SCINT_L_TRCUT ||
						 arstat_cur == D_SCINT_L_ECCUT){
					state_cut = 2;
					flag_end = 1;
				}
			}
		}
		else if (frm_cur > frm_cut_e){
			flag_end = 1;
		}
		//--- 確定位置だったら前回位置を今回の位置に変更 ---
		if (plogo->stat_scpos[i] >= 2){
			nsc_last = i;
		}
	}
	return ret;
}


//---------------------------------------------------------------------
// AutoAddで判断する指定位置の前後状態から優先順位算出元データ取得
// 出力：
//  r_type_logo       : ロゴからの状態（0-9:ロゴなし 10-19:ロゴあり +100:ロゴ開始側）
//  r_frmdifsec_logo  : ロゴからの時間
//  r_type_tr         : 予告からの状態（0:該当なし 1:後側 2:前側 3:最初の予告位置 4:予告間隙間）
//  r_frmdifsec_tr    : 予告からの時間
//  r_type_sp         : 番組提供からの状態（0:該当なし 1:後側 2:前側 3:内部）
//  r_frmdifsec_sp    : エンドカードからの時間（前側の時のみ計測）
//  r_type_ec         : エンドカードからの状態（0:該当なし 1:後側 2:前側）
//  r_frmdifsec_ec    : 番組提供からの時間
//  r_type_nolast     : 予告ではなく後に内容がある場合=1
//  r_type_endlogo    : 範囲内に完全に含まれる最後のロゴからの状態（0:該当ロゴなし 1:ロゴ以降 2:ロゴ手前側）
//  r_frmdifsec_sc    : １つ前の番組構成からの時間
//---------------------------------------------------------------------
void autoadd_sub_locinfo(
	int *r_type_logo, int *r_frmdifsec_logo,
	int *r_type_tr, int *r_frmdifsec_tr,
	int *r_type_sp, int *r_frmdifsec_sp,
	int *r_type_ec, int *r_frmdifsec_ec,
	int *r_type_nolast,
	int *r_type_endlogo,
	int *r_frmdifsec_sc,
	LOGO_RESULTREC *plogo,
	PRMAUTOREC *prm,
	int nsc_cur, int nsc_last,
	int frm_cut_s, int frm_cut_e)
{
	int j;
	int flag_rf, flag_logo_rise, flag_logo_cur;
	int nsc_tmp1, nsc_tmp2;
	int frm_tmp1, frm_tmp2;
	int arstat_tmp1, arstat_tmp2;
	int inlogo_tmp1, inlogo_tmp2;
	int nsc_logo_l, nsc_logo_r;
	int nsc_tr, nsc_tr1st, nsc_tr1stc, nsc_pretr, nsc_trstop, nsc_tr2nd;
	int nsc_sp, nsc_presp, nsc_preec;
	int frm_cur, frm_last;
	int frm_tr_dif;
	int frm_riseend, frm_riseend_cand;
	int state_riseend, type_endlogo;
	int type_logo_add;
	int type_logo, type_tr, type_sp, type_ec;
	int type_nolast;
	int frmdif_tmp1, frmdif_tmp2, frmdif_tmp;
	int frmdifsec_logo, frmdifsec_tr, frmdifsec_sp, frmdifsec_ec, frmdifsec_sc;
	int frm_spc;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;
	int prm_frm_wcomp_trmax = plogo->prmvar.frm_wcomp_trmax;

	// 初期代入
	frm_spc  = plogo->frm_spc;
	frm_cur  = plogo->frm_scpos_s[nsc_cur];
	frm_last = plogo->frm_scpos_s[nsc_last];

	// 検索範囲となるロゴ・予告位置を取得
	type_nolast = 0;
	nsc_tmp2 = 0;		// tmp2:1つ前の位置
	frm_tmp2 = 0;
	arstat_tmp2 = D_SCINT_UNKNOWN;
	inlogo_tmp2 = 0;
	flag_logo_rise = 0;
	flag_logo_cur  = 0;
	nsc_logo_l = -1;
	nsc_logo_r = -1;
	nsc_tr     = -1;
	nsc_tr1st  = -1;
	nsc_tr1stc = -1;
	nsc_pretr  = -1;
	nsc_trstop = -1;
	nsc_tr2nd  = -1;
	nsc_sp     = -1;
	nsc_presp  = -1;
	nsc_preec  = -1;
	frm_tr_dif = 0;
	state_riseend = 0;
	frm_riseend = -1;
	frm_riseend_cand = -1;
	for(j=prm_noedge; j<plogo->num_scpos - prm_noedge; j++){
//		if (plogo->stat_scpos[j] >= 2 || plogo->nchk_scpos_1st == j){
		if (plogo->stat_scpos[j] >= 2){
			flag_rf = 0;
			nsc_tmp1 = j;			// tmp1:現在の位置
			frm_tmp1 = plogo->frm_scpos_s[j];
			arstat_tmp1 = plogo->arstat_sc_e[j];
			inlogo_tmp1 = auto_is_logoarea(plogo, nsc_tmp1);
			//--- ロゴの切り替わり地点検出 ---
			if (inlogo_tmp1 > 0 && inlogo_tmp2 == 0){
				flag_rf = 1;					// ロゴ開始
			}
			if (inlogo_tmp1 == 0 && inlogo_tmp2 > 0){
				flag_rf = 2;					// ロゴ終了
			}
			if (flag_rf > 0){
				// 現対象開始位置よりロゴ切り替わりが前の場合
				if (frm_tmp2 <= frm_last + frm_spc){
					nsc_logo_l = nsc_tmp2;		// 前側ロゴ切り替わり更新
					if (flag_rf == 1){
						flag_logo_rise = 1;		// ロゴ内
					}
					else{
						flag_logo_rise = 0;		// ロゴ外
					}
				}
				// 現対象終了位置よりロゴ切り替わりが後の場合
				if (frm_tmp2 >= frm_cur - frm_spc && nsc_logo_r < 0){
					nsc_logo_r = nsc_tmp2;		// 後側ロゴ切り替わり更新
				}
				// 現在のロゴ状態を設定
				if (flag_rf == 1){
					flag_logo_cur = 1;		// ロゴ内
				}
				else{
					flag_logo_cur = 0;		// ロゴ外
				}
			}
			//--- ロゴ最終立ち上がり位置検出 ---
			if (frm_tmp2 >= frm_cut_s - frm_spc && frm_tmp2 <= frm_cut_e + frm_spc){
				if (flag_logo_cur == 1){
					if (state_riseend == 1 && arstat_tmp1 != D_SCINT_L_LGCUT){	// ロゴカット部分以外
						frm_riseend_cand = frm_tmp2;
						state_riseend = 2;
					}
				}
				else{
					if (state_riseend == 2){
						frm_riseend = frm_riseend_cand;
						state_riseend = 1;
					}
				}
			}
			if (frm_tmp1 >= frm_cut_s - frm_spc && frm_tmp1 <= frm_cut_e + frm_spc){
				if (flag_logo_cur == 0){
					state_riseend = 1;
				}
			}
			//--- 予告・番組提供は範囲内のみ検索 ---
			if (frm_tmp2 >= frm_cut_s - frm_spc && frm_tmp1 <= frm_cut_e + frm_spc){
				//--- 予告部分 ---
				if (arstat_tmp1 == D_SCINT_L_TRKEEP){
					if (nsc_tr1st < 0){							// 一番最初
						nsc_tr1st  = nsc_tmp2;
						nsc_tr1stc = nsc_tmp1;
					}
//					if (frm_tmp1 <= frm_cur + frm_spc){			// 予告が現位置と同じか手前
					if (nsc_tr < 0 || abs(frm_tmp1 - frm_cur) <= frm_tr_dif){
						nsc_tr    = nsc_tmp1;
						nsc_pretr = nsc_tmp2;
						frm_tr_dif = abs(frm_tmp1 - frm_cur);
					}
					if (nsc_trstop >= 0 && nsc_tmp1 <= nsc_cur){	// 予告終了位置変更
						nsc_trstop = -1;
						nsc_tr2nd  = -1;
					}
					if (nsc_trstop >= 0 && nsc_tr2nd < 0){		// 一度予告終了後再開
						nsc_tr2nd = nsc_tmp2;
					}
				}
				else if (nsc_tr1st >= 0 && nsc_trstop < 0){		// 予告終了位置
					nsc_trstop = nsc_tmp1;
				}
				//--- 番組提供部分 ---
				if (arstat_tmp1 == D_SCINT_L_SP){
					if (nsc_presp < 0){							// 一番最初
						nsc_presp = nsc_tmp2;
					}
					nsc_sp    = nsc_tmp1;
				}
				//--- エンドカード部分 ---
				if (arstat_tmp1 == D_SCINT_L_EC){
					if (nsc_preec < 0){							// 一番最初
						nsc_preec = nsc_tmp2;
					}
				}
				//--- 予告・番組提供有効範囲チェック ---
				if (frm_tmp1 - frm_tmp2 > prm_frm_wcomp_trmax + frm_spc){
					if (frm_tmp1 > frm_cur - frm_spc){			// 現在地点より後に予告を超える構成
						if (flag_logo_cur == 1){				// ロゴ内
							if (arstat_tmp1 != D_SCINT_L_LGCUT){	// ロゴカット部分以外
								type_nolast = 1;					// 本編がまだ後に存在
							}
						}
					}
				}
				//--- 予告・番組提供無効化チェック ---
				if (arstat_tmp1 == D_SCINT_L_TRCUT ||
					arstat_tmp1 == D_SCINT_L_ECCUT){
					if (frm_tmp1 < frm_cur - frm_spc){			// 予告カットが手前に存在
						if (prm->c_cutskip == 0){				// カット以降は無効の時
							nsc_tr     = -1;
							nsc_tr1st  = -1;
							nsc_tr1stc = -1;
							nsc_sp     = -1;
							nsc_presp  = -1;
							nsc_preec  = -1;
							nsc_logo_l = -1;
							nsc_logo_r = -1;
						}
					}
				}
				//--- 境界部分だった場合の補正 ---
				if (arstat_tmp1 == D_SCINT_B_OTHER){
					if (frm_tmp1 < frm_cur - frm_spc){			// 境界位置が現位置よりも手前
						if (arstat_tmp2 == D_SCINT_L_TRKEEP){	// 予告部分からの延長
							nsc_tr = nsc_tmp1;
						}
					}
					if (frm_tmp1 <= frm_cur + frm_spc){			// 境界位置が現位置と同じか手前
						if (arstat_tmp2 == D_SCINT_L_SP){		// 番組提供部分からの延長
							nsc_sp = nsc_tmp1;
						}
					}
				}
			}
			//--- 前回地点変更 ---
			nsc_tmp2 = nsc_tmp1;
			frm_tmp2 = frm_tmp1;
			arstat_tmp2 = arstat_tmp1;
			inlogo_tmp2 = inlogo_tmp1;
		}
	}
	//--- ロゴ隣接状態検出 ---
	type_logo = 0;							// ロゴ検出なし
	frmdif_tmp1 = CnvTimeFrm(600,0);		// 未検出時の最大フレーム数を設定
	frmdif_tmp2 = CnvTimeFrm(600,0);		// 未検出時の最大フレーム数を設定
//printf("[%d:%d,%d:%d]",nsc_logo_l,(int)plogo->frm_scpos_s[nsc_logo_l],nsc_logo_r,(int)plogo->frm_scpos_s[nsc_logo_r]);
//printf("[%d:%d,%d:%d]",nsc_tr1st,(int)plogo->frm_scpos_s[nsc_tr1st],nsc_tr1stc,nsc_tr);
	if (nsc_logo_l >= 0){					// ロゴ切り替わり前側からの距離
		// ロゴ開始側・終了側を候補から外す時の処理
		if ((prm->c_lgprev == 0 && prm->c_lgpost != 0 && flag_logo_rise == 1) ||
			(prm->c_lgprev != 0 && prm->c_lgpost == 0 && flag_logo_rise == 0)){
		}
		else{
			frmdif_tmp1 = frm_last - plogo->frm_scpos_s[nsc_logo_l];
		}
	}
	if (nsc_logo_r >= 0){					// ロゴ切り替わり後側からの距離
		// ロゴ開始側・終了側を候補から外す時の処理
		if ((prm->c_lgprev != 0 && prm->c_lgpost == 0 && flag_logo_rise == 1) ||
			(prm->c_lgprev == 0 && prm->c_lgpost != 0 && flag_logo_rise == 0)){
		}
		else{
			frmdif_tmp2 = plogo->frm_scpos_s[nsc_logo_r] - frm_cur;
		}
	}
	type_logo_add = 0;
	if (frmdif_tmp1 <= frmdif_tmp2){		// 前側が現位置に近い
		frmdif_tmp = frmdif_tmp1;
		if (flag_logo_rise == 1){			// 開始時ロゴあり
			type_logo_add = 100;
		}
	}
	else{									// 後側が現位置に近い
		frmdif_tmp = frmdif_tmp2;
		if (flag_logo_rise == 0){			// 開始時ロゴなし＝終了側がロゴあり
			type_logo_add = 100;
		}
	}
	frmdifsec_logo = GetSecFromFrm(frmdif_tmp);		// 秒数に変換
	if (flag_logo_rise > 0){		// ロゴ内部
		if (frmdifsec_logo >= 300){
			type_logo = 10;			// ロゴ内部で５分以上離れている
		}
		else if (frmdifsec_logo >= 31){
			type_logo = 12;			// ロゴ内部で31秒以上離れている
		}
		else if (frmdifsec_logo >= 15){
			type_logo = 13;			// ロゴ内部で15秒以上離れている
		}
		else if (frmdifsec_logo > 1){
			type_logo = 15;			// ロゴ内部で近いが隣接ではない
		}
		else{
			type_logo = 16;			// ロゴ内部で隣接
		}
	}
	else{							// ロゴ外部
		if (frmdifsec_logo >= 300){
			type_logo = 0;			// ロゴ外部で５分以上離れている
		}
		else if (frmdifsec_logo >= 15){
			type_logo = 3;			// ロゴ外部で15秒以上離れている
		}
		else if (frmdifsec_logo > 1){
			type_logo = 4;			// ロゴ外部で近いが隣接ではない
		}
		else{
			type_logo = 7;			// ロゴ外部で隣接
		}
	}
	type_logo += type_logo_add;

	//--- 予告からの時間 ---
	if (nsc_tr < 0){					// 予告が見つからない場合
		frmdifsec_tr = 0;
		type_tr = 0;
	}
	else if (nsc_tr1st == nsc_cur){		// 現位置が予告開始に隣接
		frmdifsec_tr = 0;
		type_tr = 2;
	}
	else if (nsc_tr1stc == nsc_cur){	// 現位置が予告開始の最初の位置
		frmdifsec_tr = 0;
		type_tr = 3;
	}
	else if (nsc_tr2nd >= 0 && nsc_trstop >= 0 &&
			 nsc_trstop == nsc_cur){		// 現位置が予告隙間
		frmdifsec_tr = 0;
		type_tr = 4;
	}
	else if (nsc_tr >= nsc_cur){		// 現位置が予告より前
		frmdifsec_tr = GetSecFromFrm(plogo->frm_scpos_s[nsc_pretr] - frm_cur);
		type_tr = 2;
	}
	else{								// 現位置が予告後
		frmdifsec_tr = GetSecFromFrm(frm_last - plogo->frm_scpos_s[nsc_tr]);
		type_tr = 1;
	}
	//--- 番組提供からの時間 ---
	if (nsc_sp < 0 || nsc_presp < 0){
		frmdifsec_sp = 0;
		type_sp = 0;
	}
	else if (nsc_presp < nsc_cur && nsc_sp >= nsc_cur){	// 現位置が番組提供と同一
		frmdifsec_sp = 0;
		type_sp = 3;
	}
	else if (nsc_presp >= nsc_cur){		// 現位置が番組提供手前
		frmdifsec_sp = GetSecFromFrm(plogo->frm_scpos_s[nsc_presp] - frm_cur);
		type_sp = 2;
	}
	else{								// 現位置が番組提供後
		frmdifsec_sp = GetSecFromFrm(frm_last - plogo->frm_scpos_s[nsc_sp]);
		type_sp = 1;
	}
	//--- エンドカード手前の時間 ---
	if (nsc_preec < 0){
		frmdifsec_ec = 0;
		type_ec = 0;
	}
	else if (nsc_preec >= nsc_cur){		// 現位置がエンドカード手前
		frmdifsec_ec = GetSecFromFrm(plogo->frm_scpos_s[nsc_preec] - frm_cur);
		type_ec = 2;
	}
	else{								// 現位置がエンドカード後
		frmdifsec_ec = 0;				// 時間チェックはしていない
		type_ec = 1;
	}
	//--- 単位間隔 ---
	frmdifsec_sc = GetSecFromFrm(frm_cur - frm_last);

	//--- 最後のロゴからの状態 ---
	if (frm_riseend < 0){
		type_endlogo = 0;
	}
	else if (frm_last < frm_riseend - frm_spc){
		type_endlogo = 2;
	}
	else{
		type_endlogo = 1;
	}

	//--- 返り値代入 ---
	*r_frmdifsec_logo = frmdifsec_logo;
	*r_type_logo      = type_logo;
	*r_frmdifsec_tr   = frmdifsec_tr;
	*r_type_tr        = type_tr;
	*r_frmdifsec_sp   = frmdifsec_sp;
	*r_type_sp        = type_sp;
	*r_frmdifsec_ec   = frmdifsec_ec;
	*r_type_ec        = type_ec;
	*r_frmdifsec_sc   = frmdifsec_sc;
	*r_type_nolast    = type_nolast;
	*r_type_endlogo   = type_endlogo;
}


//---------------------------------------------------------------------
// 追加候補として指定箇所の優先順位を算出
// 出力：
//  返り値  : 入力配置の優先順位
//---------------------------------------------------------------------
int autoadd_sub_priority(
	int arstat_cur,							// 対象の配置状態
	int type_logo, int frmdifsec_logo,
	int type_tr, int frmdifsec_tr,
	int type_sp, int frmdifsec_sp,
	int type_ec, int frmdifsec_ec,
	int type_nolast,
	int type_endlogo,
	int frmdifsec_sc,
	PRMAUTOREC *prm,
	int flag_trailer,						// 1:予告検出有効
	int flag_sponsor,						// 1:番組提供有効
	int flag_notfound_tr					// 1:前回までに予告なしを検出済み
){
	int prior1;
	int prior1bak;
	int type_logo_prior, type_logo_in, type_logo_prev;
	int type_logo_nonbr;
	int invalid_pos;
	int invalid_sp, invalid_tr;
	int valid_lgsp;

	//--- type_logoを分類 ---
	type_logo_prior = type_logo % 10;				// 優先順位
	type_logo_in    = (type_logo / 10) % 10;		// ロゴ内部
	type_logo_prev  = (type_logo / 100) % 10;		// ロゴ手前
	// 両隣も含めロゴなし
	type_logo_nonbr = ((type_logo_in == 0) && (type_logo_prior <= 5))? 1 : 0;

	//--- 予告と番組提供の位置関係から無効箇所を検出 ---
	invalid_pos = 0;
	invalid_sp = 0;
	invalid_tr = 0;
	if (flag_sponsor > 0 && flag_trailer > 0){
		if (prm->c_lgprev == 0){				// 前側を無効化
			if (type_sp == 2 || type_tr == 2){
				invalid_pos = 1;
			}
		}
		if (prm->c_lgpost == 0){				// 後側を無効化
			if (type_sp == 1 || type_tr == 1){
				invalid_pos = 1;
			}
		}
		if (prm->c_lgintr > 0){					// 中間を残す
			if (type_tr == 1 && (type_sp == 2 || type_sp == 0)){
				invalid_pos = 0;
			}
			else if (type_tr == 2 && type_sp == 1){
				invalid_pos = 0;
			}
			else if (type_tr == 4){
				invalid_pos = 0;
			}
		}
		if (invalid_pos == 0){						// 予告・番組提供ともに有効な場合
			if ((type_sp == 1 && type_tr == 1) ||	// 両方後側の場合
				(type_sp == 2 && type_tr == 2)){	// 両方前側の場合
				if (frmdifsec_sp > frmdifsec_tr){	// 離れていたら近い側を残す
					invalid_sp = 1;
				}
				else if (frmdifsec_sp < frmdifsec_tr){	// 離れていたら近い側を残す
					invalid_tr = 1;
				}
			}
		}
	}
	else if (flag_sponsor > 0){
		if (prm->c_lgprev == 0){				// 前側を無効化
			if (type_sp == 2){
				invalid_pos = 1;
			}
		}
		if (prm->c_lgpost == 0){				// 後側を無効化
			if (type_sp == 1){
				invalid_pos = 1;
			}
		}
	}
	else if (flag_trailer > 0){
		if (prm->c_lgprev == 0){				// 前側を無効化
			if (type_tr == 2){
				invalid_pos = 1;
			}
		}
		if (prm->c_lgpost == 0){				// 後側を無効化
			if (type_tr == 1){
				invalid_pos = 1;
			}
		}
	}

	//--- 検索範囲による優先順位設定 ---
	prior1 = 0;
	// ロゴなしのみ検索でロゴあり領域、またはその逆の場合は無効
	if ((prm->c_lgy == 0 && type_logo_in != 0) ||
		(prm->c_lgn == 0 && type_logo_in == 0)){
		prior1 = 0;
	}
	// 両隣も含めてロゴなし検索で条件を満たさない場合は無効
	else if (prm->c_lgbn > 0 && type_logo_nonbr == 0){
		prior1 = 0;
	}
	// 前側検索なしで前側、後側検索なしで後側の場合は無効
	else if ((prm->c_lgprev == 0 && type_logo_prev != 0) ||
			 (prm->c_lgpost == 0 && type_logo_prev == 0)){
		prior1 = 0;
	}
	else{
		// ロゴ切り替わり位置から
		if (type_logo_prior > 0){
			// 探索範囲内
			if (((prm->c_search == 1 || prm->c_search == 4) && (frmdifsec_logo <= 1)) ||
				((prm->c_search == 2 || prm->c_search == 5) &&
				    (frmdifsec_logo >= prm->scopen && frmdifsec_logo <= prm->scope)) ||
				((prm->c_search == 3 || prm->c_search == 6) && (frmdifsec_logo == prm->scope))){
				prior1 = type_logo_prior;
			}
		}
	}
	// 直後が番組提供・エンドカード時のみとする場合に条件を満たさない場合は無条件で無効
	if (prm->c_lgsp != 0 &&
		(flag_sponsor == 0 || type_sp != 2 || frmdifsec_sp != 0) &&
		(type_ec != 2 || frmdifsec_ec != 0)){
		prior1 = 0;
	}
	// 予告・番組提供が範囲内にない、または検索しない場合は現状維持
	else if ((prm->c_search >= 4 && prm->c_search <= 6) ||
		(flag_trailer == 0 && flag_sponsor == 0)){
		// 前後関係のチェックだけは実行
		if (invalid_pos > 0){
			prior1 = 0;
		}
	}
	// ロゴなしのみ検索でロゴあり領域、またはその逆の場合は無効
	else if ((prm->c_lgy == 0 && type_logo_in != 0) ||
			 (prm->c_lgn == 0 && type_logo_in == 0)){
		prior1 = 0;
	}
	// 両隣も含めてロゴなし検索で条件を満たさない場合は無効
	else if (prm->c_lgbn > 0 && type_logo_nonbr == 0){
		prior1 = 0;
	}
	else{
		// 直後に番組提供・エンドカードが条件でロゴからの制約は満たしている場合
		valid_lgsp = 0;
		if ((prm->c_lgsp != 0) && (prior1 > 0) &&
			(((type_sp == 2) && (frmdifsec_sp == 0)) ||
			 ((type_ec == 2) && (frmdifsec_ec == 0)))){
			valid_lgsp = 1;
		}
		// 予告・番組提供がある場合はロゴ端からの優先順位は（ロゴなしで隣接）以外無効にする
		if (prior1 > 0 && prior1 < 7){
			prior1bak = 0;
		}
		else{
			prior1bak = prior1;
		}
		// ロゴ優先順位は外し改めて優先順位を検索
		prior1 = 0;
		// 番組提供がある場合の処理
		if (flag_sponsor > 0){
			if (type_sp == 0){
			}
			// 前側検索なしで前側、後側検索なしで後側の場合は無効
			else if (invalid_pos > 0){
			}
			// 番組提供は無効の場合
			else if (invalid_sp > 0){
			}
			// 探索範囲内
			else if (((prm->c_search == 1) && (frmdifsec_sp <= 1)) ||
					 ((prm->c_search == 2) &&
					      (frmdifsec_sp >= prm->scopen && frmdifsec_sp <= prm->scope)) ||
					 ((prm->c_search == 3) && (frmdifsec_sp == prm->scope)) ||
					 (valid_lgsp > 0)){
				// 番組提供より前
				if (type_sp == 2){
					// 隣接のみ優先順位を上げる
					if (frmdifsec_sp <= 1){
						prior1 += 20;
					}
					// 手前側は隣接に限定する場合
					else if (prm->c_search == 2){
						prior1 = 0;
					}
				}
				else if (type_sp == 1){
					// 番組提供直後に隣接
					if (frmdifsec_sp <= 1){
						prior1 += 130;
					}
					// 番組提供から指定時間以内
					else{
						prior1 += 30;
					}
				}
			}
		}
		if (flag_trailer > 0){
			// 予告が現地点に近くない場合は無効
			if (type_tr == 0){
			}
			// 前側検索なしで前側、後側検索なしで後側の場合は無効
			else if (invalid_pos > 0){
			}
			// 予告は無効の場合
			else if (invalid_tr > 0){
			}
			// 探索範囲内
			else if (((prm->c_search == 1) && (frmdifsec_tr <= 1)) ||
					 ((prm->c_search == 2) && (frmdifsec_tr <= prm->scope)) ||
					 ((prm->c_search == 3) && (frmdifsec_tr == prm->scope))){
				// 予告開始前に隣接
				if (type_tr == 2 && frmdifsec_tr == 0){
					prior1 += 20;
					// ロゴなしだった場合は優先度を上げる
					if (type_logo_in == 0){
						prior1 += 15;
					}
				}
				else if (type_tr == 1){
					// 予告直後に隣接
					if (frmdifsec_tr <= 1){
						prior1 += 30;
						// ロゴなしだった場合は優先度を上げる
						if (type_logo_in == 0){
							prior1 += 15;
						}
					}
					// 予告から指定時間以内
					else{
						prior1 += 20;
					}
				}
				else if (type_tr == 4){		// 予告間隙間
						prior1 += 40;
				}
				// type_tr == 3 は現状維持
			}
		}
		// 有効であれば残したロゴ優先順位も追加
		if (prior1 > 0){
			prior1 += prior1bak;
		}
	}
	//--- 本編内容前のロゴなし領域に予告・エンドカード位置がないか確認 ---
	if (prm->c_chklast > 0 && type_nolast > 0){
		if (type_logo_in == 0){
			prior1 = 0;
		}
	}
	//--- 予告が見つからず、最後のロゴより前で前側は候補を外す場合の確認 ---
	if (flag_notfound_tr > 0 && type_endlogo == 2 && prm->c_lgprev == 0){
		prior1 = 0;
	}
	//--- 現状態による選択 ---
	if (prior1 > 0){
		if (arstat_cur == D_SCINT_L_TRKEEP){
			// 予告と認識した最初の内容を番組提供部分とする場合
			if (type_tr == 3 && prm->c_in1 > 0){
				if (prm->c_in1 == 2){		// 優先度上げる
					prior1 += 500;
				}
				if (prm->c_in1 == 1){		// 優先度上げる
					prior1 += 50;
				}
				else{						// 優先度最小
					prior1 = 1;
				}
			}
			else{
				prior1 = 0;
			}
		}
		else if (arstat_cur == D_SCINT_L_EC ||
				 arstat_cur == D_SCINT_L_SP){
				prior1 = 0;
		}
		else if (arstat_cur == D_SCINT_L_LGCUT ||
				 arstat_cur == D_SCINT_L_LGADD ||
				 arstat_cur == D_SCINT_N_LGCUT ||
				 arstat_cur == D_SCINT_N_LGADD){
				prior1 = 0;
		}
		else if ((arstat_cur == D_SCINT_N_AUNIT) ||
				 (arstat_cur == D_SCINT_N_BUNIT)){
			// 合併１５秒単位CMの無効化処理
			if (prm->c_unitcmoff > 0){			// 合併１５秒単位CMは強制的にCM以外とする
			}
			else if (prm->c_unitcmon > 0){		// 合併１５秒単位CMは強制的にCMとする
				prior1 = 0;
			}
			else if (type_tr != 1){				// 予告後の認識期間以外
				if (flag_notfound_tr == 0){		// 特に探してなければ無効でCMとする
					prior1 = 0;
				}
				else if (flag_notfound_tr > 0){	// 特に探している時は優先度最低で保持
					prior1 = 1;
				}
			}
		}
		else if (arstat_cur == D_SCINT_B_UNIT ||
				 arstat_cur == D_SCINT_B_OTHER){
			prior1 += 200;
		}
	}
	return prior1;
}


//---------------------------------------------------------------------
// 次の構成までの時間条件に合致するかチェック
// 出力：
//  返り値  : 0=指定秒の構成なし 1=指定秒の構成あり
//---------------------------------------------------------------------
int autoadd_sub_chksec(LOGO_RESULTREC *plogo, int nsc_cur, int difsec){
	int ret;
	int step;
	int flag_end;
	int frmdifsec_tmp;
	int frm_cur, frm_next;
	int nsc_max, nsc_min, nsc_next;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	ret = 0;
	if (difsec < 0){
		step = -1;
	}
	else{
		step = 1;
	}
	nsc_max = plogo->num_scpos - prm_noedge;
	nsc_min = prm_noedge;
	nsc_next = nsc_cur + step;
	flag_end = 0;
	if (nsc_next >= nsc_min && nsc_next < nsc_max && step != 0){
		while(nsc_next >= nsc_min && nsc_next < nsc_max &&
			  flag_end == 0){
			if (plogo->stat_scpos[nsc_next] == 2){
				frm_cur  = plogo->frm_scpos_s[nsc_cur];
				frm_next = plogo->frm_scpos_s[nsc_next];
				frmdifsec_tmp = GetSecFromFrm(frm_next - frm_cur);
				if (frmdifsec_tmp == difsec){
					ret = 1;
				}
				if (frmdifsec_tmp >= difsec){
					flag_end = 1;
				}
			}
			nsc_next += step;
		}
	}
	return ret;
}


//---------------------------------------------------------------------
// AutoAdd用の内部パラメータを設定
// 出力：
//  prm     : 内部パラメータ情報
//---------------------------------------------------------------------
void autoadd_sub_paramset(LOGO_RESULTREC *plogo, PRMAUTOREC *prm, const CMDSETREC *cmdset, int cmdtype){
	int scope_tmp, period_tmp, maxprd_tmp;
	int c_wtmp, c_seatmp, c_lgtmp, c_ptmp, c_limtmp, c_unittmp;
	int default_c_wmin, default_c_wmax;
	int default_scope, default_period, default_search;
	int enable_w15, enable_in1;

	//--- コマンドによる違い部分 ---
	if (cmdtype == D_CMDAUTO_SP){			// 番組提供
		//--- デフォルト値 ---
		default_c_wmin = 6;					// 最小期間秒数
		default_c_wmax = 14;				// 最大期間秒数
		default_scope  = 90;				// 検索範囲秒数
		default_period = 5;					// 設定期間秒数
		default_search = 1;					// 検索範囲設定
		//--- フラグ設定 ---
		enable_w15     = 1;					// 15秒の検索
		enable_in1     = 1;					// 予告に挿入
	}
	else if (cmdtype == D_CMDAUTO_EC){		// エンドカード
		//--- デフォルト値 ---
		default_c_wmin = 1;					// 最小期間秒数
		default_c_wmax = 14;				// 最大期間秒数
		default_scope  = 90;				// 検索範囲秒数
		default_period = 5;					// 設定期間秒数
		default_search = 1;					// 検索範囲設定
		//--- フラグ設定 ---
		enable_w15     = 0;					// 15秒の検索
		enable_in1     = 0;					// 予告に挿入
	}
	else{
		//--- デフォルト値 ---
		default_c_wmin = 1;					// 最小期間秒数
		default_c_wmax = 14;				// 最大期間秒数
		default_scope  = 90;				// 検索範囲秒数
		default_period = 5;					// 設定期間秒数
		default_search = 1;					// 検索範囲設定
		//--- フラグ設定 ---
		enable_w15     = 0;					// 15秒の検索
		enable_in1     = 0;					// 予告に挿入
	}

	//--- パラメータ取得 ---
	c_wtmp       = cmdset->autop_code % 10;				// 検索秒数決定用中間値
	c_seatmp     = (cmdset->autop_code / 10) % 10;		// 検索範囲
	c_lgtmp      = (cmdset->autop_code / 100) % 10;		// ロゴ状態検索
	c_ptmp       = (cmdset->autop_code / 1000) % 10;
	c_limtmp     = (cmdset->autop_code / 10000) % 10;
	c_unittmp    = (cmdset->autop_code / 100000) % 10;
	period_tmp   = cmdset->autop_period;
	maxprd_tmp   = cmdset->autop_maxprd;
	scope_tmp    = cmdset->autop_scope;
	prm->scopen  = cmdset->autop_scopen;
	prm->limit   = cmdset->autop_limit;
	prm->secnext = cmdset->autop_secnext;
	prm->secprev = cmdset->autop_secprev;
	prm->trsumprd = cmdset->autop_trsumprd;

	//--- 実行判断 ---
	prm->c_exe = (c_wtmp != 0)? 1 : 0;
	//--- デフォルト値付き設定 ---
	prm->scope    = (scope_tmp  == 0)? default_scope  : scope_tmp;
	prm->period   = (period_tmp == 0)? default_period : period_tmp;
	prm->maxprd   = (maxprd_tmp == 0)? default_c_wmax : maxprd_tmp;
	prm->c_search = (c_seatmp   == 0)? default_search : c_seatmp;
	//--- ロゴ制約 ---
	prm->c_lgy   = ((c_lgtmp & 0x3) != 1)? 1 : 0;	// ロゴ付き検索
	prm->c_lgn   = ((c_lgtmp & 0x3) != 2)? 1 : 0;	// ロゴなし検索
	prm->c_lgbn  = ((c_lgtmp & 0x3) == 3)? 1 : 0;	// 両隣を含めロゴなし検索
	prm->c_cutskip = ((c_lgtmp & 0x4) != 0)? 1 : 0;	// 予告カット以降も有効
	//--- 位置制限 ---
	prm->c_lgprev = (c_ptmp == 1 || c_ptmp == 3 || (enable_in1 > 0 && c_ptmp >= 4))? 0 : 1;	// 前側を対象外
	prm->c_lgpost = (c_ptmp == 2 || c_ptmp == 3 || (enable_in1 > 0 && c_ptmp >= 4))? 0 : 1;	// 後側を対象外
	prm->c_lgintr = (c_ptmp == 3)? 1 : 0;	// 間を残す
	prm->c_lgsp   = (c_ptmp == 4 && enable_in1 == 0)? 1 : 0;	// 番組提供・エンドカードが直後にある場合のみ対象
	prm->c_limloc    = ((c_limtmp & 0x1) > 0)? 1 : 0;
	prm->c_limtrsum  = ((c_limtmp & 0x2) > 0)? 1 : 0;
	prm->c_unitcmoff = ((c_unittmp & 0x1) > 0)? 1 : 0;
	prm->c_unitcmon  = ((c_unittmp & 0x2) > 0)? 1 : 0;
	prm->c_wdefmin = default_c_wmin;
	prm->c_wdefmax = default_c_wmax;

	//--- 15秒特殊設定（trailerでカットされた所も検索に入れる） ---
	if (enable_w15 > 0){
		prm->c_w15  = ((c_wtmp == 6) ||
					  ((c_wtmp == 3) && (prm->period % 15 == 0)))? 1 : 0;
	}
	else{
		prm->c_w15 = 0;
	}
	//--- 予告部分最初に番組提供設定、本編前のエンドカード等無効化 ---
	if (enable_in1 > 0){
		prm->c_in1     = (c_ptmp >= 4)? c_ptmp - 3 : 0;
		prm->c_chklast = 0;
	}
	else{
		prm->c_in1     = 0;
		prm->c_chklast = 1;
	}

	//--- 検索下限秒数 ---
	if (c_wtmp == 2 || c_wtmp == 4 || c_wtmp == 5){
		prm->c_wmin = prm->period;
	}
	else if (c_wtmp == 6 && enable_w15){	// 15秒限定
		prm->c_wmin = 15;
	}
	else{
		prm->c_wmin = default_c_wmin;		// 標準設定
	}
	//--- 検索上限秒数 ---
	if (c_wtmp == 2 || c_wtmp == 3){
		prm->c_wmax = prm->period;
	}
	else if (c_wtmp == 5){
		prm->c_wmax = prm->maxprd;
	}
	else if (c_wtmp == 6 && enable_w15){	// 15秒限定
		prm->c_wmax = 15;
	}
	else{
		prm->c_wmax = default_c_wmax;		// 標準設定
	}
}


//---------------------------------------------------------------------
// 既存構成を検索
// 出力：
//  返り値  : 検出した目的配置の合計
//  r_flag_trailer : 予告有無確認 0=なし 1=あり
//  r_flag_sponsor : 番組提供有無確認 0=なし 1=あり
//  r_totalsec_tr  : 予告認識秒数
//---------------------------------------------------------------------
int autoadd_sub_cntlimit(
	int *r_flag_trailer,
	int *r_flag_sponsor,
	int *r_totalsec_tr,
	LOGO_RESULTREC *plogo,
	int cmdtype,
	int frm_cut_s, int frm_cut_e
){
	int cnt_limit;
	int i;
	int flag_trailer, flag_sponsor;
	int nsc_last;
	int frm_last, frm_cur;
	int arstat_cur, arstat_dst;
	int frm_spc;
	int totalsec_tr, tmpsec;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	frm_spc = plogo->frm_spc;
	//--- コマンドに対応する構成取得 ---
	if (cmdtype == D_CMDAUTO_SP){			// 番組提供
		arstat_dst = D_SCINT_L_SP;
	}
	else if (cmdtype == D_CMDAUTO_EC){		// エンドカード
		arstat_dst = D_SCINT_L_EC;
	}
	else{
		arstat_dst = D_SCINT_L_TRKEEP;
	}

	//--- limit確認、予告・番組提供有無確認 ---
	cnt_limit = 0;
	flag_trailer = 0;
	flag_sponsor = 0;
	totalsec_tr = 0;
	nsc_last = 0;
	frm_last = 0;
	if (prm_noedge > 0){
		nsc_last = -1;
	}
	for(i=1; i<plogo->num_scpos - prm_noedge; i++){
//		if (plogo->stat_scpos[i] >= 2 || plogo->nchk_scpos_1st == i){
		if (plogo->stat_scpos[i] >= 2){
			if (nsc_last >= 0){
				frm_cur  = plogo->frm_scpos_s[i];
				arstat_cur = plogo->arstat_sc_e[i];
				if (frm_last >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc){
					if (arstat_cur == arstat_dst){			// 目的存在
						cnt_limit ++;
					}
					if (arstat_cur == D_SCINT_L_TRKEEP){	// 予告存在
						flag_trailer = 1;
						// 予告秒数取得
						tmpsec = GetSecFromFrm(frm_cur - frm_last);
						totalsec_tr += tmpsec;
					}
					if (arstat_cur == D_SCINT_L_SP){		// 番組提供存在
						flag_sponsor = 1;
					}
				}
			}
			nsc_last = i;
			frm_last = frm_cur;
		}
	}
	*r_flag_trailer = flag_trailer;
	*r_flag_sponsor = flag_sponsor;
	*r_totalsec_tr  = totalsec_tr;

	return cnt_limit;
}


//---------------------------------------------------------------------
// 追加位置より前の予告等カットは無効化する処理
// 出力：
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
void autoadd_sub_cancelcut(LOGO_RESULTREC *plogo, PRMAUTOREC *prm,
	int nsc_prior,
	int frm_cut_s, int frm_cut_e
){
	int i;
	int nsc_last;
	int frm_last, frm_cur;
	int arstat_cur;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	if (nsc_prior > 0){
		nsc_last = -1;
		frm_last = 0;
		//--- 追加位置までの検索 ---
		for(i=1; i < nsc_prior; i++){
			frm_cur  = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
			//--- 範囲内の場合 ---
			if (plogo->stat_scpos[i] >= 2){
				if (frm_last >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc && nsc_last >= 0){
					//--- カット処理を無効化 ---
					if (arstat_cur == D_SCINT_L_TRCUT){
						plogo->arstat_sc_e[i] = D_SCINT_L_TRKEEP;
					}
					else if (arstat_cur == D_SCINT_L_ECCUT){
						plogo->arstat_sc_e[i] = D_SCINT_L_TRRAW;
					}
				}
				nsc_last = i;
				frm_last = frm_cur;
			}
		}
	}
}


//---------------------------------------------------------------------
// 合併１５秒単位CMの一部を解除する場合の処理
// 出力：
//  plogo->arstat_sc_e[] : 合併１５秒単位CM情報を書き換え
//---------------------------------------------------------------------
void autoadd_sub_revunitcm(LOGO_RESULTREC *plogo,
	int nsc_target
){
	int i;
	int arstat_target, arstat_cur;
	int arstat_new;
	int type_cont;
	int nsc_1st, nsc_2nd;
	int frm_cur, frm_prev;
	int frmdifsec;

	if (nsc_target <= 0 || nsc_target >= plogo->num_scpos){
		return;
	}
	arstat_target = plogo->arstat_sc_e[nsc_target];

	// 合併１５秒単位CM中間位置だった場合は手前側を解除
	if (arstat_target == D_SCINT_N_AUNIT && nsc_target > 1){
		// 手前側解除後の状態を設定
		arstat_new = D_SCINT_N_OTHER;
		// 手前が予告かチェック
		i = nsc_target - 1;
		arstat_cur = plogo->arstat_sc_e[i];
		while((plogo->stat_scpos[i] < 2 || arstat_cur == D_SCINT_N_AUNIT) && i > 0){
			i --;
			arstat_cur = plogo->arstat_sc_e[i];
		}
		if (arstat_cur == D_SCINT_N_BUNIT){			// 先頭位置の場合
			// 手前の構成取得
			i --;
			while((plogo->stat_scpos[i] < 2) && i > 0){
				i --;
			}
			if (i > 0){
				// 手前が予告の場合は予告状態を継続させる設定
				arstat_cur = plogo->arstat_sc_e[i];
				if ((arstat_cur == D_SCINT_L_TRKEEP) ||
					(arstat_cur == D_SCINT_L_TRRAW )){
					arstat_new = D_SCINT_L_TRRAW;		// 手前側解除後の状態を変更
				}
			}
		}
		// 解除処理
		i = nsc_target - 1;
		arstat_cur = plogo->arstat_sc_e[i];
		while((plogo->stat_scpos[i] < 2 || arstat_cur == D_SCINT_N_AUNIT) && i > 0){
			if (arstat_cur == D_SCINT_N_AUNIT){		// 中間位置を解除
				plogo->arstat_sc_e[i] = arstat_new;
			}
			i --;
			arstat_cur = plogo->arstat_sc_e[i];
		}
		if (arstat_cur == D_SCINT_N_BUNIT){			// 先頭位置を解除
			plogo->arstat_sc_e[i] = arstat_new;
		}
	}
	// 合併１５秒単位CMだった場合の後半部分処理
	if ((arstat_target == D_SCINT_N_AUNIT || arstat_target == D_SCINT_N_BUNIT) &&
		nsc_target < plogo->num_scpos - 1){
		i = nsc_target;
		type_cont = 0;
		while(type_cont >= 0 && type_cont <= 3 && i < plogo->num_scpos){
			if (plogo->stat_scpos[i] >= 2){
				arstat_cur = plogo->arstat_sc_e[i];
				if (arstat_cur == D_SCINT_N_AUNIT ||
					arstat_cur == D_SCINT_N_BUNIT ||
					arstat_cur == D_SCINT_N_OTHER){
					frm_cur    = plogo->frm_scpos_s[i];
					if (type_cont > 1){		// ２回目以降の場合
						frmdifsec  = GetSecFromFrm(frm_cur - frm_prev);
						// 新たに合併１５秒単位となる場合は設定
						if ((frmdifsec % 15) == 0 && frmdifsec <= 30){
							plogo->arstat_sc_e[nsc_1st] = D_SCINT_N_BUNIT;
							plogo->arstat_sc_e[i]       = D_SCINT_N_AUNIT;
							if (type_cont > 2){
								plogo->arstat_sc_e[nsc_2nd] = D_SCINT_N_AUNIT;
							}
							type_cont = 0;
						}
					}
					if (type_cont == 0){
						frm_prev   = frm_cur;
					}
					else if (type_cont == 1){
						nsc_1st    = i;
					}
					else if (type_cont == 2){
						nsc_2nd    = i;
					}
					type_cont ++;
				}
				else{		// 候補ではない構成の場合は終了
					if (type_cont > 2){
						arstat_cur = plogo->arstat_sc_e[nsc_2nd];
						if (arstat_cur == D_SCINT_N_AUNIT ||
							arstat_cur == D_SCINT_N_BUNIT){
							plogo->arstat_sc_e[nsc_2nd] = D_SCINT_N_OTHER;
						}
					}
					if (type_cont > 1){
						arstat_cur = plogo->arstat_sc_e[nsc_1st];
						if (arstat_cur == D_SCINT_N_AUNIT ||
							arstat_cur == D_SCINT_N_BUNIT){
							plogo->arstat_sc_e[nsc_1st] = D_SCINT_N_OTHER;
						}
					}
					type_cont = -1;
				}
			}
			i ++;
		}
	}
}


//---------------------------------------------------------------------
// 優先順位最大の位置を検索
// limitない時は書き換えも実行
// 出力：
//  返り値  : 優先順位最大のシーンチェンジ番号
//  plogo->arstat_sc_e[] : limit(prm->limit)ない時は配置状態を変更
//---------------------------------------------------------------------
int autoadd_sub_search(LOGO_RESULTREC *plogo, PRMAUTOREC *prm,
	int cmdtype,
	int flag_trailer, int flag_sponsor,
	int frm_cut_s, int frm_cut_e
){
	int i;
	int nsc_prior;
	int nsc_sub_prior;
	int total_prior, prior1;
	int total_sub_prior, sub_prior1;
	int flag_notfound_tr;
	int flag_end;
	int nsc_last;
	int frm_last, frm_cur;
	int arstat_cur, arstat_dst;
	int type_logo, frmdifsec_logo;
	int type_tr, frmdifsec_tr;
	int type_sp, frmdifsec_sp;
	int type_ec, frmdifsec_ec;
	int type_nolast;
	int type_endlogo;
	int frmdifsec_sc;
	int frm_spc;
	int prm_noedge = (plogo->prmvar.flag_edge == 1)? 0 : 1;

	frm_spc = plogo->frm_spc;

	//--- コマンドに対応する構成取得 ---
	if (cmdtype == D_CMDAUTO_SP){			// 番組提供
		arstat_dst = D_SCINT_L_SP;
	}
	else if (cmdtype == D_CMDAUTO_EC){		// エンドカード
		arstat_dst = D_SCINT_L_EC;
	}
	else{
		arstat_dst = D_SCINT_L_TRKEEP;
	}

	//--- 予告検出なしの検出後は番組提供について合併15秒構成を分解して認識させる ---
	flag_notfound_tr = 0;
	if (plogo->frm_notfound_tr == frm_cut_s){
		if (cmdtype == D_CMDAUTO_SP){			// 番組提供
			flag_notfound_tr = 1;
		}
	}
	if (flag_trailer == 0 && cmdtype == D_CMDAUTO_TR){	// 予告なし予告検出時も同様
		flag_notfound_tr = 1;
	}

	//--- 各シーンチェンジ位置確認 ---
	flag_end = 0;
	nsc_prior = -1;
	total_prior = 0;
	prior1 = 0;
	nsc_sub_prior = -1;
	total_sub_prior = 0;
	sub_prior1 = 0;
	i = 0;
	nsc_last = 0;
	frm_last = 0;
	if (prm_noedge > 0){
		nsc_last = -1;
	}
	while(++i < plogo->num_scpos - prm_noedge && flag_end == 0){
		frm_cur  = plogo->frm_scpos_s[i];
		arstat_cur = plogo->arstat_sc_e[i];
		//--- 範囲内の場合 ---
//		if (plogo->stat_scpos[i] >= 2 || plogo->nchk_scpos_1st == i){
		if (plogo->stat_scpos[i] >= 2){
			if (frm_last >= frm_cut_s - frm_spc && frm_cur <= frm_cut_e + frm_spc && nsc_last >= 0){
				//--- 指定位置の前後状態を取得 ---
				autoadd_sub_locinfo(&type_logo, &frmdifsec_logo,
								&type_tr, &frmdifsec_tr,
								&type_sp, &frmdifsec_sp,
								&type_ec, &frmdifsec_ec,
								&type_nolast,
								&type_endlogo,
								&frmdifsec_sc,
								plogo, prm, i, nsc_last,
								frm_cut_s, frm_cut_e);
//if (cmdtype == D_CMDAUTO_SP){
//printf("(%d,%d,%d:(%d,%d),(%d,%d),(%d,%d),%d)", i,frm_cur,arstat_cur,
//	type_logo,frmdifsec_logo,type_tr,frmdifsec_tr,type_sp,frmdifsec_sp,frmdifsec_sc);
//}
				//--- 状態からの優先順位設定 ---
				prior1 = autoadd_sub_priority(
							arstat_cur,
							type_logo, frmdifsec_logo,
							type_tr, frmdifsec_tr,
							type_sp, frmdifsec_sp,
							type_ec, frmdifsec_ec,
							type_nolast,
							type_endlogo,
							frmdifsec_sc,
							prm,
							flag_trailer, flag_sponsor, flag_notfound_tr);
				//--- 次の構成時間に制約ある場合のチェック ---
				if (prm->secnext > 0){
					if (autoadd_sub_chksec(plogo, i, prm->secnext) == 0){
						prior1 = 0;
					}
				}
				//--- 前の構成時間に制約ある場合のチェック ---
				if (prm->secprev > 0){
					if (autoadd_sub_chksec(plogo, nsc_last, -1 * prm->secprev) == 0){
						prior1 = 0;
					}
				}
				//--- 候補限定用 ---
				sub_prior1 = prior1;
				//--- 内容期間による選択 ---
				if ((frmdifsec_sc >= prm->c_wmin && frmdifsec_sc <= prm->c_wmax) ||
					(frmdifsec_sc == 15 && prm->c_w15 > 0)){	//指定範囲内
				}
				else if (frmdifsec_sc >= prm->c_wdefmin && frmdifsec_sc <= prm->c_wdefmax){
					prior1 = 0;		// 指定範囲外であるが標準範囲内
				}
				else{				// 期間が範囲外
					prior1 = 0;
					sub_prior1 = 0;
				}
				//--- 最優先位置の更新 ---
				if (total_prior < prior1){
					total_prior = prior1;
					nsc_prior = i;
				}
				if (total_sub_prior < sub_prior1){
					total_sub_prior = sub_prior1;
					nsc_sub_prior = i;
				}
				//--- limitない場合は毎回書き換え ---
				if (prior1 > 0 && prm->limit == 0){
					autoadd_sub_revunitcm(plogo, i);	// 合併１５秒単位CM処理
					plogo->arstat_sc_e[i] = arstat_dst;
				}
			}
			nsc_last = i;
			frm_last = frm_cur;
		}
		
	}
	//--- 標準期間の候補位置のみに候補を限定する処理 ---
	if (prm->c_limloc > 0){
		if (nsc_prior >= 0 && nsc_sub_prior != nsc_prior){
			nsc_prior = -1;
		}
	}
	//--- 追加位置より前の予告等カットは無効化する処理 ---
	if (nsc_prior > 0 && prm->c_cutskip > 0){
		autoadd_sub_cancelcut(plogo, prm, nsc_prior, frm_cut_s, frm_cut_e);
	}
//printf("[%d]", nsc_prior);

	return nsc_prior;
}


//---------------------------------------------------------------------
// AutoAdd SP 番組提供追加処理
// 出力：
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
int autoadd_addsp(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int j;
	int cnt_limit;
	int flag_trailer, flag_sponsor;
	int totalsec_tr;
	int arstat_tmp;
	int nsc_prior;
	PRMAUTOREC prmbody;
	PRMAUTOREC *prm;

	prm = &prmbody;
	//--- 内部パラメータ設定 ---
	autoadd_sub_paramset(plogo, prm, cmdset, D_CMDAUTO_SP);

	//--- 実行有無確認 ---
	if (prm->c_exe == 0){
		return 0;
	}

	//--- limit確認、予告・番組提供有無確認 ---
	cnt_limit = autoadd_sub_cntlimit(
					&flag_trailer, &flag_sponsor, &totalsec_tr, plogo,
					D_CMDAUTO_SP,
					frm_cut_s, frm_cut_e);
	//--- limit上限確認、上限以上であれば何もせず終了 ---
	if (prm->limit != 0 && cnt_limit >= prm->limit){
		return 0;
	}
	//--- 予告秒数が指定以上あれば何もせず終了 ---
	if (prm->c_limtrsum > 0 && totalsec_tr >= prm->trsumprd){
		return 0;
	}
	//--- 既存の番組提供情報は検索位置に使わない ---
	flag_sponsor = 0;

//if (1){
//	printf("##SP:%d\n", (int)(cmdset->autop_code));
//}
	//--- 優先順位最大のシーンチェンジ位置確認（prm->limit == 0の時は書き換えも実行） ---
	nsc_prior = autoadd_sub_search(plogo, prm,
					D_CMDAUTO_SP, 
					flag_trailer, flag_sponsor, frm_cut_s, frm_cut_e);
	if (nsc_prior < 0){
		return 0;
	}

	//--- 一番優先順位の高い候補を番組提供とする ---
	if (nsc_prior > 0 && prm->limit > 0){
		//--- 予告と認識した最初の内容を番組提供部分とする場合 ---
		if (plogo->arstat_sc_e[nsc_prior] == D_SCINT_L_TRKEEP){
			j = nsc_prior+1;
			arstat_tmp = plogo->arstat_sc_e[j];
			while((arstat_tmp < D_SCINTR_L_LOW || arstat_tmp > D_SCINTR_L_HIGH ||
					arstat_tmp == D_SCINT_L_TRKEEP) && j < plogo->num_scpos-1){
				j ++;
				arstat_tmp = plogo->arstat_sc_e[j];
				// 検索範囲を超えたら終了
				if (plogo->frm_scpos_s[j] > frm_cut_e){
					j = plogo->num_scpos-1;
				}
			}
			// 候補がある場合は予告認識を１つ後にずらす
			if (j < plogo->num_scpos-1){
				plogo->arstat_sc_e[j] = D_SCINT_L_TRKEEP;
			}
			// 候補がない場合は強制設定時以外は中止
			else if (prm->c_in1 < 2){
				nsc_prior = -1;
			}
		}
		//--- 番組提供部分を設定 ---
		if (nsc_prior > 0){
			// 候補位置がカット状態だった場合は次以降のカット位置を確認
			if (plogo->arstat_sc_e[nsc_prior] == D_SCINT_L_TRCUT){
				j = nsc_prior + 1;
				arstat_tmp = plogo->arstat_sc_e[j];
				while((plogo->stat_scpos[j] < 2 || arstat_tmp == D_SCINT_L_ECCUT) &&
						j < plogo->num_scpos-1){
					// エンドカードカットだった場合は判断前に戻す
					if (arstat_tmp == D_SCINT_L_ECCUT){
						plogo->arstat_sc_e[j] = D_SCINT_L_TRRAW;
					}
					j ++;
					arstat_tmp = plogo->arstat_sc_e[j];
				}
			}
			autoadd_sub_revunitcm(plogo, nsc_prior);	// 合併１５秒単位CM処理
			plogo->arstat_sc_e[nsc_prior] = D_SCINT_L_SP;
		}
	}
	return 1;
}


//---------------------------------------------------------------------
// AutoAdd EC エンドカード追加処理
// 出力：
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
int autoadd_addec(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int cnt_limit;
	int flag_trailer, flag_sponsor;
	int totalsec_tr;
	int nsc_prior;
	PRMAUTOREC prmbody;
	PRMAUTOREC *prm;

	prm = &prmbody;
	//--- 内部パラメータ設定 ---
	autoadd_sub_paramset(plogo, prm, cmdset, D_CMDAUTO_EC);

	//--- 実行有無確認 ---
	if (prm->c_exe == 0){
		return 0;
	}

	//--- limit確認、予告・番組提供有無確認 ---
	cnt_limit = autoadd_sub_cntlimit(
					&flag_trailer, &flag_sponsor, &totalsec_tr,
					plogo, D_CMDAUTO_EC, frm_cut_s, frm_cut_e);
	//--- limit上限確認、上限以上であれば何もせず終了 ---
	if (prm->limit != 0 && cnt_limit >= prm->limit){
		return 0;
	}
	//--- 予告秒数が指定以上あれば何もせず終了 ---
	if (prm->c_limtrsum > 0 && totalsec_tr >= prm->trsumprd){
		return 0;
	}

	//--- 優先順位最大のシーンチェンジ位置確認（prm->limit == 0の時は書き換えも実行） ---
	nsc_prior = autoadd_sub_search(plogo, prm,
					D_CMDAUTO_EC, 
					flag_trailer, flag_sponsor, frm_cut_s, frm_cut_e);
	if (nsc_prior < 0){
		return 0;
	}

	//--- 一番優先順位の高い候補をエンドカードとする ---
	if (nsc_prior > 0 && prm->limit > 0){
		// エンドカードを設定
		autoadd_sub_revunitcm(plogo, nsc_prior);	// 合併１５秒単位CM処理
		plogo->arstat_sc_e[nsc_prior] = D_SCINT_L_EC;
	}
	return 1;
}


//---------------------------------------------------------------------
// AutoAdd TR 予告追加処理
// 出力：
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
int autoadd_addtr(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int cnt_limit;
	int flag_trailer, flag_sponsor;
	int totalsec_tr;
	int nsc_prior;
	PRMAUTOREC prmbody;
	PRMAUTOREC *prm;

	prm = &prmbody;
	//--- 内部パラメータ設定 ---
	autoadd_sub_paramset(plogo, prm, cmdset, D_CMDAUTO_TR);

	//--- 実行有無確認 ---
	if (prm->c_exe == 0){
		return 0;
	}

	//--- limit確認、予告・番組提供有無確認 ---
	cnt_limit = autoadd_sub_cntlimit(
					&flag_trailer, &flag_sponsor, &totalsec_tr,
					plogo, D_CMDAUTO_TR, frm_cut_s, frm_cut_e);
	//--- limit上限確認、上限以上であれば何もせず終了 ---
	if (prm->limit != 0 && cnt_limit >= prm->limit){
		return 0;
	}
	//--- 予告秒数が指定以上あれば何もせず終了 ---
	if (prm->c_limtrsum > 0 && totalsec_tr >= prm->trsumprd){
		return 0;
	}

	//--- 優先順位最大のシーンチェンジ位置確認（prm->limit == 0の時は書き換えも実行） ---
	nsc_prior = autoadd_sub_search(plogo, prm,
					D_CMDAUTO_TR, 
					flag_trailer, flag_sponsor, frm_cut_s, frm_cut_e);
	if (nsc_prior < 0){
		return 0;
	}

	//--- 一番優先順位の高い候補を予告とする ---
	if (nsc_prior > 0 && prm->limit > 0){
		// 予告を設定
		autoadd_sub_revunitcm(plogo, nsc_prior);	// 合併１５秒単位CM処理
		plogo->arstat_sc_e[nsc_prior] = D_SCINT_L_TRKEEP;
	}
	//--- 予告検出なしの検出 ---
	if (nsc_prior <= 0 && prm->limit > 0){
		plogo->frm_notfound_tr = frm_cut_s;
	}
	return 1;
}


//---------------------------------------------------------------------
// ロゴ端部分のカット・追加実行部分
// 出力：
//  返り値  : 動作実行 0=未実行 1=実行
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
int autoedge_cutadd_exec(LOGO_RESULTREC *plogo,
	int exetype,		// 1:ロゴ開始前  2:ロゴ開始後  3:ロゴ終了前  4:ロゴ終了後
	int nsc_logo,		// ロゴに対応するシーンチェンジ番号
	int frm_endlogo,	// 検索終了フレーム位置
	int prm_c_search, int prm_scope, int prm_c_wmin, int prm_c_wmax,
	int prm_c_add, int prm_c_allcom
){
	int j, m;
	int prm_noedge;
	int flag1, ncal_sgn1, ncal_sec1, ncal_dis1;
	int flag2, ncal_sgn2, ncal_sec2, ncal_dis2;
	int step, inlogo;
	int flag_end, flag_exec;
	int flag_adapt, flag_ovw;
	int nsc_base, nsc_last, nsc_arstat;
	int frm_base, frm_cur, frm_baselogo;
	int arstat_cur, arstat_add;
	int select_arstat;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	//--- 実行タイプによる分類 ---
	if (exetype == 1 || exetype == 3){		// ロゴ開始前、ロゴ終了前
		step = -1;							// 前方検索
	}
	else{
		step = 1;							// 後方検索
	}
	if (exetype == 2 || exetype == 3){		// ロゴ開始後、ロゴ終了前
		inlogo = 1;							// ロゴ内部
	}
	else{
		inlogo = 0;							// ロゴ外部
	}
	//--- 初期化 ---
	prm_noedge = 1;
	flag_exec = 0;
	flag_end = 0;
	nsc_base = nsc_logo;
	frm_base = plogo->frm_scpos_s[nsc_base];
	frm_baselogo = plogo->frm_scpos_s[nsc_base];
	nsc_last = nsc_logo;
	flag_adapt = (prm_c_search == 2 || prm_c_search == 3)? 0 : 1;		// 既存カット適用後からの時1
	flag_ovw = (prm_c_search == 3)? 1 : 0;
	//--- ロゴ位置から順番に確認 ---
	j = nsc_logo + step;
	while(j >= prm_noedge && j < plogo->num_scpos - prm_noedge && flag_end == 0){
		frm_cur = plogo->frm_scpos_s[j];
		if (plogo->stat_scpos[j] == 2){
			//--- 配置状態取得 ---
			if (step < 0){
				nsc_arstat = nsc_last;
				flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
						frm_cur, frm_base);
				flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
						frm_base, frm_baselogo);
			}
			else{
				nsc_arstat = j;
				flag1 = adjust_calcdif_select(&ncal_sgn1, &ncal_sec1, &ncal_dis1,
						frm_base, frm_cur);
				flag2 = adjust_calcdif_select(&ncal_sgn2, &ncal_sec2, &ncal_dis2,
						frm_baselogo, frm_base);
			}
			arstat_cur = plogo->arstat_sc_e[nsc_arstat];
			select_arstat = 0;
			if (arstat_cur == D_SCINT_N_LGCUT ||
				arstat_cur == D_SCINT_N_LGADD ||
				arstat_cur == D_SCINT_L_LGCUT ||
				arstat_cur == D_SCINT_L_LGADD){
				select_arstat = 1;						// カット（残留）が既に存在
			}
			if (flag_adapt > 0 && select_arstat == 1){
				nsc_base = j;							// 既存カット適用後の位置に基準を移動
				frm_base = plogo->frm_scpos_s[j];
			}
			else{
				if (flag_adapt > 0){
					flag_adapt = 0;					// 基準位置確定
				}
				// カット可能な最大秒数チェック
				if ((ncal_sec2 > prm_scope && flag2 >= 0) || ncal_sec1 > prm_c_wmax){
					flag_end = 1;
				}
				// 既存カットが存在で書き換えしない
				else if (select_arstat > 0 && flag_ovw == 0){
					flag_end = 1;
				}
				// カット適用範囲内の処理（秒数単位の誤差は気にせずflag1=0も許可）
				else if (ncal_sec1 >= prm_c_wmin && ncal_sec1 <= prm_c_wmax && flag1 >= 0){
					flag_exec = 1;
					if (prm_c_allcom == 0){		// 見つかったら終了する場合
						flag_end  = 1;
					}
					if (prm_c_add == 0){
						if (inlogo == 0){
							arstat_add = D_SCINT_N_LGCUT;
						}
						else{
							arstat_add = D_SCINT_L_LGCUT;
						}
					}
					else{
						if (inlogo == 0){
							arstat_add = D_SCINT_N_LGADD;
						}
						else{
							arstat_add = D_SCINT_L_LGADD;
						}
					}
					
					if (step < 0){				// 前側に検索時
						for(m=nsc_base; m>=nsc_arstat; m--){
							if (plogo->stat_scpos[m] == 2){
								plogo->arstat_sc_e[m] =arstat_add;
							}
						}
					}
					else{						// 後側に検索時
						for(m=nsc_base+1; m<=nsc_arstat; m++){
							if (plogo->stat_scpos[m] == 2){
								plogo->arstat_sc_e[m] =arstat_add;
							}
						}
					}
				}
			}
			nsc_last = j;
		}
		// ロゴ全体まで検出したら終了
		if ((step < 0 && frm_cur < frm_endlogo + frm_spc) ||
			(step > 0 && frm_cur > frm_endlogo - frm_spc)){
			flag_end = 1;
		}
		// 次のシーンチェンジ
		j += step;
	}
	return flag_exec;
}


//---------------------------------------------------------------------
// AutoEdge EC ロゴ端部分のカット・追加
// 出力：
//  返り値  : 動作実行 0=未実行 1=実行
//  plogo->arstat_sc_e[] : 配置状態を変更
//---------------------------------------------------------------------
int autoedge_cutadd(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int nedge, int nlogo){
//	int prm_limit;
	int prm_c_search, prm_c_seatmp, prm_c_exe;
	int prm_scope, prm_period, prm_maxprd, prm_scope_tmp, prm_period_tmp, prm_maxprd_tmp;
//	int prm_c_w15
	int prm_c_wmin, prm_c_wmax, prm_c_wtmp;
	int prm_c_cmpart, prm_c_add;
	int prm_c_allcom;
//	int prm_noedge;
//	int cnt_limit;
	int flag_exec;
	int nsc_logo;
	int frm_logo, frm_endlogo;
	int frm_mgn;
	int frm_spc;

	//--- パラメータ取得 ---
//	prm_limit = cmdset->autop_limit;
	prm_scope_tmp  = cmdset->autop_scope;
	prm_period_tmp = cmdset->autop_period;
	prm_maxprd_tmp = cmdset->autop_maxprd;
	prm_c_wtmp = cmdset->autop_code % 10;			// 検索秒数決定用中間値
	prm_c_seatmp = (cmdset->autop_code / 10) % 10;	// 検索範囲
	prm_c_cmpart = ((((cmdset->autop_code / 100) % 10) & 0x1) == 1)? 1 : 0;
	prm_c_add    = ((((cmdset->autop_code / 100) % 10) & 0x2) == 1)? 1 : 0;
	prm_c_allcom = ((((cmdset->autop_code / 1000) % 10) & 0x1) == 1)? 1 : 0;
//	prm_noedge = 1;
	//--- 実行判断 ---
	prm_c_exe = (prm_c_wtmp != 0)? 1 : 0;
	//--- デフォルト値付き設定 ---
	prm_scope  = (prm_scope_tmp == 0)? 90 : prm_scope_tmp;
	prm_period = (prm_period_tmp == 0)?  5  : prm_period_tmp;
	prm_maxprd = (prm_maxprd_tmp == 0)?  10 : prm_maxprd_tmp;
	prm_c_search = (prm_c_seatmp == 0)?  1  : prm_c_seatmp;
	//--- 検索下限秒数 ---
	if (prm_c_wtmp == 2 || prm_c_wtmp == 4 || prm_c_wtmp == 5){
		prm_c_wmin = prm_period;
	}
	else{
		prm_c_wmin = 3;
	}
	//--- 検索上限秒数 ---
	if (prm_c_wtmp == 2 || prm_c_wtmp == 3){
		prm_c_wmax = prm_period;
	}
	else if (prm_c_wtmp == 5){
		prm_c_wmax = prm_maxprd;
	}
	else{
		prm_c_wmax = 10;
	}

	//--- 初期状態設定 ---
	frm_spc = plogo->frm_spc;
	frm_mgn = CnvTimeFrm(6,0);
	//--- 実行有無確認 ---
	flag_exec = 0;
	if (prm_c_exe == 0){
		return 0;
	}

	//--- ロゴに対応するシーンチェンジ番号を取得 ---
	if (nedge == 0){
		frm_logo = plogo->frm_rise[nlogo];
	}
	else{
		frm_logo = plogo->frm_fall[nlogo];
	}
    nsc_logo = auto_getedge_nsc(plogo, frm_logo, nedge, frm_mgn);
    if (nsc_logo < 0){
		nsc_logo = auto_getnsc(plogo, frm_logo, frm_spc);
	}

	//--- ロゴ開始前に追加 ---
	if (nsc_logo >= 0 && nedge == 0 && prm_c_cmpart > 0){
		if (nlogo > 0){
			frm_endlogo = plogo->frm_fall[nlogo-1];
		}
		else{
			frm_endlogo = 0;
		}
		flag_exec = autoedge_cutadd_exec(plogo, 1, nsc_logo, frm_endlogo,
						prm_c_search, prm_scope, prm_c_wmin, prm_c_wmax,
						prm_c_add, prm_c_allcom);
	}
	//--- ロゴ開始直後に追加 ---
	else if (nsc_logo >= 0 && nedge == 0 && prm_c_cmpart == 0){
		frm_endlogo = plogo->frm_fall[nlogo];
		flag_exec = autoedge_cutadd_exec(plogo, 2, nsc_logo, frm_endlogo,
						prm_c_search, prm_scope, prm_c_wmin, prm_c_wmax,
						prm_c_add, prm_c_allcom);
	}
	//--- ロゴ終了直前に追加 ---
	else if (nsc_logo >= 0 && nedge == 1 && prm_c_cmpart == 0){
		frm_endlogo = plogo->frm_rise[nlogo];
		flag_exec = autoedge_cutadd_exec(plogo, 3, nsc_logo, frm_endlogo,
						prm_c_search, prm_scope, prm_c_wmin, prm_c_wmax,
						prm_c_add, prm_c_allcom);
	}
	//--- ロゴ終了直後に追加 ---
	else if (nsc_logo >= 0 && nedge == 1 && prm_c_cmpart > 0){
		if (nlogo < plogo->num_find - 1){
			frm_endlogo = plogo->frm_rise[nlogo+1];
		}
		else{
			frm_endlogo = plogo->frm_totalmax;
		}
		flag_exec = autoedge_cutadd_exec(plogo, 4, nsc_logo, frm_endlogo,
						prm_c_search, prm_scope, prm_c_wmin, prm_c_wmax,
						prm_c_add, prm_c_allcom);
	}
	return flag_exec;
}



//---------------------------------------------------------------------
// ロゴ状態を確認
// 作成したロゴ情報を構成情報に反映
//   uptype : 0=AutoCM用内部ロゴ情報 1=確定ロゴ情報
//---------------------------------------------------------------------
void auto_chklogo(LOGO_RESULTREC *plogo, int uptype){
	int i, n, nst, ned;
	int frm_rise, frm_fall;
	int frm_s, frm_e;
	int tmp_s, tmp_e;
	int sum_frm;
	int flag_1st;
	int nsc_last;
	int arstat_cur;
	int frm_spc;

	frm_spc = plogo->frm_spc;

	nsc_last = 0;
	flag_1st = 1;     // 先頭はロゴ扱いにしない
	for(i=1; i<plogo->num_scpos; i++){
		if (plogo->stat_scpos[i] == 2){
			frm_s = plogo->frm_scpos_s[nsc_last];
			frm_e = plogo->frm_scpos_s[i];
			arstat_cur = plogo->arstat_sc_e[i];
			//--- ロゴ期間を取得 ---
			sum_frm = 0;
			n = 0;
			while(n < plogo->num_find){
				nst = n;
				ned = n;
				//--- 確定状態を見る場合の確定ロゴ位置検索 ---
				if (uptype > 0){
					while(plogo->flag_rise[nst] != 1 && nst < plogo->num_find-1){
						nst ++;
					}
					if (plogo->flag_rise[nst] != 1){
						nst = plogo->num_find;
					}
					if (ned < nst){
						ned = nst;
					}
					while(plogo->flag_rise[ned] != 1 && ned < plogo->num_find-1){
						ned ++;
					}
					if (plogo->flag_rise[ned] != 1){
						ned = plogo->num_find;
					}
				}
				//--- ロゴ位置の検索 ---
				if (nst < plogo->num_find){
					if (uptype > 0){
						frm_rise = plogo->frm_result_rise[nst];
						if (ned < plogo->num_find){
							frm_fall = plogo->frm_result_fall[ned];
						}
						else{
							frm_fall = plogo->frm_totalmax;
						}
					}
					else{
						frm_rise = plogo->frm_rise[nst];
						frm_fall = plogo->frm_fall[ned];
					}
					//--- 範囲内にロゴ表示期間がある場合 ---
					if (frm_rise + frm_spc < frm_e && frm_fall > frm_s + frm_spc){
						if (frm_rise < frm_s){
							tmp_s = frm_s;
						}
						else{
							tmp_s = frm_rise;
						}
						if (frm_fall > frm_e){
							tmp_e = frm_e;
						}
						else{
							tmp_e = frm_fall;
						}
						// ロゴ表示期間を追加
						if (tmp_s < tmp_e){
							sum_frm += (tmp_e - tmp_s + 1);
						}
					}
				}
				n = ned;
				n ++;
			}
			//--- 半分以上の領域がロゴなら構成もロゴ扱い ---
			if (frm_e - frm_s <= sum_frm * 2 && flag_1st == 0){
				if (arstat_cur == D_SCINT_N_UNIT){
					plogo->arstat_sc_e[i] = D_SCINT_L_UNIT;
				}
				else if (arstat_cur == D_SCINT_N_OTHER){
					plogo->arstat_sc_e[i] = D_SCINT_L_OTHER;
				}
				else if (arstat_cur == D_SCINT_N_AUNIT){
					plogo->arstat_sc_e[i] = D_SCINT_L_UNIT;
				}
				else if (arstat_cur == D_SCINT_N_BUNIT){
					plogo->arstat_sc_e[i] = D_SCINT_L_UNIT;
				}
			}
			else if (flag_1st == 0){
				if (arstat_cur == D_SCINT_L_UNIT){
					plogo->arstat_sc_e[i] = D_SCINT_N_UNIT;
				}
				else if (arstat_cur == D_SCINT_L_OTHER){
					plogo->arstat_sc_e[i] = D_SCINT_N_OTHER;
				}
			}
			//--- 最終ロゴチェック ---
			if (i == plogo->num_scpos-1){
				if (frm_e - frm_s <= plogo->prmvar.frm_wcomp_last){
					plogo->arstat_sc_e[plogo->num_scpos-1] = D_SCINT_N_OTHER;
				}
			}
			//--- 完了・次入力用 ---
			flag_1st = 0;
			nsc_last = i;
		}
	}
}


//---------------------------------------------------------------------
// ロゴ情報を再構築
//---------------------------------------------------------------------
void auto_directuplogo_restruct(LOGO_RESULTREC *plogo){
	int ncur, nnew;
	int edge;

	nnew = 0;
	ncur = 0;
	edge = 0;
	while(ncur < plogo->num_find){
		if (edge == 0){							// rise
			if (plogo->flag_rise[ncur] == 1){	// 確定位置
				edge = 1;
				//--- 再構築 ---
				plogo->frm_rise[nnew]   = plogo->frm_rise[ncur];
				plogo->frm_rise_l[nnew] = plogo->frm_rise_l[ncur];
				plogo->frm_rise_r[nnew] = plogo->frm_rise_r[ncur];
				plogo->fade_rise[nnew]  = plogo->fade_rise[ncur];
				plogo->intl_rise[nnew]  = plogo->intl_rise[ncur];
				plogo->flag_rise[nnew]  = plogo->flag_rise[ncur];
				plogo->flag_edge_rise[nnew]    = plogo->flag_edge_rise[ncur];
				plogo->frm_result_rise[nnew]   = plogo->frm_result_rise[ncur];
			}
		}
		if (edge != 0){
			if (plogo->flag_fall[ncur] == 1){	// 確定位置
				edge = 0;
				//--- 再構築 ---
				plogo->frm_fall[nnew]   = plogo->frm_fall[ncur];
				plogo->frm_fall_l[nnew] = plogo->frm_fall_l[ncur];
				plogo->frm_fall_r[nnew] = plogo->frm_fall_r[ncur];
				plogo->fade_fall[nnew]  = plogo->fade_fall[ncur];
				plogo->intl_fall[nnew]  = plogo->intl_fall[ncur];
				plogo->flag_fall[nnew]  = plogo->flag_fall[ncur];
				plogo->flag_edge_fall[nnew]    = plogo->flag_edge_fall[ncur];
				plogo->frm_result_fall[nnew]   = plogo->frm_result_fall[ncur];
				nnew ++;
			}
		}
		ncur ++;
	}
	if (nnew > 0 && nnew < plogo->num_find){
		plogo->num_find = nnew;
	}
}


//---------------------------------------------------------------------
// 確定ロゴ状態をそのまま推測構成に入れる（１箇所分）
// 出力：
//  返り値：対応するロゴ位置
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->*_scpos*         : シーンチェンジ挿入あり
//---------------------------------------------------------------------
int auto_directuplogo_chg(LOGO_RESULTREC *plogo, int frm_src, int frm_difmax, int edge){
	int j;
	int frm_cur, frm_s, frm_e;
	int nsc_dst, nsc_fix;
	int frmdif_dst, frmdif_fix, frmdif_tmp;
	int nsc_ret;

	//--- 一番近い位置を取得 ---
	nsc_dst = -1;		// 全候補
	nsc_fix = -1;		// 確定候補
	frmdif_dst = 0;
	frmdif_fix = 0;
	for(j=1; j<plogo->num_scpos-1; j++){
		if (edge == 0){
			frm_cur = plogo->frm_scpos_s[j];
		}
		else{
			frm_cur = plogo->frm_scpos_e[j];
		}
		// ロゴからの最短距離
		frmdif_tmp = abs(frm_cur - frm_src);
		if (frmdif_tmp <= frm_difmax){
			if (frmdif_tmp < frmdif_dst || nsc_dst < 0){
				nsc_dst = j;
				frmdif_dst = frmdif_tmp;;
			}
			// 番組構成として認識されているシーンチェンジのみ検索
			if (plogo->stat_scpos[j] == 2){
				if (frmdif_tmp < frmdif_fix || nsc_fix < 0){
					nsc_fix = j;
					frmdif_fix = frmdif_tmp;
				}
			}
		}
	}
	if (edge == 0){
		frm_s = frm_src;
		frm_e = frm_src - 1;
	}
	else{
		frm_s = frm_src + 1;
		frm_e = frm_src;
	}
	if (nsc_dst < 0 || frmdif_dst > CnvTimeFrm(0, 500)){	// 近くに無音シーンチェンジがない場合
		if (frm_s < 0){
			nsc_ret = 0;
		}
		else{
			nsc_ret = insert_scpos(plogo, frm_s, frm_e, frm_e, frm_s, 2, 0);
		}
	}
	else{
		nsc_ret = nsc_dst;
		if (frmdif_dst > 0){
			plogo->frm_scpos_s[nsc_dst] = frm_s;
			plogo->frm_scpos_e[nsc_dst] = frm_e;
			plogo->stat_scpos[nsc_dst] = 2;
		}
		if ((nsc_fix >= 0) && (frmdif_fix <= CnvTimeFrm(0, 500)) && (nsc_fix != nsc_dst)){
			plogo->stat_scpos[nsc_fix] = 0;
		}
	}
	return nsc_ret;
}

//---------------------------------------------------------------------
// 確定ロゴ状態をそのまま推測構成に入れる
// 出力：
//  返り値：0:ロゴ未検出で推測なし 1:推測構成変更
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->*_scpos*         : シーンチェンジ挿入あり
//---------------------------------------------------------------------
int auto_directuplogo(LOGO_RESULTREC *plogo){
	int nlg, edge, j;
	int frm_logo_rise, frm_logo_fall;
	int nsc_rise, nsc_fall, nsc_lastfall;
	int arstat_cur;
	int flag_ins;
	int frm_spc;

	//--- 初期状態設定 ---
	frm_spc = plogo->frm_spc;

	//--- 処理開始 ---
	flag_ins = 0;
	nsc_lastfall = 0;
	nlg = 0;
	edge = 0;
	while(nlg < plogo->num_find){
		if (edge == 0){							// rise
			if (plogo->flag_rise[nlg] == 1){	// 確定位置
				//--- ロゴのフレーム位置取得 ---
				frm_logo_rise = plogo->frm_result_rise[nlg];
				//--- 構成位置をロゴ位置に変更 ---
				nsc_rise = auto_directuplogo_chg(plogo, frm_logo_rise, frm_spc, edge);
				if (nsc_rise >= 0){
					edge = 1;
					//--- 確定前のロゴ位置も検出用に変更 ---
					plogo->frm_rise[nlg] = frm_logo_rise;
					if (plogo->frm_rise_l[nlg] > frm_logo_rise){
						plogo->frm_rise_l[nlg] = frm_logo_rise;
					}
					if (plogo->frm_rise_r[nlg] < frm_logo_rise){
						plogo->frm_rise_r[nlg] = frm_logo_rise;
					}
				}
			}
		}
		if (edge != 0){							// fall
			if (plogo->flag_fall[nlg] == 1){	// 確定位置
				//--- ロゴのフレーム位置取得 ---
				frm_logo_fall = plogo->frm_result_fall[nlg];
				//--- 構成位置をロゴ位置に変更 ---
				nsc_fall = auto_directuplogo_chg(plogo, frm_logo_fall, frm_spc, edge);
				if (nsc_fall >= 0){
					edge = 0;
					//--- 確定前のロゴ位置も検出用に変更 ---
					plogo->frm_fall[nlg] = frm_logo_fall;
					if (plogo->frm_fall_l[nlg] > frm_logo_fall){
						plogo->frm_fall_l[nlg] = frm_logo_fall;
					}
					if (plogo->frm_fall_r[nlg] < frm_logo_fall){
						plogo->frm_fall_r[nlg] = frm_logo_fall;
					}
				}
			}
			//--- 位置確定時の処理 ---
			if (edge == 0){
				flag_ins = 1;
				for(j=nsc_lastfall+1; j <= nsc_rise; j++){		// ロゴ手前
					if (plogo->stat_scpos[j] == 2){
						arstat_cur = plogo->arstat_sc_e[j];
						if (arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH){
							plogo->arstat_sc_e[j] = D_SCINT_N_OTHER;
						}
					}
				}
				for(j=nsc_rise+1; j <= nsc_fall; j++){			// ロゴ内
					if (plogo->stat_scpos[j] == 2){
						arstat_cur = plogo->arstat_sc_e[j];
						if (arstat_cur < D_SCINTR_L_LOW || arstat_cur > D_SCINTR_L_HIGH){
							plogo->arstat_sc_e[j] = D_SCINT_L_OTHER;
						}
					}
				}
				nsc_lastfall = nsc_fall;
			}
		}
		nlg ++;
	}
	if (flag_ins > 0 && nsc_lastfall < plogo->num_scpos-1){
		for(j=nsc_lastfall+1; j < plogo->num_scpos; j++){
			if (plogo->stat_scpos[j] == 2){
				arstat_cur = plogo->arstat_sc_e[j];
				if (arstat_cur >= D_SCINTR_L_LOW && arstat_cur <= D_SCINTR_L_HIGH){
					plogo->arstat_sc_e[j] = D_SCINT_N_OTHER;
				}
			}
		}
	}
	auto_directuplogo_restruct(plogo);
	return flag_ins;
}


//---------------------------------------------------------------------
// ロゴ位置を更新した時に推測構成も変更
// 出力：
//  plogo->arstat_sc_e[]    : シーンチェンジ番組構成配置状態を変更
//  plogo->num_find         : ロゴ合計数
//  plogo->frm_rise*        : ロゴ開始位置
//  plogo->frm_fall*        : ロゴ終了位置
//  plogo->stat_scpos[]     : シーンチェンジ候補状態を変更
//  plogo->*_scpos*         : シーンチェンジ挿入あり
//---------------------------------------------------------------------
int autochg_exec(LOGO_RESULTREC *plogo, int nlg, int nedge, int frm_newlogo){
	int i;
	int flag_exec;
	int nsc_baselogo, nsc_ins;
	int frm_new_s, frm_new_e;
	int frm_logo, frm_baselogo;
	int frm_mgn;
	int frm_spc;

	//--- 初期状態設定 ---
	frm_spc = plogo->frm_spc;
	frm_mgn = CnvTimeFrm(6,0);

	flag_exec = 0;

	//--- ロゴ位置に対応するシーンチェンジ位置取得 ---
	if (nedge == 0){
		frm_logo = plogo->frm_rise[nlg];
		frm_new_s = frm_newlogo;
		frm_new_e = frm_newlogo - 1;
	}
	else{
		frm_logo = plogo->frm_fall[nlg];
		frm_new_s = frm_newlogo + 1;
		frm_new_e = frm_newlogo;
	}
	nsc_baselogo = auto_getnsc(plogo, frm_logo, frm_spc);
	if (nsc_baselogo < 0){
		nsc_baselogo = auto_getedge_nsc(plogo, frm_logo, nedge, frm_mgn);
	}
	//--- 変更地点が既存のシーンチェンジか確認 ---
	for(i=0; i<plogo->num_scpos; i++){
		if ((frm_new_s >= plogo->frm_scpos_e[i] - 1) &&
			(frm_new_s <= plogo->frm_scpos_s[i] + 1)){
			frm_new_s = plogo->frm_scpos_s[i];
			frm_new_e = plogo->frm_scpos_e[i];
		}
	}

	//--- 推測情報を変更 ---
	if (nedge == 0){		// ロゴ開始部分
		if (nsc_baselogo > 0){
			frm_baselogo = plogo->frm_scpos_s[nsc_baselogo];
			if (frm_baselogo + frm_spc < frm_newlogo){
				flag_exec = 1;
				nsc_ins = insert_scpos(plogo, frm_new_s, frm_new_e,
							frm_new_s, frm_new_s, 2, 0);
				for(i=nsc_baselogo + 1; i < nsc_ins; i++){
					if (plogo->stat_scpos[i] == 2){
						plogo->stat_scpos[i] = 0;
					}
				}
				plogo->arstat_sc_e[nsc_ins] = D_SCINT_L_LGCUT;
			}
			else if (frm_baselogo - frm_spc > frm_newlogo){
				flag_exec = 1;
				nsc_ins = insert_scpos(plogo, frm_new_s, frm_new_e,
							frm_new_s, frm_new_s, 2, 0);
				for(i=nsc_ins + 1; i <= nsc_baselogo+1; i++){	// 挿入でnsc_baselogoは１ずれる可能性あり
					if (plogo->stat_scpos[i] == 2){
						if (plogo->frm_scpos_s[i] < frm_baselogo){
							plogo->stat_scpos[i] = 0;
						}
						else if (plogo->frm_scpos_s[i] == frm_baselogo){
							plogo->arstat_sc_e[nsc_ins] = D_SCINT_N_LGADD;
						}
					}
				}
			}
		}
	}
	else{					// ロゴ終了部分
		if (nsc_baselogo > 0){
			frm_baselogo = plogo->frm_scpos_e[nsc_baselogo];
			if (frm_baselogo + frm_spc < frm_newlogo){
				flag_exec = 1;
				nsc_ins = insert_scpos(plogo, frm_new_s, frm_new_e,
							frm_new_s, frm_new_s, 2, 0);
				for(i=nsc_baselogo + 1; i < nsc_ins; i++){
					if (plogo->stat_scpos[i] == 2){
						plogo->stat_scpos[i] = 0;
					}
				}
				plogo->arstat_sc_e[nsc_ins] = D_SCINT_N_LGADD;
			}
			else if (frm_baselogo - frm_spc > frm_newlogo){
				flag_exec = 1;
				nsc_ins = insert_scpos(plogo, frm_new_s, frm_new_e,
							frm_new_s, frm_new_s, 2, 0);
				for(i=nsc_ins + 1; i <= nsc_baselogo+1; i++){	// 挿入でnsc_baselogoは１ずれる可能性あり
					if (plogo->stat_scpos[i] == 2){
						if (plogo->frm_scpos_e[i] < frm_baselogo){
							plogo->stat_scpos[i] = 0;
						}
						else if (plogo->frm_scpos_e[i] == frm_baselogo){
							plogo->arstat_sc_e[nsc_ins] = D_SCINT_L_LGCUT;
						}
					}
				}
			}
		}
	}
	return flag_exec;
}


//---------------------------------------------------------------------
// ロゴ位置の推測情報を変更
//---------------------------------------------------------------------
int autochg_start(LOGO_RESULTREC *plogo, int nlg, int nedge, int frm_newlogo){
	int ret;

	if (plogo->flag_autosetup == 0){
		auto_arrange(plogo);
		plogo->flag_autosetup = 1;
	}
	ret = autochg_exec(plogo, nlg, nedge, frm_newlogo);

	return ret;
}

//---------------------------------------------------------------------
// AutoCMコマンド実行
//---------------------------------------------------------------------
int autocm_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int ret;

	ret = 0;
	if (plogo->flag_autosetup == 0){
		auto_arrange(plogo);
		plogo->flag_autosetup = 1;
	}
	if (plogo->flag_nologo == 0){		// ロゴあり時は何もせず終わりにする処理
		ret = 2;
	}
	else if (plogo->flag_autosetup >= 2){	// 推測検出済みの時は何もしない
		ret = 2;
	}
	else{
		ret = autocm_cmdet(plogo, cmdset);
		ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
	}
	return ret;
}


//---------------------------------------------------------------------
// AutoUpコマンド実行
//---------------------------------------------------------------------
int autoup_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset){
	int ret;
	int prm_code;

	ret = 0;
	prm_code = cmdset->autop_code % 10;
	if (prm_code == 1){					// 確定ロゴ情報を推測位置に適用
		if (plogo->flag_autosetup == 0){
			auto_arrange(plogo);
			plogo->flag_autosetup = 1;
		}
		auto_chklogo(plogo, 1);
		plogo->flag_autosetup = 2;
		ret = 1;
	}
	else if (prm_code == 2){			// 確定ロゴ位置をそのまま推測位置に
		ret = auto_directuplogo(plogo);
		if (ret > 0){
			if (plogo->flag_autosetup == 0){
				auto_arrange(plogo);
				ret = auto_directuplogo(plogo);			// 補正されたロゴ位置を元に戻す
			}
			plogo->flag_autosetup = 2;
		}
	}
	ret += 2;			// 各ロゴ実行する必要なく１度だけで終わりにする処理
	return ret;
}


//---------------------------------------------------------------------
// ロゴそれぞれでAutoCut
//---------------------------------------------------------------------
int autocut_start_each(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int ret;
	int nlg, j;
	int frm_dr, frm_df, frm_cur;
	int frm_each_left, frm_each_right;

	ret = 0;
	for(nlg=0; nlg < plogo->num_find; nlg++){
		//--- ロゴ開始検出 ---
		if (plogo->flag_rise[nlg] == 1){
			frm_dr = plogo->frm_result_rise[nlg];
		}
		else{
			frm_dr = plogo->frm_rise[nlg];
		}
		//--- ロゴ終了検出 ---
		if (nlg >= plogo->num_find - 1){
			frm_df = plogo->frm_totalmax;
		}
		else{
			frm_df = plogo->frm_rise[nlg+1];
			if (plogo->flag_rise[nlg+1] == 1){
				if (frm_df > plogo->frm_result_rise[nlg+1]){
					frm_df = plogo->frm_result_rise[nlg+1];
				}
			}
		}
		//--- ロゴに対応するシーンチェンジ位置 ---
		frm_each_left = -1;
		frm_each_right = -1;
		for(j=1; j<plogo->num_scpos-1; j++){
			if (plogo->stat_scpos[j] == 2){
				frm_cur = plogo->frm_scpos_s[j];
				if (frm_cur >= frm_cut_s && frm_cur <= frm_cut_e){
					if (frm_cur >= frm_dr){
						if (frm_each_left < 0){
							frm_each_left = frm_cur;
						}
					}
					if (frm_cur <= frm_df){
						frm_each_right = frm_cur;
					}
				}
			}
		}
		//--- 実行処理 ---
		if (frm_each_left > 0 && frm_each_right > 0 && frm_each_left < frm_each_right){
			if (cmdset->select_auto == D_CMDAUTO_TR){
				ret = autocut_cuttr(plogo, cmdset, frm_each_left, frm_each_right);
				ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
			}
			else if (cmdset->select_auto == D_CMDAUTO_EC){
				ret = autocut_cutec(plogo, cmdset, frm_each_left, frm_each_right);
				ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
			}
		}
	}
	return ret;
}

//---------------------------------------------------------------------
// AutoCutコマンド実行
//---------------------------------------------------------------------
int autocut_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int ret;

	ret = 0;
	if (frm_cut_e < 0){
		frm_cut_e = plogo->frm_totalmax;
	}
	if (cmdset->flag_autoeach == 0){			// 通常
		if (plogo->flag_autosetup == 0){
			auto_arrange(plogo);
			plogo->flag_autosetup = 2;
		}
		else if (plogo->flag_autosetup == 1){	// AutoCMのみ実行時
			auto_chklogo(plogo, 0);
			plogo->flag_autosetup = 2;
		}
		if (cmdset->select_auto == D_CMDAUTO_TR){
			ret = autocut_cuttr(plogo, cmdset, frm_cut_s, frm_cut_e);
			ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
		}
		else if (cmdset->select_auto == D_CMDAUTO_EC){
			ret = autocut_cutec(plogo, cmdset, frm_cut_s, frm_cut_e);
			ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
		}
	}
	else{		// 各ロゴそれぞれで実行する場合
		ret = autocut_start_each(plogo, cmdset, frm_cut_s, frm_cut_e);
	}
	return ret;
}


//---------------------------------------------------------------------
// AutoAddコマンド実行
//---------------------------------------------------------------------
int autoadd_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e){
	int ret;

	ret = 0;
	if (frm_cut_e < 0){
		frm_cut_e = plogo->frm_totalmax;
	}
	if (plogo->flag_autosetup == 0){
		auto_arrange(plogo);
		plogo->flag_autosetup = 2;
	}
	else if (plogo->flag_autosetup == 1){	// AutoCMのみ実行時
		auto_chklogo(plogo, 0);
		plogo->flag_autosetup = 2;
	}
	if (cmdset->select_auto == D_CMDAUTO_SP){
		ret = autoadd_addsp(plogo, cmdset, frm_cut_s, frm_cut_e);
		ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
	}
	else if (cmdset->select_auto == D_CMDAUTO_EC){
		ret = autoadd_addec(plogo, cmdset, frm_cut_s, frm_cut_e);
		ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
	}
	else if (cmdset->select_auto == D_CMDAUTO_TR){
		ret = autoadd_addtr(plogo, cmdset, frm_cut_s, frm_cut_e);
		ret += 2;		// 各ロゴ実行する必要なく１度だけで終わりにする処理
	}
	return ret;
}


//---------------------------------------------------------------------
// AutoEdgeコマンド実行
//---------------------------------------------------------------------
int autoedge_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int nlogo){
	int ret;

	ret = 0;
	if (plogo->flag_autosetup == 0){
		auto_arrange(plogo);
		plogo->flag_autosetup = 2;
	}
	else if (plogo->flag_autosetup == 1){	// AutoCMのみ実行時
		auto_chklogo(plogo, 0);
		plogo->flag_autosetup = 2;
	}
	// ロゴ開始部分
	if (cmdset->select_edge == 0 || cmdset->select_edge == 2){
		ret = autoedge_cutadd(plogo, cmdset, 0, nlogo);
	}
	// ロゴ終了部分
	if (cmdset->select_edge == 1 || cmdset->select_edge == 2){
		ret = autoedge_cutadd(plogo, cmdset, 1, nlogo);
	}
	return ret;
}


