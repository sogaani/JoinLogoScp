/*====================================================================
* CM位置検出用ヘッダファイル（auto系パラメータ保持）
*===================================================================*/

/* パラメータ内部状態 */
typedef struct {
	/* codeパラメータ */
	int		c_exe;				/* 0:コマンド実行なし 1:コマンド実行 */
	int		c_search;			/* 検索する範囲を選択 */
	int		c_wmin;				/* 構成期間の最小値秒数 */
	int		c_wmax;				/* 構成期間の最大値秒数 */
	int		c_w15;				/* 1:番組構成で15秒を検索 */
	int		c_lgprev;			/* 0:ロゴ・予告の前側を対象外 */
	int		c_lgpost;			/* 0:ロゴ・予告の後側を対象外 */
	int		c_lgintr;			/* 1:予告と番組提供の間のみ対象とする */
	int		c_lgsp;				/* 1:番組提供が直後にある場合のみ対象 */
	int		c_cutskip;			/* 1:予告カット以降も対象とする */
	int		c_in1;				/* 1:予告位置に番組提供を入れる */
	int		c_chklast;			/* 1:本体構成が後にあれば対象外とする */
	int		c_lgy;				/* 1:ロゴ内を対象とする */
	int		c_lgn;				/* 1:ロゴ外を対象とする */
	int		c_lgbn;				/* 1:両隣を含めロゴ外の場合を対象とする */
	int		c_limloc;			/* 1:標準期間の候補位置のみに限定 */
	int		c_limtrsum;			/* 1:予告期間により無効化する */
	int		c_unitcmoff;		/* 1:CM分割した構成の検出を強制無効 */
	int		c_unitcmon;			/* 1:CM分割した構成の検出を強制設定 */
	int		c_wdefmin;			/* 標準の構成期間の最小値秒数 */
	int		c_wdefmax;			/* 標準の構成期間の最大値秒数 */

	/* 数値パラメータ */
	int		limit;
	int		scope;
	int		scopen;
	int		period;
	int		maxprd;
	int		trsumprd;
	int		secprev;
	int		secnext;
} PRMAUTOREC;


/* ロゴ無し時の自動CM化検出用 */
#define D_AUTOCM_NUM_SMAX 5		/* 状態数最大 */
#define D_AUTOCM_NUM_CMAX 7		/* 項目合計 */
#define D_AUTOCM_C_NSCST  0	 	/* 開始位置シーンチェンジ番号 */
#define D_AUTOCM_C_NSCED  1	 	/* 終了位置シーンチェンジ番号 */
#define D_AUTOCM_C_DET    2	 	/* 構成数（合計）カウント */
#define D_AUTOCM_C_SEC    3	 	/* 秒数（合計）カウント */
#define D_AUTOCM_C_D15    4	 	/* 構成数（15秒）カウント */
#define D_AUTOCM_C_D30    5	 	/* 構成数（30秒）カウント */
#define D_AUTOCM_C_DOV15  6	 	/* 構成数（15秒以上）カウント */
#define D_AUTOCM_T_INIT   0		/* コマンド：初期化 */
#define D_AUTOCM_T_SFT    1		/* コマンド：平行移動 */
#define D_AUTOCM_T_MRG    2		/* コマンド：合併 */
