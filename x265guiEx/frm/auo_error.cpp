﻿//  -----------------------------------------------------------------------------------------
//    拡張 x264/x265 出力(GUI) Ex  v1.xx/2.xx/3.xx by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  -----------------------------------------------------------------------------------------

#include "auo.h"
#include "auo_frm.h" 
#include "auo_pipe.h"
#include "auo_chapter.h"

void warning_failed_getting_temp_path() {
	write_log_auo_line(LOG_WARNING, "一時フォルダ名取得に失敗しました。一時フォルダ指定を解除しました。");
}

void warning_no_temp_root(const char *dir) {
	write_log_auo_line_fmt(LOG_WARNING, 
		"指定された一時フォルダ \"%s\" が存在しません。一時フォルダ指定を解除しました。",
		dir);
}

void warning_no_aud_temp_root(const char *dir) {
	write_log_auo_line_fmt(LOG_WARNING,
		"指定された音声用一時フォルダ \"%s\" が存在しません。一時フォルダ指定を解除しました。",
		dir);
}

void error_filename_too_long() {
	write_log_auo_line(LOG_ERROR, "出力ファイル名が長すぎます。もっと短くしてください。");
}

void error_nothing_to_output() {
	write_log_auo_line(LOG_ERROR, "出力すべきものがありません。");
}

void error_amp_bitrate_confliction() {
	write_log_auo_line(LOG_ERROR, "上限ビットレートが、目標ビットレートより小さく設定されています。エンコードできません。");
	write_log_auo_line(LOG_ERROR, "ビットレート設定を見なおしてください。");
}

void error_amp_afs_audio_delay_confliction() {
	write_log_auo_line(LOG_ERROR, "上限設定付き自動マルチパス、自動フィールドシフト、音声ディレイカット[動画追加]は同時に指定できません。");
	write_log_auo_line(LOG_ERROR, "エンコードを続行できません。");
}

void info_afs_audio_delay_confliction() {
	write_log_auo_line(LOG_INFO, "自動フィールドシフト、音声ディレイカット[動画追加]が同時に指定されている場合には、音声エンコードは後で行います。");
}

static const char *get_target_limit_name(DWORD target_limit) {
	const char *str_limit = "";
	switch (target_limit) {
		case AMPLIMIT_BITRATE:
			str_limit = "ビットレート";   break;
		case AMPLIMIT_FILE_SIZE:
			str_limit = "ファイルサイズ"; break;
		case (AMPLIMIT_BITRATE & AMPLIMIT_FILE_SIZE):
			str_limit = "ファイルサイズ/ビットレート";   break;
		default: break;
	}
	return str_limit;
}

void info_amp_do_aud_enc_first(DWORD target_limit) {
	write_log_auo_line_fmt(LOG_INFO, "自動マルチパスでの上限%sのチェックを行う場合は、音声エンコードを先に行います。", get_target_limit_name(target_limit));
}

void error_amp_aud_too_big(DWORD target_limit) {
	write_log_auo_line_fmt(LOG_ERROR, "音声ファイルのみで、上限%sの設定をを超えてしまいました。", get_target_limit_name(target_limit));
	write_log_auo_line(    LOG_ERROR, "ビットレートの設定を見なおしてください。");
}

void error_amp_target_bitrate_too_small(DWORD target_limit) {
	write_log_auo_line_fmt(LOG_ERROR, "上限%sの設定を守るには、指定された目標ビットレートは大きすぎます。", get_target_limit_name(target_limit));
	write_log_auo_line(    LOG_ERROR, "ビットレートの設定を見なおしてください。");
}

void warning_amp_change_bitrate(int bitrate_old, int bitrate_new, DWORD target_limit) {
	if (bitrate_old >= 0) {
		write_log_auo_line_fmt(LOG_WARNING, "上限%sの設定を守るには、指定された目標ビットレートは大きすぎます。", get_target_limit_name(target_limit));
		write_log_auo_line_fmt(LOG_WARNING, "目標ビットレートを %d kbps -> %d kbpsに変更します。", bitrate_old, bitrate_new);
	} else {
		//-1は上限確認付crfで使用する
		write_log_auo_line_fmt(LOG_WARNING, "目標ビットレートを %d kbpsに設定します。", bitrate_new);
	}
}

void error_invalid_resolution(BOOL width, int mul, int w, int h) {
	write_log_auo_line_fmt(LOG_ERROR, "%s入力解像度が %d で割りきれません。エンコードできません。入力解像度:%dx%d",
		(width) ? "横" : "縦", mul, w, h);
}

void error_log_line_cache() {
	write_log_auo_line(LOG_ERROR, "ログ保存キャッシュ用メモリ確保に失敗しました。");
}

void error_no_exe_file(const char *name, const char *path) {
	if (strlen(path))
		write_log_auo_line_fmt(LOG_ERROR, "指定された %s が %s にありません。", name, path);
	else
		write_log_auo_line_fmt(LOG_ERROR, "%s の場所が指定されていません。", name);
	write_log_auo_line_fmt(LOG_ERROR, "%s を用意し、その場所を設定画面から正しく指定してください。", name);
}

void warning_auto_afs_disable() {
	write_log_line(LOG_WARNING, ""
		"auo [warning]: Aviutlからの映像入力の初期化に失敗したため、\n"
		"               自動フィールドシフト(afs)をオフにして再初期化を行いました。\n"
		"               この問題は、Aviutlでafsを使用していないにも関わらず、\n"
		"               x265guiEx側でafsをオンにしていると発生します。\n"
		"               他のエラーの可能性も考えられます。afsがオフになっている点に注意してください。"
		);
}

void error_afs_setup(BOOL afs, BOOL auto_afs_disable) {
	if (afs && !auto_afs_disable) {
		write_log_line(LOG_ERROR, ""
			"auo [error]: Aviutlからの映像入力の初期化に失敗しました。以下のような原因が考えられます。\n"
			"             ・自動フィールドシフト(afs)をAviutlで使用していないにもかかわらず、\n"
			"               x265guiExの設定画面で自動フィールドシフトにチェックを入れていたり、\n"
			"               自動フィールドシフト非対応の動画(60fps読み込み等)を入力したりしている。\n"
			"             ・メモリ不足による、メモリ確保の失敗。"
			);
	} else
		write_log_auo_line(LOG_ERROR, "Aviutlからの映像入力の初期化に失敗しました。メモリを確保できませんでした。");
}

void error_open_pipe() {
	write_log_auo_line(LOG_ERROR, "パイプの作成に失敗しました。");
}

void error_get_pipe_handle() {
	write_log_auo_line(LOG_ERROR, "パイプハンドルの取得に失敗しました。");
}

void error_run_process(const char *exe_name, int rp_ret) {
	switch (rp_ret) {
		case RP_ERROR_OPEN_PIPE:
			write_log_auo_line(LOG_ERROR, "パイプの作成に失敗しました。");
			break;
		case RP_ERROR_GET_STDIN_FILE_HANDLE:
			write_log_auo_line(LOG_ERROR, "パイプハンドルの取得に失敗しました。");
			break;
		case RP_ERROR_CREATE_PROCESS:
		default:
			write_log_auo_line_fmt(LOG_ERROR, "%s の実行に失敗しました。", exe_name);
			break;
	}
}

void error_video_output_thread_start() {
	write_log_auo_line(LOG_ERROR, "パイプ出力用スレッドの生成に失敗しました。");
}

void warning_auto_qpfile_failed() {
	write_log_auo_line(LOG_WARNING, "Aviutlのキーフレーム検出用 qpfileの自動作成に失敗しました。");
}

void warning_auo_tcfile_failed() {
	write_log_auo_line(LOG_WARNING, "タイムコードファイル作成に失敗しました。");
}

void error_malloc_pixel_data() {
	write_log_auo_line(LOG_ERROR, "映像バッファ用メモリ確保に失敗しました。");
}

void error_malloc_tc() {
	write_log_auo_line(LOG_ERROR, "タイムコード用メモリ確保に失敗しました。");
}

void error_malloc_8bit() {
	write_log_auo_line(LOG_ERROR, "音声16bit→8bit変換用メモリ確保に失敗しました。");
}

void error_afs_interlace_stg() {
	write_log_line(LOG_ERROR, 
		"auo [error]: 自動フィールドシフトとインターレース設定が両方オンになっており、設定が矛盾しています。\n"
		"             設定を見なおしてください。");
}

void error_x26x_dead(int enc_type) {
	write_log_auo_line_fmt(LOG_ERROR, "%sが予期せず途中終了しました。%sに不正なパラメータ(オプション)が渡された可能性があります。", X26X_NAME[enc_type], X26X_NAME[enc_type]);
}

void error_x26x_version(int enc_type, const char *required_ver, const char *current_ver) {
	write_log_line_fmt(LOG_ERROR, ""
		"auo [error]: %sのバージョンが古く、エンコードできません。\n"
		"             最新の%sをダウンロードし、設定画面で最新版に指定しなおしてください。\n"
		"             必要なバージョン:         %s\n"
		"             実行ファイルのバージョン: %s\n",
		X26X_NAME[enc_type], X26X_NAME[enc_type], required_ver, current_ver);
}

void error_afs_get_frame() {
	write_log_auo_line(LOG_ERROR, "Aviutlからのフレーム読み込みに失敗しました。");
}

void error_open_wavfile() {
	write_log_auo_line(LOG_ERROR, "wavファイルのオープンに失敗しました。");
}

void error_no_wavefile() {
	write_log_auo_line(LOG_ERROR, "wavファイルがみつかりません。音声エンコードに失敗しました。");
}

void warning_audio_length() {
	write_log_line(LOG_WARNING, 
		"auo [warning]: 音声の長さが動画の長さと大きく異なるようです。\n"
		"               これが意図したものでない場合、音声が正常に出力されていないかもしれません。\n"
		"               この問題は圧縮音声をソースとしていると発生することがあります。\n"
		"               一度音声をデコードし、「音声読み込み」から無圧縮wavとして別に読み込むか、\n"
		"               異なる入力プラグインを利用して読み込むといった方法を試してみてください。");
}

void error_audenc_failed(const char *name, const char *args) {
	write_log_auo_line_fmt(LOG_ERROR, "出力音声ファイルがみつかりません。%s での音声のエンコードに失敗しました。", name);
	if (args) {
		write_log_auo_line(    LOG_ERROR, "音声エンコードのコマンドラインは…");
		write_log_auo_line(    LOG_ERROR, args);
	}
}

void error_mux_failed(const char *name, const char *args) {
	write_log_auo_line_fmt(LOG_ERROR, "%s でのmuxに失敗しました。", name);
	write_log_auo_line(    LOG_ERROR, "muxのコマンドラインは…");
	write_log_auo_line(    LOG_ERROR, args);
}

void warning_no_mux_tmp_root(const char *dir) {
	write_log_auo_line_fmt(LOG_WARNING,
		"指定されたmux用一時ドライブ \"%s\" が存在しません。一時フォルダ指定を解除しました。",
		dir);
}

void warning_failed_mux_tmp_drive_space() {
	write_log_auo_line(LOG_WARNING, "指定されたmux用一時フォルダのあるドライブの空き容量取得に失敗しました。mux用一時フォルダ指定を解除しました。");
}

void warning_failed_muxer_drive_space() {
	write_log_auo_line(LOG_WARNING, "muxerのあるドライブの空き容量取得に失敗しました。容量不足によりmuxが失敗する可能性があります。");
}

void warning_failed_out_drive_space() {
	write_log_auo_line(LOG_WARNING, "出力先のあるドライブの空き容量取得に失敗しました。容量不足によりmuxが失敗する可能性があります。");
}

void warning_failed_get_aud_size() {
	write_log_auo_line(LOG_WARNING, "音声一時ファイルのサイズ取得に失敗しました。muxが正常に行えるか確認できません。");
}

void warning_failed_get_vid_size() {
	write_log_auo_line(LOG_WARNING, "映像一時ファイルのサイズ取得に失敗しました。muxが正常に行えるか確認できません。");
}

void error_no_aud_file() {
	write_log_auo_line(LOG_ERROR, "音声一時ファイルが見つかりません。muxを行えません。");
}

void error_no_vid_file() {
	write_log_auo_line(LOG_ERROR, "映像一時ファイルが見つかりません。muxを行えません。");
}

void error_aud_file_zero_byte() {
	write_log_auo_line(LOG_ERROR, "音声一時ファイルが 0 byteです。muxを行えません。");
}

void error_vid_file_zero_byte() {
	write_log_auo_line(LOG_ERROR, "映像一時ファイルが 0 byteです。muxを行えません。");
}

void warning_mux_tmp_not_enough_space() {
	write_log_auo_line(LOG_WARNING, "mux一時フォルダのあるドライブに十分な空きがありません。mux用一時フォルダ指定を解除しました。");
}

void error_muxer_drive_not_enough_space() {
	write_log_auo_line(LOG_ERROR, "muxerのあるドライブに十分な空きがありません。muxを行えません。");
}

void error_out_drive_not_enough_space() {
	write_log_auo_line(LOG_ERROR, "出力先のドライブに十分な空きがありません。muxを行えません。");
}

void warning_failed_to_get_duration_from_timecode() {
	write_log_auo_line(LOG_WARNING, "タイムコードからの動画長さの取得に失敗しました。");
	write_log_auo_line(LOG_WARNING, "Apple形式チャプターに記述する動画長さはAviutlから取得したものを使用します。");
	write_log_auo_line(LOG_WARNING, "そのため、チャプターストリームの長さが実際の動画と異なる恐れがあります。");
}

void error_check_muxout_exist() {
	write_log_auo_line(LOG_ERROR, "mux後ファイルが見つかりませんでした。");
}

void error_check_muxout_too_small(int expected_filesize_KB, int muxout_filesize_KB) {
	write_log_auo_line    (LOG_ERROR, "mux後ファイルが小さすぎます。muxに失敗したものと思われます。");
	write_log_auo_line_fmt(LOG_ERROR, "推定ファイルサイズ %d KB,  出力ファイルサイズ %d KB", expected_filesize_KB, muxout_filesize_KB);
}

void warning_failed_check_muxout_filesize() {
	write_log_auo_line(LOG_WARNING, "mux後ファイルのファイルサイズ確認に失敗しました。正常にmuxされていない可能性があります。");
}

void warning_amp_failed() {
	write_log_auo_line(LOG_WARNING, "自動マルチパスがチェックに失敗しました。指定した上限が守られていない可能性があります。");
}

void warning_amp_filesize_over_limit() {
	write_log_auo_line_fmt(LOG_WARNING, "エンコード中にファイルサイズが上限を上回ってしました。");
	write_log_auo_line_fmt(LOG_WARNING, "エンコードを中断し、設定を調整した後、再エンコードを行います。");
}

void warning_no_auto_save_log_dir() {
	write_log_auo_line(LOG_WARNING, "指定した自動ログ保存先が存在しません。動画出力先に保存します。");
}

void info_encoding_aborted() {
	write_log_auo_line(LOG_INFO, "エンコードを中断しました。");
}

void warning_mux_no_chapter_file() {
	write_log_auo_line(LOG_WARNING, "指定されたチャプターファイルが存在しません。チャプターはmuxされません。");
}

void info_amp_result(DWORD status, int amp_result, UINT64 filesize, double file_bitrate, double limit_filesize, double limit_filebitrate, int retry_count, int new_bitrate) {
	int log_index = (status) ? ((amp_result) ? LOG_WARNING : LOG_ERROR) : LOG_INFO;
	write_log_auo_line_fmt(    log_index, "出力ファイルサイズ %.2f MB, ファイルビットレート %.2f kbps", filesize / (double)(1024*1024), file_bitrate);
	if (status & AMPLIMIT_FILE_SIZE)
		write_log_auo_line_fmt(log_index, "上限ファイルサイズ %.2f MB を上回ってしまいました。", limit_filesize);
	if (status & AMPLIMIT_BITRATE)
		write_log_auo_line_fmt(log_index, "上限ファイルビットレート %.2f kbps を上回ってしまいました。", limit_filebitrate);
	if (status && amp_result)
		if (amp_result == 2)
			write_log_auo_line_fmt(log_index, "音声目標ビットレートを %d kbpsに変更し、再エンコードを行います。", new_bitrate);
		else if (new_bitrate >= 0) //-1は上限確認付crfで使用する
			write_log_auo_line_fmt(log_index, "映像目標ビットレートを %d kbpsに変更し、再エンコードを行います。", new_bitrate);

	if (!status)
		write_log_auo_line_fmt(log_index, "指定された上限を下回っていることを確認しました。");
	else if (!amp_result)
		write_log_auo_line_fmt(log_index, "%d回トライしましたが、いずれも上限を上回ってしまいました。目標ビットレートを見なおしてください。", retry_count);
}

void warning_mux_chapter(int sts) {
	switch (sts) {
		case AUO_CHAP_ERR_NONE: break;
		case AUO_CHAP_ERR_FILE_OPEN:        write_log_auo_line(LOG_WARNING, "チャプターファイルのオープンに失敗しました。"); break;
		case AUO_CHAP_ERR_FILE_READ:        write_log_auo_line(LOG_WARNING, "チャプターファイルの読み込みに失敗しました。"); break;
		case AUO_CHAP_ERR_FILE_WRITE:       write_log_auo_line(LOG_WARNING, "チャプターファイルの書き込みに失敗しました。"); break;
		case AUO_CHAP_ERR_FILE_SWAP:        write_log_auo_line(LOG_WARNING, "チャプターファイル名の交換に失敗しました。"); break;
		case AUO_CHAP_ERR_CP_DETECT:        write_log_auo_line(LOG_WARNING, "チャプターファイルのコードページの判定に失敗しました。"); break;
		case AUO_CHAP_ERR_INIT_IMUL2:       write_log_auo_line(LOG_WARNING, "コードページ変換の初期化に失敗しました。"); break;
		case AUO_CHAP_ERR_INVALID_FMT:      write_log_auo_line(LOG_WARNING, "指定されたチャプターファイルの書式が不正です。"); break;
		case AUO_CHAP_ERR_NULL_PTR:         write_log_auo_line(LOG_WARNING, "ぬるぽ。"); break;
		case AUO_CHAP_ERR_INIT_XML_PARSER:  write_log_auo_line(LOG_WARNING, "Xml Parserの初期化に失敗しました。"); break;
		case AUO_CHAP_ERR_INIT_READ_STREAM: write_log_auo_line(LOG_WARNING, "チャプターファイルのオープンに失敗しました。"); break;
		case AUO_CHAP_ERR_FAIL_SET_STREAM:  write_log_auo_line(LOG_WARNING, "Xml Parserと入力ストリームの接続に失敗しました。"); break;
		case AUO_CHAP_ERR_PARSE_XML:        write_log_auo_line(LOG_WARNING, "チャプターファイルの読み取りに失敗しました。"); break;
		default:                            write_log_auo_line(LOG_WARNING, "チャプターmux: 不明なエラーが発生しました。"); break;
	}
	return;
}

void error_select_convert_func(int width, int height, int bit_depth, BOOL interlaced, int output_csp) {
	const char *bit_depth_str = "";
	switch (bit_depth) {
	case 16: bit_depth_str = "(16bit)"; break;
	case 12: bit_depth_str = "(12bit)"; break;
	case 10: bit_depth_str = "(10bit)"; break;
	default: break;
	}
	write_log_auo_line(    LOG_ERROR, "色形式変換関数の取得に失敗しました。");
	write_log_auo_line_fmt(LOG_ERROR, "%dx%d%s, output-csp %s%s",
		width, height,
		(interlaced) ? "i" : "p",
		specify_csp[output_csp],
		bit_depth_str
		);
}

void warning_no_batfile(const char *batfile) {
	write_log_auo_line_fmt(LOG_WARNING, "指定されたバッチファイル \"%s\"が存在しません。", batfile);
}

void warning_malloc_batfile_tmp() {
	write_log_auo_line(LOG_WARNING, "一時バッチファイル作成用バッファの確保に失敗しました。");
}

void warning_failed_open_bat_orig() {
	write_log_auo_line(LOG_WARNING, "バッチファイルを開けませんでした。");
}

void warning_failed_open_bat_new() {
	write_log_auo_line(LOG_WARNING, "一時バッチファイルを作成できませんでした。");
}
