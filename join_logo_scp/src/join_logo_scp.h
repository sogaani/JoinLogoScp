/*====================================================================
* CM位置検出用ヘッダファイル
*===================================================================*/

#ifndef ___LOGOCHAPTSET_H
#define ___LOGOCHAPTSET_H

#define SIZE_CMDBUF_MAX 512
#define SIZE_BUF_MAX 512

/* フラグ保持数 */
#define SIZE_VARNAME_MAX 24		/* 変数名文字列長 */
#define SIZE_VARNUM_MAX  256	/* 変数合計 */
#define SIZE_VARVAL_MAX  32		/* 各変数の格納データ長 */

/* IF文ネスト最大数 */
#define IFDEPTH_MAX 5

/* ロゴ表示期間の最大組数 */
#define LOGO_FIND_MAX  256
#define SCPOS_FIND_MAX 1024

/* 配置状態 */
#define D_SCINT_UNKNOWN  0		/* 不明 */
#define D_SCINT_L_UNIT   1		/* ロゴ有 １５秒単位 */
#define D_SCINT_L_OTHER  2		/* ロゴ有 その他 */
#define D_SCINT_L_TRKEEP 3		/* ロゴ有 残す予告 */
#define D_SCINT_L_TRCUT  4		/* ロゴ有 カット番宣 */
#define D_SCINT_L_TRRAW  5		/* ロゴ有 エンドカード判断前 */
#define D_SCINT_L_ECCUT  6		/* ロゴ有 カット番宣 */
#define D_SCINT_L_EC     7		/* ロゴ有 エンドカード */
#define D_SCINT_L_SP     8		/* ロゴ有 番組提供 */
#define D_SCINT_L_LGCUT  9		/* ロゴ有 ロゴ端部分カット */
#define D_SCINT_L_LGADD 10		/* ロゴ有 ロゴ端部分残す */
#define D_SCINT_L_MIXED 11		/* ロゴ有 ロゴ無も混合 */
#define D_SCINT_N_UNIT  21		/* ロゴ無 １５秒単位 */
#define D_SCINT_N_OTHER 22		/* ロゴ無 その他 */
#define D_SCINT_N_AUNIT 23		/* ロゴ無 合併で１５秒の中間地点 */
#define D_SCINT_N_BUNIT 24		/* ロゴ無 合併で１５秒の端 */
#define D_SCINT_N_TRCUT 25		/* ロゴ無 カット番宣 */
#define D_SCINT_N_LGCUT 26		/* ロゴ無 ロゴ端部分カット */
#define D_SCINT_N_LGADD 27		/* ロゴ無 ロゴ端部分残す */
#define D_SCINT_B_UNIT  41		/* ロゴ境界 １５秒単位 */
#define D_SCINT_B_OTHER 42		/* ロゴ境界 その他 */

#define D_SCINTR_L_LOW   1		/* ロゴ有の下限値 */
#define D_SCINTR_L_HIGH 11		/* ロゴ有の上限値 */

/* Auto系コマンド指定 */
#define D_CMDAUTO_NONE   0		/* 指定なし */
#define D_CMDAUTO_TR     1		/* TR指定 */
#define D_CMDAUTO_SP     2		/* SP指定 */
#define D_CMDAUTO_EC     3		/* EC指定 */
#define D_CMDAUTO_OTH    4		/* その他指定 */
#define D_CMDAUTO_EDGE  10		/* ロゴ切り替わり指定 */


/* 値保持用の変数構造体 */
typedef struct {
	/* オプション実行分岐用フラグ */
	char	var_num;
	char	var_name[SIZE_VARNUM_MAX][SIZE_VARNAME_MAX];
	char	var_val[SIZE_VARNUM_MAX][SIZE_VARVAL_MAX];
} REGVARREC;

/* IF文状態保持用 */
typedef struct {
	char	ifskip;
	char	ifdepth;
	char	ifstate[IFDEPTH_MAX];
} CMDIFREC;

/* 共通パラメータ保持 */
typedef struct {
	char	path_file[SIZE_CMDBUF_MAX];	/* 追加実行スクリプト用のパス */
	long	frm_wlogo_trmax;			/* AutoCutコマンドでカット対象とするロゴ期間最大フレーム数 */
	long	frm_wcomp_trmax;			/* AutoCutコマンドTRで予告と認識する構成最大フレーム数 */
	long	frm_wlogo_sftmrg;			/* Autoコマンド前調整でロゴ切り替わりのずれを許すフレーム数 */
	long	frm_wcomp_last;				/* ロゴが最後まである時にカット扱いにする構成最大フレーム数 */
	long	frm_wlogo_summin;			/* ロゴ合計期間が指定フレーム未満の時はロゴなしとして扱う */
	long	sec_wcomp_spmin;			/* Autoコマンド番組提供で標準最小秒数 */
	long	sec_wcomp_spmax;			/* Autoコマンド番組提供で標準最大秒数 */
	char	flag_cuttr;					/* 15秒以上番宣をカットする場合は1をセット */
	char	flag_cutsp;					/* 番組提供をカットする場合は1をセット */
	char	flag_addlogo;				/* ロゴあり通常構成を残す場合は1をセット */
	char	flag_adduc;					/* ロゴなし不明構成を残す場合は1をセット */
	char	flag_edge;					/* 0フレームと最終フレームをauto系処理に入れる場合は1をセット */
} PRMVARREC;

/* 結果データの格納 */
typedef struct {
	/* オプション */
	long	frm_cutin;					/* オプション -CutMrgIn  */
	long	frm_cutout;					/* オプション -CutMrgOut */
	long	frm_lastcut;				/* オプション -lastcut */
	char	half_cutin;					/* CutMrgIn小数  0:0 1:0.5 */
	char	half_cutout;				/* CutMrgOut小数 0:0 1:0.5 */
	/* 内部保持パラメータ */
	char	flag_autosetup;				/* 0:初期状態 1:autocut/autoadd用配置実行済み */
	char	flag_nologo;				/* 0:通常 1:ロゴを読み込まない場合 */
	char	fix_cutin;					/* 0:CutMrgIn指定なし 1:CutMrgIn指定あり */
	char	fix_cutout;					/* 0:CutMrgOut指定なし 1:CutMrgOut指定あり */
	long	frm_totalmax;				/* 最大フレーム番号 */
	long	frm_spc;					/* 計算時のマージン時間とするフレーム(AutoCut/AutoAdd用) */

	PRMVARREC	prmvar;					/* 共通パラメータ */

	/* ロゴ表示データ */
	long	num_find;					/* ロゴ表示期間の組数 */
	long	frm_rise[LOGO_FIND_MAX];	/* 各開始フレーム */
	long	frm_fall[LOGO_FIND_MAX];	/* 各終了フレーム */
	long	frm_rise_l[LOGO_FIND_MAX];	/* 各開始フレーム候補開始 */
	long	frm_rise_r[LOGO_FIND_MAX];	/* 各開始フレーム候補終了 */
	long	frm_fall_l[LOGO_FIND_MAX];	/* 各終了フレーム候補開始 */
	long	frm_fall_r[LOGO_FIND_MAX];	/* 各終了フレーム候補終了 */
	char	fade_rise[LOGO_FIND_MAX];	/* 各開始フェードイン状態(0 or fadein) */
	char	fade_fall[LOGO_FIND_MAX];	/* 各終了フェードアウト状態(0 or fadeout) */
	char	intl_rise[LOGO_FIND_MAX];	/* インターレース状態(0:ALL 1:TOP 2:BTM) */
	char	intl_fall[LOGO_FIND_MAX];	/* インターレース状態(0:ALL 1:TOP 2:BTM) */

	/* シーンチェンジ検出データ */
	long	num_scpos;						/* シーンチェンジ検出回数 */
	long	frm_scpos_s[SCPOS_FIND_MAX];	/* シーンチェンジ検出開始位置 */
	long	frm_scpos_e[SCPOS_FIND_MAX];	/* シーンチェンジ検出終了位置 */
	long	frm_smute_s[SCPOS_FIND_MAX];	/* シーンチェンジ無音検出開始位置 */
	long	frm_smute_e[SCPOS_FIND_MAX];	/* シーンチェンジ無音検出終了位置 */
	char	still_scpos[SCPOS_FIND_MAX];	/* シーンチェンジ検出位置で変化なし */

	/* シーンチェンジ検出データの調整用 */
	long	nchk_scpos_1st;					/* 最初の開始位置候補 */
	long	frm_scpos_1stsel;				/* 最初位置をSelectで確定した場合の位置 */
	char	stat_scpos[SCPOS_FIND_MAX];		/* -1:はずれ確定 0:初期 1:候補 2:確定 */
	/* シーンチェンジ検出データの調整用 */
	char	arstat_sc_e[SCPOS_FIND_MAX];	/* 配置状態（終了地点） */
	long	frm_notfound_tr;				/* 予告検出ができなかったフレーム開始位置 */
	/* ロゴ検出データの調整用 */
	char	stat_rise[LOGO_FIND_MAX];		/* -1:はずれ確定 0:初期 1:候補 2:確定 */
	char	stat_fall[LOGO_FIND_MAX];		/* -1:はずれ確定 0:初期 1:候補 2:確定 */

	/* 途中結果格納用 */
	char	flag_unit[LOGO_FIND_MAX];	/* 1:独立フレーム */
	char	flag_exe1st;				/* 1:最初の実行コマンド実行前 */

	/* AutoEdge用 */
	char	flag_edge_rise[LOGO_FIND_MAX];	/* 0:未適用 1:格納結果を反映 */
	char	flag_edge_fall[LOGO_FIND_MAX];
	/* 結果格納 */
	char	flag_rise[LOGO_FIND_MAX];		/* 0:未確定 1:エッジ確定 2:abort確定 */
	char	flag_fall[LOGO_FIND_MAX];
	long	frm_result_rise[LOGO_FIND_MAX];
	long	frm_result_fall[LOGO_FIND_MAX];
} LOGO_RESULTREC;


/* コマンド内容を格納 */
typedef struct {
	char	cmdname[SIZE_CMDBUF_MAX];
	long	frm_center;
	long	frm_left;
	long	frm_right;
	short	pos_str;
	char	flag_cond;
	char	flag_shift;
	char	flag_fromabs;
	char	flag_wide;
	char	flag_fromlast;
	char	flag_withp;
	char	flag_withn;
	char	flag_noedge;
	char	flag_overlap;
	char	flag_confirm;
	char	flag_unit;
	char	flag_else;
	char	flag_cont;
	char	flag_reset;
	char	flag_flat;
	char	flag_fr;
	char	flag_fhead;
	char	flag_ftail;
	char	flag_fmid;
	char	flag_force;
	char	flag_autochg;
	char	flag_autoeach;
	char	select_edge;
	char	select_auto;
	short	select_num;
	short	select_nr;
	long	select_frm_left;
	long	select_frm_right;
	long	select_frm_min;
	long	select_frm_max;
	long	lenp_min;
	long	lenp_max;
	long	lenn_min;
	long	lenn_max;
	short	lens_num;
	char	lens_sctype[10];	/* 0:SC 1:NoSC 2:SM 3:NoSM 4:SMA */
	long	lens_min[10];
	long	lens_max[10];
	long	endlen_center;
	long	endlen_left;
	long	endlen_right;
	long	sft_center;
	long	sft_left;
	long	sft_right;
	long	frm_fromabs;
	long	logoext_left;
	long	logoext_right;
	long	autop_code;
	long	autop_limit;
	long	autop_scope;
	long	autop_scopen;
	long	autop_scopex;
	long	autop_period;
	long	autop_maxprd;
	long	autop_secnext;
	long	autop_secprev;
	long	autop_trscope;
	long	autop_trsumprd;
	long	autop_tr1stprd;

	char	exe_last;
} CMDSETREC;


/* 最終結果を格納 */
typedef struct {
	long	num;						/* 結果格納データ数 */
	long	frm[LOGO_FIND_MAX*2];		/* フレーム番号（偶数：開始、奇数：終了） */
} LAST_RESULTREC;

#endif
