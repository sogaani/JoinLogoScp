//
// join_logo_scp 入力データからの微調整を行う処理
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "join_logo_scp.h"
#include "join_logo_scp_extern.h"


//---------------------------------------------------------------------
// 無音シーンチェンジの候補選別
//---------------------------------------------------------------------
int adjust_select_scpos(LOGO_RESULTREC *plogo){
	int i, j, num_scpos;
	int flag1, flag2;
	int nsec_last;
	int frm_src, frm_dst;
	int ncmp_frm;
	int ncmp_i;
	int ncmp_stat;
	int ntmp_stat;
	int nst, ned;
	int nc_st, nc_nxt;
	int frm_st, frm_ed;
	int dis_dst, dis_ncmp;
	int count_src, count_cmp;
	int ncal_sgn, ncal_sec, ncal_dis;
	int ncal2_sgn, ncal2_sec, ncal2_dis;
	int frm_tmp_st, frm_tmp_ed;
	int frm_tmp_st2, frm_tmp_ed2;
	int ntmp_nxt;
	int flag_nd15;
	int nd5_bsum[SCPOS_FIND_MAX];
	int nd5_sum[SCPOS_FIND_MAX];
	int nd5_nxt[SCPOS_FIND_MAX];
	int nd15_flag[SCPOS_FIND_MAX];


	num_scpos = plogo->num_scpos;

	// ５の倍数秒のシーンチェンジ間隔場所を探索
	for(i=0; i<num_scpos; i++){
		frm_src = plogo->frm_scpos_s[i];
		plogo->stat_scpos[i] = 0;
		nd5_bsum[i] = 0;
		nd5_sum[i] = 0;
		nd5_nxt[i] = -1;
		nd15_flag[i] = 0;
		nsec_last   = 0;
		if (i > 0 && i < num_scpos-1){			// 先頭と最後は除く
			for(j=1; j<num_scpos-1; j++){		// 先頭と最後は除く
				frm_dst = plogo->frm_scpos_s[j];
				if (adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, frm_src, frm_dst) > 0){
					if (ncal_sgn < 0){
						if (ncal_sec % 5 == 0){
							nd5_bsum[i] ++;
						}
						if (ncal_sec % 15 == 0){
							nd15_flag[i] = 1;
						}
					}
					else if (ncal_sgn > 0){
						if (ncal_sec % 5 == 0){
							if (nd5_sum[i] == 0){
								nd5_nxt[i] = j;
							}
							if (nsec_last != ncal_sec){
								nd5_sum[i] ++;
								nsec_last = ncal_sec;
							}
						}
						if (ncal_sec % 15 == 0){
							nd15_flag[i] = 1;
						}
					}
				}
			}
		}
		if (nd5_bsum[i] > 0 || nd5_sum[i] > 0){
			plogo->stat_scpos[i] = 1;
		}
		else{
			plogo->stat_scpos[i] = 0;
		}
	}
	// 15の倍数のシーンチェンジ間隔がある場所に関係あるかチェック
	for(i=0; i<num_scpos; i++){
		if (plogo->stat_scpos[i] > 0){
			flag_nd15 = 0;
			// 関係性をチェック
			j = i;
			while(j >= 1 && j < num_scpos-1 && flag_nd15 == 0){		// 先頭と最後は除く
				if (nd15_flag[j] > 0){		// 15の倍数だった場合フラグを立てる
					flag_nd15 = 1;
				}
				j = nd5_nxt[j];
			}
			// 関係性があった場合のセット
			if (flag_nd15 > 0){
				j = i;
				while(j >= 0 && j < num_scpos){
					nd15_flag[j] = 1;
					j = nd5_nxt[j];
				}
			}
		}
	}
	for(i=0; i<num_scpos; i++){
		if (nd15_flag[i] == 0){			// 15の倍数に関連なければ同期ステータスを解除
			plogo->stat_scpos[i] = 0;
		}
	}
//	for(i=0; i<num_scpos; i++){
//		printf("(%ld:%d,%d,%d)", plogo->frm_scpos_s[i], nd15_flag[i], nd5_sum[i], nd5_bsum[i]);
//	}

	// ２５フレーム以内に５の倍数秒間隔シーンチェンジが重なる場合、片方のみにする
	ncmp_i = -1;
	ncmp_frm = 0;
	for(i=1; i<num_scpos-1; i++){		// 先頭と最後は除く
		frm_src = plogo->frm_scpos_s[i];
		if (frm_src - ncmp_frm >= CnvTimeFrm(0,870) && ncmp_i >= 0){
			ncmp_i = -1;
		}
		if (plogo->stat_scpos[i] > 0){
			if (ncmp_i < 0){
				ncmp_i = i;
				ncmp_frm = frm_src;
			}
			else{
				count_src = 0;
				count_cmp = 0;
				for(j=1; j<num_scpos-1; j++){		// 先頭と最後以外で比較
					if (plogo->stat_scpos[j] > 0){
						frm_dst = plogo->frm_scpos_s[j];
						flag1 = adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, frm_src, frm_dst);
						flag2 = adjust_calcdif(&ncal2_sgn, &ncal2_sec, &ncal2_dis, ncmp_frm, frm_dst);
						if (flag1 > 0 && flag2 > 0 &&
							ncal_sec % 5 == 0 && ncal_sec == ncal2_sec && ncal_sec <= 60){
							if (ncal_dis < ncal2_dis){
								count_src += 3;
							}
							else if (ncal_dis > ncal2_dis){
								count_cmp += 3;
							}
						}
						else if (flag1 > 0 && (flag2 == 0 || ncal2_sec % 5 != 0) && ncal_sec % 5 == 0){
							if (ncal_sec <= 60){
								count_src ++;
								if (ncal_dis <= 3){
									count_src ++;
								}
							}
						}
						else if (flag2 > 0 && (flag1 == 0 || ncal_sec % 5 != 0) && ncal2_sec % 5 == 0){
							if (ncal2_sec <= 60){
								count_cmp ++;
								if (ncal2_dis <= 3){
									count_cmp ++;
								}
							}
						}
					}
				}
//printf("(0:%d-%d,%d,%d)", frm_src,ncmp_frm,count_src,count_cmp);
				if (count_src <= 1 && count_cmp <= 1){
					// 関連フレームが何もない場合は残す
				}
				else if (count_src > count_cmp){
//printf("(1:%d,%d,%d)", ncmp_i,count_src,count_cmp);
					for(j=0; j<ncmp_i; j++){		// 差し替え元参照を入れ替える
						if (nd5_nxt[j] == ncmp_i){
							nd5_nxt[j] = i;
						}
					}
					plogo->stat_scpos[ncmp_i] = -1;
					ncmp_i = i;
					ncmp_frm = frm_src;
				}
				else{
//printf("(2:%d,%d,%d)", i,count_src,count_cmp);
					for(j=0; j<i; j++){
						if (nd5_nxt[j] == i){	// 差し替え元参照を入れ替える
							nd5_nxt[j] = ncmp_i;
						}
					}
					plogo->stat_scpos[i] = -1;
				}
			}
		}
	}

	// 先頭から順番にシーンチェンジ位置をたどっていく
	nst = 0;
//	plogo->stat_scpos[nst] = 2;
	while( nst<num_scpos-1 ){
		// 非同期終了地点を検索
		ned = nst+1;
		while((plogo->stat_scpos[ned] <= 0 || nd5_sum[ned] == 0) && ned < num_scpos-1){
			ned ++;
		}
		// 少し先に正しい非同期終了地点がないか検索
		if (ned < num_scpos-1){
			nc_nxt = nd5_nxt[ned];
			i = ned + 1;
			while(i < nc_nxt){
				if (plogo->stat_scpos[i] >= 0 && nd5_sum[i] > 0){
					if (nd5_sum[i] > nd5_sum[ned]){		// 5の倍数間隔が多ければ非同期終了地点を変更
						ned = i;
						nc_nxt = nd5_nxt[ned];
					}
				}
				i ++;
			}
		}

		// 非同期２地点間のシーンチェンジがないか検索
		// 間隔が5の倍数ではなくても非同期両端から3〜10秒のシーンチェンジは入れる
		frm_st = plogo->frm_scpos_s[nst];
		frm_ed = plogo->frm_scpos_s[ned];
		plogo->stat_scpos[ned] = 2;
		ncmp_i = -1;
		ncmp_frm = 0;
		for(i=nst+1; i<ned; i++){
			frm_dst = plogo->frm_scpos_s[i];
			// １つ前の候補地点から離れたら１つ前の候補は確定する
			if (frm_dst - ncmp_frm >= CnvTimeFrm(0,940) && ncmp_i >= 0){
				ncmp_i = -1;
			}
			// 非同期２地点のどちらからも３秒以上離れていることが前提
			if (frm_dst - frm_st >= CnvTimeFrm(2,670) && frm_ed - frm_dst >= CnvTimeFrm(2,670)){
				dis_ncmp = 0;
				dis_dst = 1;
				// 前方からの間隔チェック（フレーム０は除く）
				if (adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, frm_st, frm_dst) > 0 && nst > 0){
					if (ncal_sec < 10){
						dis_dst = 0;
						if (ncmp_i >= 0 && nst != 0){
							if (adjust_calcdif(&ncal2_sgn, &ncal2_sec, &ncal2_dis, frm_st, ncmp_frm) > 0){
								if (ncal_dis < ncal2_dis){
									dis_ncmp = 1;
								}
								else{
									dis_dst = 1;
								}
							}
						}
					}
				}
				// 後方からの間隔チェック
				if (adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, frm_dst, frm_ed) > 0){
					if (ncal_sec < 10){
						dis_dst = 0;
						if (ncmp_i >= 0 && ned < num_scpos-1){
							if (adjust_calcdif(&ncal2_sgn, &ncal2_sec, &ncal2_dis, ncmp_frm, frm_ed) > 0){
								if (ncal_dis < ncal2_dis){
									dis_ncmp = 1;
								}
								else{
									dis_dst = 1;
								}
							}
						}
					}
				}
				if (dis_ncmp > 0){			// １つ前の候補が無効と判定された場合
					plogo->stat_scpos[ncmp_i] = -1;
					ncmp_i = -1;
				}
				if (dis_dst == 0){			// 現在の候補が無効ではないと判定された場合
					plogo->stat_scpos[i] = 2;
					ncmp_i = i;
					ncmp_frm = frm_dst;
				}
			}
		}
		// 同期しているシーンチェンジの最終位置を取得
		nc_st = ned;
		nc_nxt = nd5_nxt[nc_st];
		i = nc_st + 1;
		while(nc_nxt > 0 && i <= nc_nxt && i <= num_scpos-1){
			if (i == nc_nxt){				// 同期フレーム到着時
				nc_st = i;
				nc_nxt = nd5_nxt[nc_st];
				plogo->stat_scpos[nc_st] = 2;
			}
			else if (i == num_scpos-1){		// 最終フレーム時
				nc_st = i;
				nc_nxt = 0;
			}
			else{
				if (nd5_sum[i] > nd5_sum[nc_st] && plogo->stat_scpos[i] >= 0){	// 同期フレーム到着前に候補があった場合
					ntmp_nxt = nd5_nxt[i];
					frm_tmp_st = plogo->frm_scpos_s[i];				// 新規候補位置
					frm_tmp_ed = plogo->frm_scpos_s[ntmp_nxt];
					frm_tmp_st2 = plogo->frm_scpos_s[nc_st];		// 元の位置
					frm_tmp_ed2 = plogo->frm_scpos_s[nc_nxt];
					flag1 = adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, frm_tmp_st, frm_tmp_ed);
					flag2 = adjust_calcdif(&ncal2_sgn, &ncal2_sec, &ncal2_dis, frm_tmp_st2, frm_tmp_ed2);
					if (flag1 > 0 && ncal_dis <= 5){			// 誤差が小さかった場合のみ非同期地点とする
						if (flag2 == 0 ||
							(ncal2_sec % 15 != 0 && (ncal_sec <= 30 || ncal_sec <= ncal2_sec)) || 
							(frm_tmp_ed <= frm_tmp_ed2 + CnvTimeFrm(10,0))){
							nc_nxt = 0;
						}
					}
				}
			}
			i ++;
		}
		nst = nc_st;
	}

	// 開始直後のシーンチェンジのチェック
	ncmp_i = -1;
	ncmp_stat = -1;
	i = 1;
	while(plogo->frm_scpos_s[i] <= CnvTimeFrm(30,400) && i < num_scpos){		// 30秒以内をチェック
		ntmp_stat = plogo->stat_scpos[i];
		if (ntmp_stat == 2 && ncmp_stat <= 1){	// 最初の確定候補
			ntmp_stat = 1;
			if (nd15_flag[i] > 0){		// 15の倍数が後で存在する場合のみ
				flag1 = 0;
				flag2 = 0;
				j = 0;					// 念のためループ防止
				// １回目の間隔取得
				nc_st  = i;
				nc_nxt = nc_st;
				while(nc_nxt > 0 && flag1 <= 1 && j < 100){
					nc_nxt = nd5_nxt[nc_nxt];
					j ++;
					if (nc_nxt > 0){
						frm_src = plogo->frm_scpos_s[nc_st];
						frm_dst = plogo->frm_scpos_s[nc_nxt];
						flag1 = adjust_calcdif_exact(&ncal_sgn, &ncal_sec, &ncal_dis, frm_src, frm_dst);
					}
				}
				// ２回目の間隔取得
				nc_st  = nc_nxt;
				while(nc_nxt > 0 && flag2 <= 0 && j < 100){
					nc_nxt = nd5_nxt[nc_nxt];
					j ++;
					if (nc_nxt > 0){
						frm_src = plogo->frm_scpos_s[nc_st];
						frm_dst = plogo->frm_scpos_s[nc_nxt];
						flag2 = adjust_calcdif_exact(&ncal2_sgn, &ncal2_sec, &ncal2_dis, frm_src, frm_dst);
					}
				}
				if (flag1 > 0 && flag2 > 0){
					ntmp_stat = 2;
				}
			}
			plogo->stat_scpos[i] = ntmp_stat;
		}
		if (ntmp_stat > 1){							// 同期が既にある場合は何もしない
			ncmp_i = -1;
			ncmp_stat = ntmp_stat;
		}
		else if (plogo->frm_scpos_s[i] <= CnvTimeFrm(30,400)){		// 開始30秒以内が候補
			if (ntmp_stat > ncmp_stat){				// 最初の同期なしシーンチェンジが候補
				ncmp_stat = ntmp_stat;
				ncmp_i = i;
			}
		}
		i ++;
	}
	if (ncmp_i > 0){				// 最初の同期なしシーンチェンジがあった場合
//		plogo->stat_scpos[ncmp_i] = 2;
		plogo->nchk_scpos_1st = ncmp_i;
	}
	else{
		plogo->nchk_scpos_1st = 0;
	}
	plogo->frm_scpos_1stsel = -1;	// 先頭位置のSelect初期確定なしで初期化

//	for(i=0; i<num_scpos; i++){
//		printf("(%ld:%d, %d,%d,%d)", plogo->frm_scpos_s[i], plogo->stat_scpos[i], nd15_flag[i], nd5_sum[i], nd5_bsum[i]);
//	}

	// 単なる候補は初期状態に戻す
	for(i=1; i<num_scpos-1; i++){		// 先頭と最後は除く
		if (plogo->stat_scpos[i] == 1){
			plogo->stat_scpos[i] = 0;
		}
	}

	// 配置情報初期化
	for(i=0; i<plogo->num_scpos; i++){
		plogo->arstat_sc_e[i] = D_SCINT_UNKNOWN;
	}

	return 0;
}

//---------------------------------------------------------------------
// CutMrgIn/CutMrgOut位置の自動検出
// 入力
//   rise : 0=CutMrgOut  1=CutMrgIn
// 出力
//   half : 0:整数 1:追加(0.5)
// 返り値
//   フレーム数
//---------------------------------------------------------------------
int adjust_cutmrg_detect_sub(int *half, LOGO_RESULTREC *plogo, int rise){
	short hist[200];
	short hist_h[200];
	short tmp_hist1[3];
	short tmp_hist2[3];
	int i, j, loop;
	int stat_level;
	int scale, sum_scale;
	int frm_logo, frm_dif;
	int fade, intl, fade_cnt;
	int tmp_dif;
	int loc_max, loc_det, loc_half;
	int val_max, val;
	const int MINUS = 10;

	loop = 2;
	while(loop > 0){
		// 使用する確定度
		if (loop == 2){
			stat_level = 2;
		}
		else{
			stat_level = 0;
		}

		fade_cnt = 0;
		sum_scale = 0;
		for(i=0; i<200; i++){
			hist[i] = 0;
			hist_h[i] = 0;
		}
		for(i=0; i<plogo->num_find; i++){
			if (rise > 0){
				frm_logo = plogo->frm_rise[i];
				frm_dif  = plogo->frm_rise_r[i] - plogo->frm_rise_l[i];
				fade     = plogo->fade_rise[i];
				intl     = plogo->intl_rise[i];
			}
			else{
				frm_logo = plogo->frm_fall[i];
				frm_dif  = plogo->frm_fall_r[i] - plogo->frm_fall_l[i];
				fade     = plogo->fade_fall[i];
				intl     = plogo->intl_fall[i];
			}
			// ロゴの不確定度で重みづけ
			if (frm_logo <= 1 || frm_logo >= plogo->frm_totalmax-1){
				scale = 0;
			}
			else if (frm_dif <= 1 || frm_dif <= fade){
				scale = 3;
			}
			else{
				scale = 1;
			}
			sum_scale += scale;
			// フェード設定ありの判別
			if (fade > 0){
				fade_cnt += scale;
			}
			else{
				fade_cnt -= scale;
			}

			// 確定無音シーンチェンジをマージン位置候補として追加していく
			if (scale > 0){
				for(j=1; j < plogo->num_scpos - 1; j++){
					if (rise > 0){
						tmp_dif = frm_logo - plogo->frm_scpos_s[j];
					}
					else{
						tmp_dif = plogo->frm_scpos_e[j] - frm_logo;
					}
					tmp_dif += MINUS;
//printf("[%d:%d,%d]", j, tmp_dif, frm_logo);
					if (plogo->stat_scpos[j] >= stat_level){
						if (tmp_dif >= 0 && tmp_dif < 200){
//printf("[%d,%d]", tmp_dif, frm_logo);
							hist[tmp_dif] += scale;
							if (intl > 0){
								hist_h[tmp_dif] += scale;
							}
						}
					}
				}
			}
		}

		// 候補から位置を決める
		loc_max = 0;
		val_max = 0;
		for(i=2; i<200-2; i++){
			if (fade_cnt > 0){
				val = hist[i-2] + hist[i-1] + hist[i] + hist[i+1] + hist[i+2];
			}
			else{
				val = hist[i-1] + hist[i] + hist[i+1];
			}
			if (i >= MINUS-2 && i <= 36+MINUS){	// 標準的なマージン範囲
				val *= 2;						// 大き目に設定
			}
			else if (val == sum_scale){			// 全位置で検出した場合
				val = val * 5 / 3;				// 少し大き目に設定
			}
			else if (i >= 60+MINUS){			// 2秒以上はほぼ対象外
				val /= 4;
			}
			if (val_max < val){
				val_max = val;
				loc_max = i;
			}
			else if (val_max == val && loc_max > 0){
				if (hist_h[loc_max-1] < (hist[i+1] - hist_h[i+1])){
					val_max = val;
					loc_max = i;
				}
			}
		}
		// 候補が存在するか確認
		if (plogo->num_find == 1 && val_max > 0){		// ロゴ１つだけの場合
			loop = 0;
		}
		if (val_max <= 1 || val_max <= plogo->num_find * 2/3){	// 候補不足
			loop --;
			loc_max = 0;
		}
		else{
			loop = 0;
		}
	}

	// 候補の中から確定させる
	loc_det = MINUS;
	loc_half = 0;
	if (loc_max >= 2){
		if (fade_cnt <= 0){				// フェードなし
			loc_det = loc_max-1;
			val_max = 0;
			for(i=loc_max-1; i<=loc_max+1; i++){
				val = hist[i];
				tmp_hist1[i-loc_max+1] = hist[i] - hist_h[i];
				tmp_hist2[i-loc_max+1] = hist_h[i];
				if (val_max < val){
					val_max = val;
					loc_det = i;
				}
			}
//printf("(%d,%d:%d,%d,%d - %d,%d,%d)", loc_det,loc_max,
// tmp_hist1[0],tmp_hist1[1],tmp_hist1[2],tmp_hist2[0],tmp_hist2[1],tmp_hist2[2]);
			if (loc_det == loc_max-1){
				if (tmp_hist2[0] < 3){		// HALFデータなし
					loc_half = 0;
					if (loc_det <= MINUS){
						loc_half = 1;
					}
				}
				else if ((tmp_hist1[1] + tmp_hist1[2])*5 <= tmp_hist1[0]){ // 全部最小
					loc_half = 1;
				}
				else{				// ALL:中間 HALF:最小
					loc_det += 1;
					loc_half = 0;
				}
			}
			else if (loc_det == loc_max + 1){
				if (tmp_hist2[0] < 3 && tmp_hist2[1] < 3 && tmp_hist2[2] < 3){	// HALFデータなし
					loc_half = 0;
					if (tmp_hist1[1] >= 3 && tmp_hist1[1] > tmp_hist1[0] * 2){
						loc_det -= 1;
					}
					if (loc_det <= MINUS){
						loc_half = 1;
					}
				}
				else if (tmp_hist2[0] >= 3 && tmp_hist2[0] > tmp_hist2[2] * 2){
					loc_det -= 1;		// ALL:中間 HALF:最小
					loc_half = 0;
				}
				else if (tmp_hist2[1] >= 3 && tmp_hist2[1] > tmp_hist2[0] * 2){
					loc_half = 0;		// ALL:最大 HALF:中間
				}
				else{
					loc_half = 1;		// 全部中間
				}
			}
			else{
				if (tmp_hist2[0] < 3 && tmp_hist2[1] < 3){	// HALFデータなし
					loc_half = 0;
					if (tmp_hist1[0] >= 3 && tmp_hist1[0] > tmp_hist1[2] * 2){
						loc_det -= 1;
					}
					if (loc_det <= MINUS){
						loc_half = 1;
					}
				}
				else if (tmp_hist2[0] >= 3 && tmp_hist2[0] > tmp_hist2[2] * 2){
					loc_half = 0;		// ALL:中間 HALF:最小
				}
				else if (tmp_hist1[2] >= 3 && tmp_hist1[2] > tmp_hist1[0] * 2){
					loc_det += 1;		// ALL:最大 HALF:中間
					loc_half = 0;
				}
				else{
					loc_half = 1;		// 全部中間
				}
			}
		}
		else{					// フェード時
			loc_det = loc_max-2;
			val_max  = hist[loc_max-2];
			if (loc_det == 0){
				loc_det = 1;
				val_max = hist[1];
			}
			for(i=loc_max-1; i<=loc_max+2; i++){
				val = hist[i];
				if (val_max * 2 <= val){
					val_max = val;
					loc_det = i;
				}
			}
//printf("(%d,%d:%d,%d,%d,%d,%d)", loc_det,loc_max,
// hist[loc_max-2],hist[loc_max-1],hist[loc_max],hist[loc_max+1],hist[loc_max+2]);
			if (hist[loc_det+1] >= 3 && hist[loc_det-1] * 2 < hist[loc_det+1] &&
				hist[loc_det+1] > hist[loc_det]){
				loc_det += 1;
			}
		}
	}
	*half = loc_half;

	return (loc_det - MINUS);
}


//---------------------------------------------------------------------
// CutMrgIn/CutMrgOut位置の自動検出
//---------------------------------------------------------------------
int adjust_cutmrg_detect(LOGO_RESULTREC *plogo){
	int cutin, cutout;
	int half_cutin, half_cutout;
	int disp_num_cutin, disp_num_cutout;
	const char strdisp[4][4] = {"", ".5", "", "+.5"};

	cutin  = adjust_cutmrg_detect_sub(&half_cutin,  plogo, 1);
	cutout = adjust_cutmrg_detect_sub(&half_cutout, plogo, 0);

	if (plogo->fix_cutin == 0){
		plogo->frm_cutin  = cutin;
		plogo->half_cutin = half_cutin;
	}
	if (plogo->fix_cutout == 0){
		plogo->frm_cutout  = cutout;
		plogo->half_cutout = half_cutout;
	}

	//--- display auto detect CurMrgIn/Out ---
	if (1){
		// 検出マージン
		disp_num_cutin  = half_cutin  & 1;
		disp_num_cutout = half_cutout & 1;
		if (cutin < 0){
			disp_num_cutin += 2;
		}
		if (cutout < 0){
			disp_num_cutout += 2;
		}
		printf("auto detect CutMrgIn=%d%s CutMrgOut=%d%s\n",
			cutin,  strdisp[disp_num_cutin],
			cutout, strdisp[disp_num_cutout]);

		// 設定マージン
		disp_num_cutin  = plogo->half_cutin  & 1;
		disp_num_cutout = plogo->half_cutout & 1;
		if (cutin < 0){
			disp_num_cutin += 2;
		}
		if (cutout < 0){
			disp_num_cutout += 2;
		}
		printf("current set CutMrgIn=%d%s CutMrgOut=%d%s\n",
			(int) plogo->frm_cutin,  strdisp[disp_num_cutin],
			(int) plogo->frm_cutout, strdisp[disp_num_cutout]);
	}

	return 0;
}


//---------------------------------------------------------------------
// CutMrgIn/CutMrgOut を設定
//---------------------------------------------------------------------
int adjust_cutmrg_set(LOGO_RESULTREC *plogo){
	int i;
	int n1, n2, n3;
	int intl;
	int frm_max;

	for(i=0; i<plogo->num_find; i++){
		// rise設定
		n1 = plogo->frm_rise[i]   - plogo->frm_cutin;
		n2 = plogo->frm_rise_l[i] - plogo->frm_cutin;
		n3 = plogo->frm_rise_r[i] - plogo->frm_cutin;
		intl = plogo->intl_rise[i];
		if (intl > 0 && plogo->half_cutin == 0){
			n1 ++;
			n2 ++;
			n3 ++;
		}
		if (n1 < 0){
			n1 = 0;
		}
		if (n2 < 0){
			n2 = 0;
		}
		if (n3 < 0){
			n3 = 0;
		}
		plogo->frm_rise[i]   = n1;
		plogo->frm_rise_l[i] = n2;
		plogo->frm_rise_r[i] = n3;

		// fall設定
		frm_max = plogo->frm_totalmax;
		n1 = plogo->frm_fall[i]   + plogo->frm_cutout;
		n2 = plogo->frm_fall_l[i] + plogo->frm_cutout;
		n3 = plogo->frm_fall_r[i] + plogo->frm_cutout;
		intl = plogo->intl_fall[i];
		if (intl > 0 && plogo->half_cutout == 0){
			n1 --;
			n2 --;
			n3 --;
		}
		if (plogo->frm_cutout <= 0){	// シーンチェンジより後になる場合
			n2 = n2 + plogo->frm_cutout - 1;
		}
		if (n1 < 0){
			n1 = 0;
		}
		if (n2 < 0){
			n2 = 0;
		}
		if (n3 < 0){
			n3 = 0;
		}
		if (n1 > frm_max){
			n1 = frm_max;
		}
		if (n2 > frm_max){
			n2 = frm_max;
		}
		if (n3 > frm_max){
			n3 = frm_max;
		}
		plogo->frm_fall[i]   = n1;
		plogo->frm_fall_l[i] = n2;
		plogo->frm_fall_r[i] = n3;
	}
	return 0;
}


//---------------------------------------------------------------------
// ロゴ位置を無音シーンチェンジ位置から補正
// 場合によってはロゴ位置から無音シーンチェンジ位置を修正
//---------------------------------------------------------------------
int adjust_logo(LOGO_RESULTREC *plogo){
	int i, j;
	int n, edge;
	int frm_lg_c, frm_lg_l, frm_lg_r;
	int frm_lgn_l, frm_lgn_r;
	int lgr_stat, lgr_dif, lgr_frm;
	int lgm1_num, lgm1_dif, lgm1_frm;
	int lgm2_num, lgm2_dif, lgm2_frm;
	int lgr_num;
	int lgn_num, lgn_dif;
	int lgn2_num, lgn2_dif;
	int frm_sc, stat_sc;
	int frm_smute_s, frm_smute_e;
	int frm_dif;
	int flag1, flag2;
	int flag_lg_change, flag_sc_change, num_sc_change;
	int tmp_stat;
	int frm_change;
	int ncal_sgn, ncal_sec, ncal_dis;
	int ncal2_sgn, ncal2_sec, ncal2_dis;

	// ロゴデータ指定がない場合、全体をロゴデータとする
//	if (plogo->flag_nologo > 0 && plogo->num_find == 0){
	if (plogo->num_find == 0){
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
		// ロゴ読み込みなしに変更
		plogo->flag_nologo = 1;
	}

	for(i=0; i<plogo->num_find*2; i++){
		n = i / 2;							// logo number
		edge = i % 2;						// 0:start edge  1:end edge


		if (edge == 0){
			frm_lg_c = plogo->frm_rise[n];
			frm_lg_l = plogo->frm_rise_l[n];
			frm_lg_r = plogo->frm_rise_r[n];
			plogo->stat_rise[n] = 0;		 	// 初期化
		}
		else{
			frm_lg_c = plogo->frm_fall[n];
			frm_lg_l = plogo->frm_fall_l[n];
			frm_lg_r = plogo->frm_fall_r[n];
			plogo->stat_fall[n] = 0;		 	// 初期化
		}
		frm_lg_l -= 5;
		frm_lg_r += 5;
		frm_lgn_l = frm_lg_l - 60;
		frm_lgn_r = frm_lg_r + 60;

		lgr_num  = -1;
		lgr_stat = -1;
		lgr_dif  = -1;
		lgr_frm  = -1;
		lgm1_num  = -1;
		lgm1_dif  = -1;
		lgm1_frm  = -1;
		lgm2_num  = -1;
		lgm2_dif  = -1;
		lgm2_frm  = -1;
		lgn_num   = -1;
		lgn_dif   = -1;
		lgn2_num  = -1;
		lgn2_dif  = -1;
		// ロゴ位置に近い無音シーンチェンジを検索
		for(j=1; j<plogo->num_scpos-1; j++){
			if (edge == 0){
				frm_sc = plogo->frm_scpos_s[j];
			}
			else{
				frm_sc = plogo->frm_scpos_e[j];
			}
			frm_smute_s = plogo->frm_smute_s[j];
			frm_smute_e = plogo->frm_smute_e[j];
			stat_sc = plogo->stat_scpos[j];
			frm_dif = abs(frm_sc - frm_lg_c);

//		printf("(%d %d)", stat_sc, frm_sc);
			// ロゴ区間内にある無音シーンチェンジ検出
			if (frm_sc >= frm_lg_l && frm_sc <= frm_lg_r){
				if ((stat_sc >= lgr_stat && (frm_dif < lgr_dif || lgr_dif < 0)) ||
					stat_sc > lgr_stat){
					lgr_num = j;
					lgr_stat = stat_sc;
					lgr_dif = frm_dif;
					lgr_frm = frm_sc;
				}
			}
			// 誤検出調査用のロゴ区間付近無音シーンチェンジ検出
			if (frm_sc >= frm_lgn_l && frm_sc <= frm_lgn_r){
				if ((frm_dif < lgn_dif || lgn_dif < 0) && stat_sc <= 1){
					lgn_num = j;
					lgn_dif = frm_dif;
				}
			}
			// 誤検出調査2用のロゴ区間付近無音シーンチェンジ検出
			if (frm_sc >= frm_smute_s-2 && frm_sc <= frm_smute_e+2){
				if ((frm_dif < lgn2_dif || lgn2_dif < 0) && stat_sc <= 1){
					lgn2_num = j;
					lgn2_dif = frm_dif;
				}
			}
			// 近くの確定無音シーンチェンジ位置を検出
			if ((frm_dif < lgm1_dif || lgm1_dif < 0) && stat_sc == 2){
				lgm1_num = j;
				lgm1_dif = frm_dif;
				lgm1_frm = frm_sc;
			}
			// 隣の確定無音シーンチェンジ位置を検出
			if ((frm_dif < lgm2_dif || lgm2_dif < 0) && stat_sc == 2 && frm_dif > 75){
				if ((edge == 0 && frm_sc < frm_lg_c) ||
					(edge != 0 && frm_sc > frm_lg_c)){
					lgm2_num = j;
					lgm2_dif = frm_dif;
					lgm2_frm = frm_sc;
				}
			}
		}
//		printf("%d %d %d %d %d\n", lgr_stat, lgr_frm, lgm1_frm, lgm1_dif, lgm2_frm);
		// ロゴの確定度
		if (edge == 0){
			plogo->stat_rise[n] = lgr_stat;
		}
		else{
			plogo->stat_fall[n] = lgr_stat;
		}
		// ロゴ位置を変更するか判断
		flag_lg_change = 0;
		flag_sc_change = 0;
		if (lgm1_dif > 3 && lgm1_dif <= 30 && lgm2_num >= 0){	// 傍に無音シーンチェンジがあった場合
			flag1 = adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, lgm2_frm, frm_lg_c);
			flag2 = adjust_calcdif(&ncal2_sgn, &ncal2_sec, &ncal2_dis, lgm2_frm, lgm1_frm);
//			if (flag1 > 0){
//printf("[%d %d]", ncal_sec, ncal2_sec);
//				if (ncal_sec % 5 != 0 && ncal2_sec % 5 == 0){
//					flag1 = 0;
//				}
//			}
			// ロゴ範囲内に確定した無音シーンチェンジがあった場合の処理
			if (lgr_stat == 2){
				if (flag1 == 0 && flag2 == 0){		// 両方とも単位距離外の場合
					flag2 = 1;						// ロゴを無音シーンチェンジ位置に移す
				}
				else if (flag1 != 0 && flag2 != 0){	// 両方とも単位距離内の場合
					if (ncal_sec != ncal2_sec){		// 時間が全然違う場合
						flag1 = 0;					// ロゴを無音シーンチェンジ位置に移す
					}
					else{
						ncal_dis += 5;				// ロゴ側を厳しく取る
					}
				}
			}
			// 確定シーンチェンジから単位距離になる位置に近い方を選択する
			if (flag1 == 0 && flag2 == 0){		// 両方とも単位距離外の場合
			}
			else if ((ncal_sec == ncal2_sec && ncal_dis > ncal2_dis) || flag1 == 0){
				flag_lg_change = 1;
				frm_change = lgm1_frm;
				lgr_num = lgm1_num;
			}
			else if ((ncal_sec == ncal2_sec && ncal_dis < ncal2_dis) || flag2 == 0){
				flag_sc_change = 1;
				num_sc_change = lgm1_num;
				frm_change = frm_lg_c;
			}
		}
		else if (lgr_stat == 2){			// ロゴ範囲内に確定した無音シーンチェンジがあった場合
			flag_lg_change = 1;
			frm_change = lgr_frm;
		}
		else if (lgn_num >= 0 && lgm2_num >= 0){	// 不確定／確定シーンチェンジが近くに両方ある場合
			if (adjust_calcdif(&ncal_sgn, &ncal_sec, &ncal_dis, lgm2_frm, frm_lg_c) > 0){
				if (ncal_sec % 15 == 0 || ncal_sec == 10){		// ロゴが確定シーンチェンジから単位距離になる場合
					if (lgn_dif > 3){							// ロゴと無音シーンチェンジが隣接していない場合
						flag_sc_change = 1;
						num_sc_change = lgn_num;
						frm_change = frm_lg_c;
					}
				}
			}
		}
		else if (lgn_num >= 0 && edge == 0 && plogo->frm_rise[n] <= 910 &&	// 先頭部分のロゴ
				 (n == 0 || n == 1)){
			if (lgr_num >= 0){										// 候補無音シーンチェンジあり
				if (frm_lg_l <= 0 && (lgr_frm > frm_lg_c * 2)){		// 0地点の方が近い場合何もしない
				}
				else{
					flag_lg_change = 1;
					frm_change = lgr_frm;
				}
			}
			else{											// ロゴ区間にシーンチェンジなし
				if (lgn2_num >= 0){							// ロゴ区間に無音あり
					if (plogo->stat_scpos[lgn2_num] <= 0){
						flag_sc_change = 1;
						num_sc_change = lgn2_num;
						frm_change = frm_lg_c;
					}
				}
			}
		}
		// 最初の開始位置候補が不確定だった場合には変更
		// 最初から開始位置で、次のロゴが30秒以内に始まる場合も追加
		if (lgr_num >= 0){
			if ((n == 0 && edge == 0 && plogo->nchk_scpos_1st != 0) ||
				(n == 1 && edge == 0 && plogo->nchk_scpos_1st != 0 &&
					plogo->frm_rise[0] <= 1 && lgr_frm <= CnvTimeFrm(30,400))){
				plogo->nchk_scpos_1st = lgr_num;
			}
		}
		// ロゴ変更が最終シーンチェンジより後のフレームだった場合は変更せず
		if (flag_lg_change > 0 && frm_change > plogo->frm_scpos_e[plogo->num_scpos-1]){
			flag_lg_change = 0;
		}
		// ロゴ位置の変更
		if (flag_lg_change > 0){
			if (edge == 0){
				if (plogo->frm_rise[n] != frm_change){
					if (abs(plogo->frm_rise[n] - frm_change) > 2){
						printf("change logo : %ld -> %d\n", plogo->frm_rise[n], frm_change);
					}
					plogo->frm_rise[n] = frm_change;
					if (plogo->frm_rise_l[n] > frm_change){
						plogo->frm_rise_l[n] = frm_change;
					}
					if (plogo->frm_rise_r[n] < frm_change){
						plogo->frm_rise_r[n] = frm_change;
					}
				}
			}
			else{
				if (plogo->frm_fall[n] != frm_change){
					if (abs(plogo->frm_fall[n] - frm_change) > 2){
						printf("change logo : %ld -> %d\n", plogo->frm_fall[n], frm_change);
					}
					plogo->frm_fall[n] = frm_change;
					if (plogo->frm_fall_l[n] > frm_change){
						plogo->frm_fall_l[n] = frm_change;
					}
					if (plogo->frm_fall_r[n] < frm_change){
						plogo->frm_fall_r[n] = frm_change;
					}
				}
			}
		}
		// シーンチェンジ位置変更
		if (flag_sc_change > 0){
			if (edge == 0){
				if (plogo->frm_scpos_s[num_sc_change] != frm_change){
					printf("change scpos : %ld -> %d\n", plogo->frm_scpos_s[num_sc_change], frm_change);
					tmp_stat = plogo->stat_scpos[num_sc_change];
					plogo->stat_scpos[num_sc_change] = -1;
					insert_scpos(plogo, frm_change, frm_change - 1,
								plogo->frm_smute_s[num_sc_change], plogo->frm_smute_e[num_sc_change],
								tmp_stat, 1);
				}
			}
			else{
				if (plogo->frm_scpos_e[num_sc_change] != frm_change){
					printf("change scpos : %ld -> %d\n", plogo->frm_scpos_e[num_sc_change], frm_change);
					tmp_stat = plogo->stat_scpos[num_sc_change];
					plogo->stat_scpos[num_sc_change] = -1;
					insert_scpos(plogo, frm_change + 1, frm_change,
								plogo->frm_smute_s[num_sc_change], plogo->frm_smute_e[num_sc_change],
								tmp_stat, 1);
				}
			}
		}
	}
	return 0;
}


//---------------------------------------------------------------------
// 入力データからの微調整を行う処理
//---------------------------------------------------------------------
void adjust_indata(LOGO_RESULTREC *plogo){
	adjust_select_scpos(plogo);
	adjust_cutmrg_detect(plogo);
	adjust_cutmrg_set(plogo);
	adjust_logo(plogo);
}
