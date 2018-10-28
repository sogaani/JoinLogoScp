//
// join_logo_scp 外部参照する関数定義
//

//--- from join_logo_scp_com.c  ---
extern int RevFrm(int frm);
extern int CnvTimeFrm(int sec, int d);
extern int ChangeFrameRate(int n, int d);
extern int GetSecFromFrm(int frm);
extern int get_str(char *dst, const char *src, int pos);
extern int get_str_word(char *dst, const char *src, int pos);
extern int get_str_num(int *result, const char *src, int pos);
extern int get_str_numl(long *result, const char *src, int pos);
extern int get_str_frm(int *result, const char *src, int pos);
extern int get_str_frml(long *result, const char *src, int pos);
extern int get_str_sec(int *result, const char *src, int pos);
extern int get_str_secl(long *result, const char *src, int pos);
extern int get_str_numhead(int *result, const char *src, int pos);
extern int get_str_filepath(char *dst, const char *src, int pos);
extern int adjust_calcdif(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst);
extern int adjust_calcdif_select(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst);
extern int adjust_calcdif_exact(int *ncal_sgn, int *ncal_sec, int *ncal_dis, int frm_src, int frm_dst);
extern int insert_scpos(LOGO_RESULTREC *plogo, int frm_dst_s, int frm_dst_e,
						int frm_dmute_s, int frm_dmute_e, int stat_scpos_dst, int overwrite);


//--- from join_logo_scp_auto.c  ---
extern int autochg_start(LOGO_RESULTREC *plogo, int nlg, int nedge, int frm_newlogo);
extern int autocm_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset);
extern int autoup_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset);
extern int autocut_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e);
extern int autoadd_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int frm_cut_s, int frm_cut_e);
extern int autoedge_start(LOGO_RESULTREC *plogo, const CMDSETREC *cmdset, int nlogo);

//--- from join_logo_scp_adj.c ---
extern void adjust_indata(LOGO_RESULTREC *plogo);
