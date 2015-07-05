﻿//  -----------------------------------------------------------------------------------------
//    拡張 x264/x265 出力(GUI) Ex  v1.xx/2.xx/3.xx by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  -----------------------------------------------------------------------------------------

#include "frmLog.h"
#include "string.h"

#include "auo_version.h"

using namespace x265guiEx;

//すべてfrmLog::Instanceを通じてアクセス

//ログウィンドウを表示させる
[STAThreadAttribute]
void show_log_window(const char *aviutl_dir, BOOL disable_visual_styles) {
    if (!disable_visual_styles)
        System::Windows::Forms::Application::EnableVisualStyles();
    System::IO::Directory::SetCurrentDirectory(String(aviutl_dir).ToString());
    frmLog::Instance::get()->Show();
    frmLog::Instance::get()->SetWindowTitle(AUO_FULL_NAME, PROGRESSBAR_DISABLED);
}
//ログウィンドウのタイトルを設定
[STAThreadAttribute]
void set_window_title(const char *chr) {
    if (!frmLog::Instance::get()->InvokeRequired)
        frmLog::Instance::get()->SetWindowTitle(chr);
}
[STAThreadAttribute]
void set_window_title(const char *chr, int progress_mode) {
    if (!frmLog::Instance::get()->InvokeRequired)
        frmLog::Instance::get()->SetWindowTitle(chr, progress_mode);
}
//メッセージをログウィンドウのタイトルに
[STAThreadAttribute]
void set_window_title_enc_mes(const char *chr, int total_drop, int frame_n) {
    frmLog::Instance::get()->SetWindowTitleX264Mes(chr, total_drop, frame_n);
}
//メッセージをログウィンドウに表示
[STAThreadAttribute]
void write_log_auo_line(int log_type_index, const char *chr, bool from_utf8) {
    frmLog::Instance::get()->WriteLogAuoLine((from_utf8) ? Utf8toString(chr) : String(chr).ToString(), log_type_index);
}
//現在実行中の内容の設定
[STAThreadAttribute]
void set_task_name(const char *chr) {
    if (!frmLog::Instance::get()->InvokeRequired)
        frmLog::Instance::get()->SetTaskName(chr);
}
//進捗情報の表示
[STAThreadAttribute]
void set_log_progress(double progress) {
    if (!frmLog::Instance::get()->InvokeRequired)
        frmLog::Instance::get()->SetProgress(progress);
}
//メッセージを直接ログウィンドウに表示
[STAThreadAttribute]
void write_log_line(int log_type_index, const char *chr, bool from_utf8) {
    frmLog::Instance::get()->WriteLogLine((from_utf8) ? Utf8toString(chr) : String(chr).ToString(), log_type_index);
}
//音声を並列に処理する際に、蓄えた音声のログを表示
//必ず音声処理が動いていないところで呼ぶこと!
void flush_audio_log() {
    frmLog::Instance::get()->FlushAudioLogCache();
}
//ログウィンドウからのx264制御を有効化
[STAThreadAttribute]
void enable_x264_control(DWORD *priority, BOOL *enc_pause, BOOL afs, BOOL add_progress, DWORD start_time, int _total_frame) {
    frmLog::Instance::get()->Enablex264Control(priority, enc_pause, afs, add_progress, start_time, _total_frame);
}
//ログウィンドウからのx264制御を無効化
[STAThreadAttribute]
void disable_x264_control() {
    frmLog::Instance::get()->Disablex264Control();
}
//ログウィンドウを閉じられるかどうかを設定
[STAThreadAttribute]
void set_prevent_log_close(BOOL prevent) {
    frmLog::Instance::get()->SetPreventLogWindowClosing(prevent);
}
//自動ログ保存を実行
[STAThreadAttribute]
void auto_save_log_file(const char *log_filepath) {
    frmLog::Instance::get()->AutoSaveLogFile(log_filepath);
}
//ログウィンドウに設定を再ロードさせる
[STAThreadAttribute]
void log_reload_settings() {
    frmLog::Instance::get()->ReloadLogWindowSettings();
}
//ログウィンドウにイベントを実行させる
[STAThreadAttribute]
void log_process_events() {
    if (!frmLog::Instance::get()->InvokeRequired)
        System::Windows::Forms::Application::DoEvents();
}
//現在のログの長さを返す
[STAThreadAttribute]
int get_current_log_len(int current_pass) {
    return frmLog::Instance::get()->GetLogStringLen(current_pass);
}

#pragma warning( push )
#pragma warning( disable: 4100 )
////////////////////////////////////////////////////
//       frmSetTransparency 関連
////////////////////////////////////////////////////
System::Void frmSetTransparency::setTransparency(int value) {
    value = Convert::ToInt32(clamp(value, fstNUTransparency->Minimum, fstNUTransparency->Maximum));
    fstNUTransparency->Value = clamp(value, fstNUTransparency->Minimum, fstNUTransparency->Maximum);
    fstTBTransparency->Value = clamp(value, fstTBTransparency->Minimum, fstTBTransparency->Maximum);
    frmLog^ log = dynamic_cast<frmLog^>(this->Owner);
    if (log != nullptr) {
        log->frmTransparency = value;
        log->Opacity = (100 - value) * 0.01f;
        //frmSetLogColor::Instance::get()->SetOpacity(log->Opacity);
    }
}

System::Void frmSetTransparency::frmSetTransparency_FormClosed(System::Object^  sender, System::Windows::Forms::FormClosedEventArgs^  e) {
    frmLog^ log = dynamic_cast<frmLog^>(this->Owner);
    if (log != nullptr)
        log->EnableToolStripMenuItemTransparent();
}

System::Void frmSetTransparency::fstSetLastTransparency() {
    frmLog^ log = dynamic_cast<frmLog^>(this->Owner);
    if (log != nullptr)
        last_transparency = 100 - (int)(log->Opacity * 100 + 0.5);
}
////////////////////////////////////////////////////
//       frmSetLogColor 関連
////////////////////////////////////////////////////
System::Void frmSetLogColor::fscBTOK_Click(System::Object^  sender, System::EventArgs^  e) {
    frmLog^ log = dynamic_cast<frmLog^>(this->Owner);
    if (log != nullptr)
        log->SetNewLogColor();
    this->Close();
}
#pragma warning( pop )
