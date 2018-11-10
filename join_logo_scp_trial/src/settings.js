const path = require("path");

exports.CHANNEL_LIST = path.join(__dirname, "../setting/ChList.csv");
exports.PARAM_LIST_1 = path.join(__dirname, "../setting/JLparam_set1.csv");
exports.PARAM_LIST_2 = path.join(__dirname, "../setting/JLparam_set2.csv");

exports.LOGOFRAME = path.join(__dirname, "../bin/logoframe");
exports.CHAPTEREXE = path.join(__dirname, "../bin/chapter_exe");
exports.JLSCP = path.join(__dirname, "../bin/join_logo_scp");

exports.JL_DIR = path.join(__dirname, "../JL");
exports.LOGO_PATH = path.join(__dirname, "../logo");

exports.LOGOFRAME_OUTPUT = path.join(__dirname, "../tmp/obs_logoframe.txt");
exports.CHAPTEREXE_OUTPUT = path.join(__dirname, "../tmp/obs_chapterexe.txt");
exports.JLSCP_OUTPUT = path.join(__dirname, "../tmp/obs_jlscp.txt");
exports.CUT_AVS = path.join(__dirname, "../tmp/obs_logo_cut.avs");
