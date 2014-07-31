﻿//  -----------------------------------------------------------------------------------------
//    拡張 x264/x265 出力(GUI) Ex  v1.xx/2.xx/3.xx by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  -----------------------------------------------------------------------------------------
#define AUO_MAIN
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#include <mmsystem.h>
#pragma comment(lib, "winmm.lib") 

#include "output.h"
#include "auo.h"
#include "auo_frm.h"
#include "auo_util.h"
#include "auo_error.h"
#include "auo_version.h"
#include "auo_conf.h"
#include "auo_system.h"

#include "auo_video.h"
#include "auo_audio.h"
#include "auo_faw2aac.h"
#include "auo_mux.h"
#include "auo_encode.h"
#include "auo_runbat.h"

#include "auo_process_parallel.h"

//---------------------------------------------------------------------
//		関数プロトタイプ宣言
//---------------------------------------------------------------------
static BOOL check_output(const OUTPUT_INFO *oip, const PRM_ENC *pe);
static void set_enc_prm(PRM_ENC *pe, const OUTPUT_INFO *oip);
static void auto_save_log(const OUTPUT_INFO *oip, const PRM_ENC *pe);
static int get_total_path();

//---------------------------------------------------------------------
//		出力プラグイン内部変数
//---------------------------------------------------------------------

static CONF_GUIEX conf;
static SYSTEM_DATA sys_dat = { 0 };
static char auo_filefilter[1024] = { 0 };
static char auo_path[MAX_PATH_LEN] = { 0 };
static int default_enc_type = ENC_TYPE_X264; //auoのファイル名からどちらがデフォルトかを決定

//---------------------------------------------------------------------
//		出力プラグイン構造体定義
//---------------------------------------------------------------------
OUTPUT_PLUGIN_TABLE output_plugin_table = {
	NULL,                         // フラグ
	AUO_FULL_NAME_X264,           // プラグインの名前
	AUO_EXT_FILTER,               // 出力ファイルのフィルタ
	AUO_VERSION_INFO_X264,        // プラグインの情報
	func_init,                    // DLL開始時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_exit,                    // DLL終了時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_output,                  // 出力時に呼ばれる関数へのポインタ
	func_config,                  // 出力設定のダイアログを要求された時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_config_get,              // 出力設定データを取得する時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
	func_config_set,              // 出力設定データを設定する時に呼ばれる関数へのポインタ (NULLなら呼ばれません)
};

//---------------------------------------------------------------------
//		エントリポイント
//---------------------------------------------------------------------
#pragma warning( push )
#pragma warning( disable: 4100 ) //C4100 : 引数は関数の本体部で 1 度も参照されません。
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	if (fdwReason == DLL_PROCESS_ATTACH) {
		//auoファイル名からx264モードか、x265モードかを決定する
		GetModuleFileName(hinstDLL, auo_path, _countof(auo_path));
		const char *dll_name = PathFindFileName(auo_path);
		default_enc_type = (NULL != stristr(dll_name, "x264guiEx")) ? ENC_TYPE_X264 : ENC_TYPE_X265;
		auo_name         = (default_enc_type == ENC_TYPE_X264) ? AUO_NAME_X264         : AUO_NAME_X265;
		auo_name_w       = (default_enc_type == ENC_TYPE_X264) ? AUO_NAME_X264_W       : AUO_NAME_X265_W;
		auo_full_name    = (default_enc_type == ENC_TYPE_X264) ? AUO_FULL_NAME_X264    : AUO_FULL_NAME_X265;
		auo_version_name = (default_enc_type == ENC_TYPE_X264) ? AUO_VERSION_NAME_X264 : AUO_VERSION_NAME_X265;
		auo_version_info = (default_enc_type == ENC_TYPE_X264) ? AUO_VERSION_INFO_X264 : AUO_VERSION_INFO_X265;
		output_plugin_table.name        = (char *)auo_full_name;
		output_plugin_table.information = (char *)auo_version_info;
	}
	return TRUE;
}
#pragma warning( pop )

//---------------------------------------------------------------------
//		出力プラグイン構造体のポインタを渡す関数
//---------------------------------------------------------------------
EXTERN_C OUTPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetOutputPluginTable( void )
{
	init_SYSTEM_DATA(&sys_dat);
	make_file_filter(NULL, 0, sys_dat.exstg->s_local.default_output_ext);
	overwrite_aviutl_ini_file_filter(sys_dat.exstg->s_local.default_output_ext);
	output_plugin_table.filefilter = auo_filefilter;
	return &output_plugin_table;
}


//---------------------------------------------------------------------
//		出力プラグイン出力関数
//---------------------------------------------------------------------
//
	//int		flag;			//	フラグ
	//						//	OUTPUT_INFO_FLAG_VIDEO	: 画像データあり
	//						//	OUTPUT_INFO_FLAG_AUDIO	: 音声データあり
	//						//	OUTPUT_INFO_FLAG_BATCH	: バッチ出力中
	//int		w,h;			//	縦横サイズ
	//int		rate,scale;		//	フレームレート
	//int		n;				//	フレーム数
	//int		size;			//	１フレームのバイト数
	//int		audio_rate;		//	音声サンプリングレート
	//int		audio_ch;		//	音声チャンネル数
	//int		audio_n;		//	音声サンプリング数
	//int		audio_size;		//	音声１サンプルのバイト数
	//LPSTR	savefile;		//	セーブファイル名へのポインタ
	//void	*(*func_get_video)( int frame );
	//						//	DIB形式(RGB24bit)の画像データへのポインタを取得します。
	//						//	frame	: フレーム番号
	//						//	戻り値	: データへのポインタ
	//						//			  画像データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効
	//void	*(*func_get_audio)( int start,int length,int *readed );
	//						//	16bitPCM形式の音声データへのポインタを取得します。
	//						//	start	: 開始サンプル番号
	//						//	length	: 読み込むサンプル数
	//						//	readed	: 読み込まれたサンプル数
	//						//	戻り値	: データへのポインタ
	//						//			  音声データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効
	//BOOL	(*func_is_abort)( void );
	//						//	中断するか調べます。
	//						//	戻り値	: TRUEなら中断
	//BOOL	(*func_rest_time_disp)( int now,int total );
	//						//	残り時間を表示させます。
	//						//	now		: 処理しているフレーム番号
	//						//	total	: 処理する総フレーム数
	//						//	戻り値	: TRUEなら成功
	//int		(*func_get_flag)( int frame );
	//						//	フラグを取得します。
	//						//	frame	: フレーム番号
	//						//	戻り値	: フラグ
	//						//  OUTPUT_INFO_FRAME_FLAG_KEYFRAME		: キーフレーム推奨
	//						//  OUTPUT_INFO_FRAME_FLAG_COPYFRAME	: コピーフレーム推奨
	//BOOL	(*func_update_preview)( void );
	//						//	プレビュー画面を更新します。
	//						//	最後にfunc_get_videoで読み込まれたフレームが表示されます。
	//						//	戻り値	: TRUEなら成功
	//void	*(*func_get_video_ex)( int frame,DWORD format );
	//						//	DIB形式の画像データを取得します。
	//						//	frame	: フレーム番号
	//						//	format	: 画像フォーマット( NULL = RGB24bit / 'Y''U''Y''2' = YUY2 / 'Y''C''4''8' = PIXEL_YC )
	//						//			  ※PIXEL_YC形式 は YUY2フィルタモードでは使用出来ません。
	//						//	戻り値	: データへのポインタ
	//						//			  画像データポインタの内容は次に外部関数を使うかメインに処理を戻すまで有効

BOOL func_init() 
{
	return TRUE;
}

BOOL func_exit() 
{
	if (parallel_task_close())
		return FALSE;
	delete_SYSTEM_DATA(&sys_dat);
	return TRUE;
}

BOOL func_output( OUTPUT_INFO *oip ) 
{
	AUO_RESULT ret = AUO_RESULT_SUCCESS;
	static const encode_task task[3][2] = { { video_output, audio_output }, { audio_output, video_output }, { audio_output_parallel, video_output }  };
	PRM_ENC pe = { 0 };
	const DWORD tm_start_enc = timeGetTime();

	//データの初期化
	init_SYSTEM_DATA(&sys_dat);
	if (!sys_dat.exstg->get_init_success()) return FALSE;

	//ログウィンドウを開く
	open_log_window(oip->savefile, 1, get_total_path());
	//各種設定を行う
	set_enc_prm(&pe, oip);
	set_prevent_log_close(TRUE); //※1 start
	pe.h_p_aviutl = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId()); //※2 start
	ret |= parallel_task_check(&conf, oip, &pe, &sys_dat);

	//チェックを行い、エンコード可能ならエンコードを開始する
	if (!ret && check_output(oip, &pe) && setup_afsvideo(oip, &conf, &pe, sys_dat.exstg->s_local.auto_afs_disable)) { //※3 start

		ret |= run_bat_file(&conf, oip, &pe, &sys_dat, RUN_BAT_BEFORE);

		for (int i = 0; !ret && i < 2; i++)
			ret |= task[conf.aud.audio_encode_timing][i](&conf, oip, &pe, &sys_dat);

		int amp_result = 1;
		do {
			if (!ret) ret |= task[0][amp_result-1](&conf, oip, &pe, &sys_dat); //再エンコ、ただしもう終了している場合、再エンコされない
			if (!ret) ret |= mux(&conf, oip, &pe, &sys_dat);
		} while (!ret && 0 < (amp_result = amp_check_file(&conf, &sys_dat, &pe, oip))); //再エンコードの必要があるかをチェック

		ret |= move_temporary_files(&conf, &pe, &sys_dat, oip, ret);

		write_log_auo_enc_time("総エンコード時間  ", timeGetTime() - tm_start_enc);

		close_afsvideo(&pe); //※3 end

	} else {
		if (!ret) ret |= AUO_RESULT_ERROR;
	}

	if (ret & AUO_RESULT_ABORT) info_encoding_aborted();

	CloseHandle(pe.h_p_aviutl); //※2 end
	set_prevent_log_close(FALSE); //※1 end
	auto_save_log(oip, &pe); //※1 end のあとで行うこと

	if (!(ret & (AUO_RESULT_ERROR | AUO_RESULT_ABORT)))
		ret |= run_bat_file(&conf, oip, &pe, &sys_dat, RUN_BAT_AFTER);

	ret |= parallel_task_add(&conf, oip, &pe, &sys_dat, ret);

	log_process_events();
	return (ret & AUO_RESULT_ERROR) ? FALSE : TRUE;
}

//---------------------------------------------------------------------
//		出力プラグイン設定関数
//---------------------------------------------------------------------
//以下部分的にwarning C4100を黙らせる
//C4100 : 引数は関数の本体部で 1 度も参照されません。
#pragma warning( push )
#pragma warning( disable: 4100 )
BOOL func_config(HWND hwnd, HINSTANCE dll_hinst)
{
	init_SYSTEM_DATA(&sys_dat);
	if (sys_dat.exstg->get_init_success())
		ShowfrmConfig(&conf, &sys_dat);
	return TRUE;
}
#pragma warning( pop )

int func_config_get(void *data, int size)
{
	if (data && size == sizeof(CONF_GUIEX)) {
		if (sys_dat.exstg->s_local.enable_process_parallel) {
			parallel_task_set_unused_parallel_info(conf.vid.parallel_div_info, _countof(conf.vid.parallel_div_info));
		}
		memcpy(data, &conf, sizeof(conf));
	}
	return sizeof(conf);
}

int func_config_set(void *data,int size)
{
	init_SYSTEM_DATA(&sys_dat);
	if (!sys_dat.exstg->get_init_success(TRUE))
		return NULL;
	init_CONF_GUIEX(&conf, FALSE);
	return (guiEx_config::adjust_conf_size(&conf, data, size)) ? size : NULL;
}


//---------------------------------------------------------------------
//		x265guiExのその他の関数
//---------------------------------------------------------------------
void init_SYSTEM_DATA(SYSTEM_DATA *_sys_dat) {
	if (_sys_dat->init)
		return;
	get_auo_path(_sys_dat->auo_path, _countof(_sys_dat->auo_path));
	get_aviutl_dir(_sys_dat->aviutl_dir, _countof(_sys_dat->aviutl_dir));
	_sys_dat->exstg = new guiEx_settings();
	set_ex_stg_ptr(_sys_dat->exstg);
	_sys_dat->init = TRUE;
}
void delete_SYSTEM_DATA(SYSTEM_DATA *_sys_dat) {
	if (_sys_dat->init) {
		delete _sys_dat->exstg;
		_sys_dat->exstg = NULL;
		set_ex_stg_ptr(_sys_dat->exstg);
	}
	_sys_dat->init = FALSE;
}
void init_CONF_GUIEX(CONF_GUIEX *conf, BOOL use_highbit) {
	ZeroMemory(conf, sizeof(CONF_GUIEX));
	guiEx_config::write_conf_header(conf);
	for (int i_enctype = 0; i_enctype < 2; i_enctype++)
		get_default_conf(&conf->x26x[i_enctype], use_highbit, i_enctype);
	conf->vid.enc_type = default_enc_type;
	conf->aud.encoder = sys_dat.exstg->s_local.default_audio_encoder;
	conf->size_all = CONF_INITIALIZED;
}
//最後に"\"なしで戻る
void get_aviutl_dir(char *aviutl_dir, size_t nSize) {
	GetModuleFileNameA(NULL, aviutl_dir, (DWORD)nSize);
	PathRemoveFileSpecFixed(aviutl_dir);
}
void get_aviutl_dir(WCHAR *aviutl_dir, size_t nSize) {
	GetModuleFileNameW(NULL, aviutl_dir, (DWORD)nSize);
	PathRemoveFileSpecFixed(aviutl_dir);
}
void get_auo_path(char *auo_path, size_t nSize) {
	GetModuleFileNameA(GetModuleHandleA(auo_name), auo_path, (DWORD)nSize);
}
void get_auo_path(WCHAR *auo_path, size_t nSize) {
	GetModuleFileNameW(GetModuleHandleW(auo_name_w), auo_path, (DWORD)nSize);
}
void write_log_line_fmt(int log_type_index, const char *format, ...) {
	va_list args;
	int len;
	char *buffer;
	va_start(args, format);
	len = _vscprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
	buffer = (char *)malloc(len * sizeof(buffer[0]));
	vsprintf_s(buffer, len, format, args);
	write_log_line(log_type_index, buffer);
	free(buffer);
}
void write_log_auo_line_fmt(int log_type_index, const char *format, ... ) {
	va_list args;
	int len;
	char *buffer;
	va_start(args, format);
	len = _vscprintf(format, args) + 1; // _vscprintf doesn't count terminating '\0'
	buffer = (char *)malloc(len * sizeof(buffer[0]));
	vsprintf_s(buffer, len, format, args);
	write_log_auo_line(log_type_index, buffer);
	free(buffer);
}
//エンコード時間の表示
void write_log_auo_enc_time(const char *mes, DWORD time) {
	time = ((time + 50) / 100) * 100; //四捨五入
	write_log_auo_line_fmt(LOG_INFO, "%s : %d時間%2d分%2d.%1d秒", 
		mes, 
		time / (60*60*1000),
		(time % (60*60*1000)) / (60*1000), 
		(time % (60*1000)) / 1000,
		((time % 1000)) / 100);
}

static BOOL check_muxer_exist(MUXER_SETTINGS *muxer_stg) {
	if (PathFileExists(muxer_stg->fullpath)) 
		return TRUE;
	error_no_exe_file(muxer_stg->filename, muxer_stg->fullpath);
	return FALSE;
}

static BOOL check_if_exe_is_mp4box(const char *exe_path, const char *version_arg) {
	BOOL ret = FALSE;
	char exe_message[8192] = { 0 };
	if (   PathFileExists(exe_path)
		&& RP_SUCCESS == get_exe_message(exe_path, version_arg, exe_message, _countof(exe_message), AUO_PIPE_MUXED)
		&& (stristr(exe_message, "mp4box") || stristr(exe_message, "GPAC"))) {
		ret = TRUE;
	}
	return ret;
}

static BOOL check_if_exe_is_lsmash(const char *exe_path, const char *version_arg) {
	BOOL ret = FALSE;
	char exe_message[8192] = { 0 };
	if (   PathFileExists(exe_path)
		&& RP_SUCCESS == get_exe_message(exe_path, version_arg, exe_message, _countof(exe_message), AUO_PIPE_MUXED)
		&& stristr(exe_message, "L-SMASH")) {
		ret = TRUE;
	}
	return ret;
}

static BOOL check_muxer_matched_with_ini() {
	BOOL ret = TRUE;
	//不確定な場合は"0", mp4boxなら"-1", L-SMASHなら"1"
	int ini_muxer_mode = (NULL != stristr(sys_dat.exstg->s_mux[MUXER_MP4].filename, "remuxer"))
		               - (NULL != stristr(sys_dat.exstg->s_mux[MUXER_MP4].filename, "mp4box"));
	int exe_muxer_mode = (FALSE != check_if_exe_is_lsmash(sys_dat.exstg->s_mux[MUXER_MP4].fullpath, "--version"))
		               - (FALSE != check_if_exe_is_mp4box(sys_dat.exstg->s_mux[MUXER_MP4].fullpath, "-version"));
	//互いに明確に相反する場合にエラーを出す
	if (ini_muxer_mode * exe_muxer_mode < 0) {
		error_mp4_muxer_unmatch_of_ini_and_exe(0 < exe_muxer_mode);
		ret = FALSE;
	}
	return ret;
}

static BOOL check_amp() {
	BOOL check = TRUE;
	if (!conf.x26x[conf.vid.enc_type].use_auto_npass)
		return check;
	if (conf.vid.amp[conf.vid.enc_type].check & AMPLIMIT_BITRATE) {
		//if (conf.x264.bitrate > conf.vid.amp[conf.vid.enc_type].limit_bitrate) {
		//	check = FALSE; error_amp_bitrate_confliction();
		//} else if (conf.vid.amp[conf.vid.enc_type].limit_bitrate <= 0.0)
		//	conf.vid.amp_check &= ~AMPLIMIT_BITRATE; //フラグを折る
		if (conf.vid.amp[conf.vid.enc_type].limit_bitrate <= 0.0)
			conf.vid.amp[conf.vid.enc_type].check &= ~AMPLIMIT_BITRATE; //フラグを折る
	}
	if (conf.vid.amp[conf.vid.enc_type].check & AMPLIMIT_FILE_SIZE) {
		if (conf.vid.amp[conf.vid.enc_type].limit_file_size <= 0.0)
			conf.vid.amp[conf.vid.enc_type].check &= ~AMPLIMIT_FILE_SIZE; //フラグを折る
	}
	if (conf.vid.amp[conf.vid.enc_type].check && conf.vid.afs && AUDIO_DELAY_CUT_ADD_VIDEO == conf.aud.delay_cut) {
		check = FALSE; error_amp_afs_audio_delay_confliction();
	}
	return check;
}

static BOOL check_output(const OUTPUT_INFO *oip, const PRM_ENC *pe) {
	BOOL check = TRUE;

	//ファイル名長さ
	if (strlen(oip->savefile) > (MAX_PATH_LEN - MAX_APPENDIX_LEN - 1)) {
		error_filename_too_long();
		check = FALSE;
	}

	//解像度
	int w_mul = 1, h_mul = 1;
	switch (conf.x26x[conf.vid.enc_type].output_csp) {
		case OUT_CSP_YUV444:
		case OUT_CSP_RGB:
			w_mul = 1, h_mul = 1; break;
		case OUT_CSP_NV16:
			w_mul = 2, h_mul = 1; break;
		case OUT_CSP_NV12:
		case OUT_CSP_YV12:
		default:
			w_mul = 2; h_mul = 2; break;
	}
	if (conf.x26x[conf.vid.enc_type].interlaced) h_mul *= 2;
	if (oip->w % w_mul) {
		error_invalid_resolution(TRUE,  w_mul, oip->w, oip->h);
		check = FALSE;
	}
	if (oip->h % h_mul) {
		error_invalid_resolution(FALSE, h_mul, oip->w, oip->h);
		check = FALSE;
	}

	//出力するもの
	if (pe->video_out_type == VIDEO_OUTPUT_DISABLED && !(oip->flag & OUTPUT_INFO_FLAG_AUDIO)) {
		error_nothing_to_output();
		check = FALSE;
	}

	if (conf.oth.out_audio_only)
		write_log_auo_line(LOG_INFO, "音声のみ出力を行います。");

	//必要な実行ファイル
	if (!conf.oth.disable_guicmd) {
		const char *x26xfullpath = (8 < conf.x26x[conf.vid.enc_type].bit_depth) ? sys_dat.exstg->s_x26x[conf.vid.enc_type].fullpath_highbit : sys_dat.exstg->s_x26x[conf.vid.enc_type].fullpath;
		if (pe->video_out_type != VIDEO_OUTPUT_DISABLED && !PathFileExists(x26xfullpath)) {
			error_no_exe_file((conf.vid.enc_type == ENC_TYPE_X264) ? "x264.exe" : "x265.exe", x26xfullpath);
			check = FALSE;
		}
	}

	//音声エンコーダ
	if (oip->flag & OUTPUT_INFO_FLAG_AUDIO) {
		AUDIO_SETTINGS *aud_stg = &sys_dat.exstg->s_aud[conf.aud.encoder];
		if (str_has_char(aud_stg->filename) && !PathFileExists(aud_stg->fullpath)) {
			//fawの場合はfaw2aacがあればOKだが、それもなければエラー
			if (!(conf.aud.encoder == sys_dat.exstg->s_aud_faw_index && check_if_faw2aac_exists())) {
				error_no_exe_file(aud_stg->filename, aud_stg->fullpath);
				check = FALSE;
			}
		}
	}

	//muxer
	switch (pe->muxer_to_be_used) {
		case MUXER_TC2MP4:
			check &= check_muxer_exist(&sys_dat.exstg->s_mux[MUXER_MP4]); //tc2mp4使用時は追加でmp4boxも必要
			//下へフォールスルー
		case MUXER_MP4:
			check &= check_muxer_matched_with_ini();
		case MUXER_MKV:
			check &= check_muxer_exist(&sys_dat.exstg->s_mux[pe->muxer_to_be_used]);
			break;
		default:
			break;
	}

	//自動マルチパス設定
	check &= check_amp();

	//オーディオディレイカット
	if (conf.vid.afs && AUDIO_DELAY_CUT_ADD_VIDEO == conf.aud.delay_cut) {
		info_afs_audio_delay_confliction();
		conf.aud.audio_encode_timing = 0;
	}

	return check;
}

void open_log_window(const char *savefile, int current_pass, int total_pass) {
	char mes[MAX_PATH_LEN + 512];
	char *newLine = (get_current_log_len(current_pass)) ? "\r\n\r\n" : ""; //必要なら行送り
	static const char *SEPARATOR = "------------------------------------------------------------------------------------------------------------------------------";
	if (total_pass < 2 || current_pass > total_pass)
		sprintf_s(mes, sizeof(mes), "%s%s\r\n[%s]\r\n%s", newLine, SEPARATOR, savefile, SEPARATOR);
	else
		sprintf_s(mes, sizeof(mes), "%s%s\r\n[%s] (%d / %d pass)\r\n%s", newLine, SEPARATOR, savefile, current_pass, total_pass, SEPARATOR);
	
	show_log_window(sys_dat.aviutl_dir, sys_dat.exstg->s_local.disable_visual_styles);
	write_log_line(LOG_INFO, mes);
}

static void set_tmpdir(PRM_ENC *pe, int tmp_dir_index, const char *savefile) {
	if (tmp_dir_index < TMP_DIR_OUTPUT || TMP_DIR_CUSTOM < tmp_dir_index)
		tmp_dir_index = TMP_DIR_OUTPUT;

	if (tmp_dir_index == TMP_DIR_SYSTEM) {
		//システムの一時フォルダを取得
		if (GetTempPath(_countof(pe->temp_filename), pe->temp_filename) != NULL) {
			PathRemoveBackslash(pe->temp_filename);
			write_log_auo_line_fmt(LOG_INFO, "一時フォルダ : %s", pe->temp_filename);
		} else {
			warning_failed_getting_temp_path();
			tmp_dir_index = TMP_DIR_OUTPUT;
		}
	}
	if (tmp_dir_index == TMP_DIR_CUSTOM) {
		//指定されたフォルダ
		if (DirectoryExistsOrCreate(sys_dat.exstg->s_local.custom_tmp_dir)) {
			strcpy_s(pe->temp_filename, _countof(pe->temp_filename), sys_dat.exstg->s_local.custom_tmp_dir);
			PathRemoveBackslash(pe->temp_filename);
			write_log_auo_line_fmt(LOG_INFO, "一時フォルダ : %s", pe->temp_filename);
		} else {
			warning_no_temp_root(sys_dat.exstg->s_local.custom_tmp_dir);
			tmp_dir_index = TMP_DIR_OUTPUT;
		}
	}
	if (tmp_dir_index == TMP_DIR_OUTPUT) {
		//出力フォルダと同じ("\"なし)
		strcpy_s(pe->temp_filename, _countof(pe->temp_filename), savefile);
		PathRemoveFileSpecFixed(pe->temp_filename);
	}
}

static int get_total_path() {
	return (conf.x26x[conf.vid.enc_type].use_auto_npass
		 && conf.x26x[conf.vid.enc_type].rc_mode == X26X_RC_BITRATE
		 && !conf.oth.disable_guicmd)
		 ? conf.x26x[conf.vid.enc_type].auto_npass : 1;
}

static void set_aud_delay_cut(PRM_ENC *pe, const OUTPUT_INFO *oip) {
	pe->delay_cut_additional_vframe = 0;
	pe->delay_cut_additional_aframe = 0;
	if (oip->flag & OUTPUT_INFO_FLAG_AUDIO) {
		int audio_delay = sys_dat.exstg->s_aud[conf.aud.encoder].mode[conf.aud.enc_mode].delay;
		if (audio_delay) {
			const double fps = oip->rate / (double)oip->scale;
			const int audio_rate = oip->audio_rate;
			switch (conf.aud.delay_cut) {
			case AUDIO_DELAY_CUT_DELETE_AUDIO:
				pe->delay_cut_additional_aframe = -1 * audio_delay;
				break;
			case AUDIO_DELAY_CUT_ADD_VIDEO:
				pe->delay_cut_additional_vframe = additional_vframe_for_aud_delay_cut(fps, audio_rate, audio_delay);
				pe->delay_cut_additional_aframe = additional_silence_for_aud_delay_cut(fps, audio_rate, audio_delay);
				break;
			case AUDIO_DELAY_CUT_NONE:
			default:
				conf.aud.delay_cut = AUDIO_DELAY_CUT_NONE;
				break;
			}
		} else {
			conf.aud.delay_cut = AUDIO_DELAY_CUT_NONE;
		}
	}
}

static void set_enc_prm(PRM_ENC *pe, const OUTPUT_INFO *oip) {
	//初期化
	ZeroMemory(pe, sizeof(PRM_ENC));
	//設定更新
	sys_dat.exstg->load_encode_stg();
	sys_dat.exstg->load_append();
	sys_dat.exstg->load_fn_replace();

	pe->video_out_type = check_video_ouput(&conf, oip);
	pe->muxer_to_be_used = check_muxer_to_be_used(&conf, pe->video_out_type, (oip->flag & OUTPUT_INFO_FLAG_AUDIO) != 0);
	pe->total_pass = get_total_path();
	pe->amp_pass_limit = pe->total_pass + sys_dat.exstg->s_local.amp_retry_limit;
	pe->current_pass = 1;
	pe->drop_count = 0;
	memcpy(&pe->append, &sys_dat.exstg->s_append, sizeof(FILE_APPENDIX));
	ZeroMemory(&pe->append.aud, sizeof(pe->append.aud));

	char filename_replace[MAX_PATH_LEN];

	//一時フォルダの決定
	set_tmpdir(pe, conf.oth.temp_dir, oip->savefile);

	//音声一時フォルダの決定
	char *cus_aud_tdir = pe->temp_filename;
	if (conf.aud.aud_temp_dir) {
		if (DirectoryExistsOrCreate(sys_dat.exstg->s_local.custom_audio_tmp_dir)) {
			cus_aud_tdir = sys_dat.exstg->s_local.custom_audio_tmp_dir;
			write_log_auo_line_fmt(LOG_INFO, "音声一時フォルダ : %s", cus_aud_tdir);
		} else {
			warning_no_aud_temp_root(sys_dat.exstg->s_local.custom_audio_tmp_dir);
		}
	}
	strcpy_s(pe->aud_temp_dir, _countof(pe->aud_temp_dir), cus_aud_tdir);

	//ファイル名置換を行い、一時ファイル名を作成
	strcpy_s(filename_replace, _countof(filename_replace), PathFindFileName(oip->savefile));
	sys_dat.exstg->apply_fn_replace(filename_replace, _countof(filename_replace));
	PathCombineLong(pe->temp_filename, _countof(pe->temp_filename), pe->temp_filename, filename_replace);
	
	//FAWチェックとオーディオディレイの修正
	if (conf.aud.faw_check)
		auo_faw_check(&conf.aud, oip, pe, sys_dat.exstg);
	set_aud_delay_cut(pe, oip);
}

static void auto_save_log(const OUTPUT_INFO *oip, const PRM_ENC *pe) {
	guiEx_settings ex_stg(true);
	ex_stg.load_log_win();
	if (!ex_stg.s_log.auto_save_log)
		return;
	char log_file_path[MAX_PATH_LEN];
	if (AUO_RESULT_SUCCESS != getLogFilePath(log_file_path, _countof(log_file_path), pe, &sys_dat, &conf, oip))
		warning_no_auto_save_log_dir();
	auto_save_log_file(log_file_path);
	return;
}

void overwrite_aviutl_ini_file_filter(int idx) {
	char ini_file[1024];
	get_aviutl_dir(ini_file, _countof(ini_file));
	PathAddBackSlashLong(ini_file);
	strcat_s(ini_file, _countof(ini_file), "aviutl.ini");
	
	char filefilter_ini[1024] = { 0 };
	make_file_filter(filefilter_ini, _countof(filefilter_ini), idx);
	WritePrivateProfileString(auo_name, "filefilter", filefilter_ini, ini_file);
}

void make_file_filter(char *filter, size_t nSize, int default_index) {
	static const char *const TOP = "All Support Formats (*.*)";
	const char separator = (filter) ? '\\' : '\0';
	if (filter == NULL) {
		filter = auo_filefilter;
		nSize = _countof(auo_filefilter);
	}
	char *ptr = filter;
	
#define ADD_FILTER(str, appendix) { \
	size_t len = strlen(str); \
	if (nSize - (ptr - filter) <= len + 1) return; \
	memcpy(ptr, (str), sizeof(ptr[0]) * len); \
	ptr += len; \
	*ptr = (appendix); \
	ptr++; \
}
#define ADD_DESC(idx) { \
	size_t len = sprintf_s(ptr, nSize - (ptr - filter), "%s (%s)", OUTPUT_FILE_EXT_DESC[idx], OUTPUT_FILE_EXT_FILTER[idx]); \
	ptr += len; \
	*ptr = separator; \
	ptr++; \
	len = strlen(OUTPUT_FILE_EXT_FILTER[idx]); \
	if (nSize - (ptr - filter) <= len + 1) return; \
	memcpy(ptr, OUTPUT_FILE_EXT_FILTER[idx], sizeof(ptr[0]) * len); \
	ptr += len; \
	*ptr = separator; \
	ptr++; \
}
	ADD_FILTER(TOP, separator);
	ADD_FILTER(OUTPUT_FILE_EXT_FILTER[default_index], ';');
	for (int idx = 0; idx < _countof(OUTPUT_FILE_EXT_FILTER); idx++)
		if (idx != default_index)
			ADD_FILTER(OUTPUT_FILE_EXT_FILTER[idx], ';');
	ADD_FILTER(OUTPUT_FILE_EXT_FILTER[default_index], separator);
	ADD_DESC(default_index);
	for (int idx = 0; idx < _countof(OUTPUT_FILE_EXT_FILTER); idx++)
		if (idx != default_index)
			ADD_DESC(idx);
	ptr[0] = '\0';
#undef ADD_FILTER
#undef ADD_DESC
}

int create_auoSetup(const char *exePath) {
	int ret = AUO_RESULT_SUCCESS;
	//リソースを取り出し
	HRSRC hResource = NULL;
	HGLOBAL hResourceData = NULL;
	const char *pDataPtr = NULL;
	DWORD resourceSize = 0;
	FILE *fp = NULL;
	HMODULE hModule = GetModuleHandleA(auo_name);
	if (   NULL == (hResource = FindResource(hModule, "AUOSETUP", "EXE_DATA"))
		|| NULL == (hResourceData = LoadResource(hModule, hResource))
		|| NULL == (pDataPtr = (const char *)LockResource(hResourceData))
		|| 0    == (resourceSize = SizeofResource(hModule, hResource))) {
		ret = AUO_RESULT_ERROR;
	} else if (fopen_s(&fp, exePath, "wb") || NULL == fp) {
		ret = AUO_RESULT_ERROR;
	} else if (resourceSize != fwrite(pDataPtr, 1, resourceSize, fp)) {
		ret = AUO_RESULT_ERROR;
	}
	if (fp)
		fclose(fp);
	return ret;
}