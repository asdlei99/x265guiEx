﻿//  -----------------------------------------------------------------------------------------
//    拡張 x264/x265 出力(GUI) Ex  v1.xx/2.xx/3.xx by rigaya
//  -----------------------------------------------------------------------------------------
//   ソースコードについて
//   ・無保証です。
//   ・本ソースコードを使用したことによるいかなる損害・トラブルについてrigayaは責任を負いません。
//   以上に了解して頂ける場合、本ソースコードの使用、複製、改変、再頒布を行って頂いて構いません。
//  -----------------------------------------------------------------------------------------

#pragma once

using namespace System;
using namespace System::Text;
using namespace System::IO;
using namespace System::Collections::Generic;
using namespace System::Threading;
using namespace System::Diagnostics;

#include "auo.h"
#include "auo_options.h"

namespace x265guiEx {

    ref class TabPageCtrl
    {
        ref class TabPageData {
        public:
            TabPage^ tabPage;
        public:
            bool Visible;
        public:
            TabPageData(TabPage^ page, bool visible) {
                tabPage = page;
                Visible = visible;
            }
        };

    private:
        TabControl^ ctrl;
        List<TabPageData^>^ tabPages;

    public:
        TabPageCtrl(TabControl^ tabCtrl) {
            ctrl = tabCtrl;
            tabPages = gcnew List<TabPageData^>();
            for (int i = 0; i < tabCtrl->TabPages->Count; i++)
                tabPages->Add(gcnew TabPageData(tabCtrl->TabPages[i], true));
        }
    public:
        ~TabPageCtrl() {
            if (tabPages != nullptr) {
                for (int i = 0; i < tabPages->Count; i++)
                    delete tabPages[i];
                delete tabPages;
            }
        }
    public:
        System::Void RefreshTabCtrl() {
            ctrl->SuspendLayout();
            int n = ctrl->TabPages->Count;
            for(int i = 0; i < n; i++)
                ctrl->TabPages->RemoveAt(0);

            for(int i = 0; i < tabPages->Count; i++)
            {
                if (tabPages[i]->Visible)
                    ctrl->TabPages->Add(tabPages[i]->tabPage);
            }
            ctrl->ResumeLayout();
        }
    public:
        System::Void SetTabPageVisible(int index, bool visible) {
            tabPages[index]->Visible = visible;
        }
    };

    ref class LocalSettings
    {
    public:
        String^ x265ExeName;
        String^ x265Path;
        List<String^>^ audEncName;
        List<String^>^ audEncExeName;
        List<String^>^ audEncPath;
        String^ MP4MuxerExeName;
        String^ MP4MuxerPath;
        String^ MKVMuxerExeName;
        String^ MKVMuxerPath;
        String^ TC2MP4ExeName;
        String^ TC2MP4Path;
        String^ MPGMuxerExeName;
        String^ MPGMuxerPath;
        String^ MP4RawExeName;
        String^ MP4RawPath;
        String^ CustomTmpDir;
        String^ CustomAudTmpDir;
        String^ CustomMP4TmpDir;
        String^ LastAppDir;
        String^ LastBatDir;

        LocalSettings() {
            audEncName = gcnew List<String^>();
            audEncExeName = gcnew List<String^>();
            audEncPath = gcnew List<String^>();
        }
        ~LocalSettings() {
            delete audEncName;
            delete audEncExeName;
            delete audEncPath;
        }
    };

    ref class TBValueBitrateConvert
    {
        value struct TBData {
            int bitrate;
            int step;
            int count;
        };
        array<TBData>^ data;
        ~TBValueBitrateConvert() {
            delete data;
        }
    public:
        TBValueBitrateConvert() {
            data = gcnew array<TBData> {
                {      0,       5,   0 }, //    0 - 1000 までのstepは  5単位
                {   1000,      50,   0 }, // 1000 - 2000 までのstepは 50単位
                {   2000,     100,   0 }, //以下同様
                {   4000,     200,   0 },
                {   8000,    1000,   0 },
                {  64000,    4000,   0 },
                { 256000,   32000,   0 },
                { 256000, INT_MAX,   0 }
            };
            //そこまでのカウント数を計算しておく
            for (int i = 0; i < data->Length - 2; i++)
                data[i+1].count = data[i].count + (data[i+1].bitrate - data[i].bitrate) / data[i].step;
        };
        int getMaxCount() //最大のカウント数(TBの最大値)
        {
            return data[data->Length - 2].count;
        };
        int BitrateToTB(int rate) //ビットレートをTBの値に換算
        {
            for (int i = data->Length - 2; i >= 0; i--)
                if (data[i].bitrate <= (int)min((DWORD)INT_MAX, (DWORD)rate)) //int:-1をDWORD:0xffffffffとして扱い、最大値として設定
                    return data[i].count + (int)((rate - data[i].bitrate) / (double)data[i].step);
            return 0;
        };
        int TBToBitrate(int TBValue) //TBの値からビットレートに
        {
            for (int i = data->Length - 2; i >= 0; i--)
                if (data[i].count <= TBValue)
                    return data[i].bitrate + (TBValue - data[i].count) * data[i].step;
            return 0;
        };
    };

//
//    ref class stgFileController
//    {
//        List<String^>^ stgPath; //stgファイルのフルパスリスト
//        List<String^>^ stgName; //stgファイル名(拡張子なし)のリスト
//        String^ stgDir; //stgファイルのディレクトリ
//
//        System::Boolean CheckStgDir() //stgファイルのディレクトリが存在するか、なければ作成
//        {
//            if (stgDir == nullptr)
//                return false;
//            if (!Directory::Exists(stgDir)) {
//                try {
//                    Directory::CreateDirectory(stgDir);
//                } catch (...) {
//                    return false;
//                }
//            }
//            return true;
//        }
//        System::Void Clear() //リストをクリア
//        {
//            stgPath->Clear();
//            stgName->Clear();
//        }
//
//    public:
//        stgFileController(String^ _stgDir)
//        {
//            stgDir = _stgDir;
//            stgPath = gcnew List<String^>();
//            stgName = gcnew List<String^>();
//        }
//        ~stgFileController()
//        {
//            delete stgPath;
//            delete stgName;
//        }
//        System::Void ReLoad()
//        {
//            Clear();
//            if (CheckStgDir()) {
//                stgPath->AddRange(Directory::GetFiles(stgDir, L"*.stg", SearchOption::AllDirectories));
//                for (int i = 0; i < stgPath->Count; i++)
//                    stgName->Add(Path::GetFileNameWithoutExtension(stgPath[i]));
//            }
//        }
//        System::Void ReLoad(String^ _stgDir)
//        {
//            stgDir = _stgDir;
//            ReLoad();
//        }
//        List<String^>^ getStgNameList() //stgファイル名(拡張子なし)のリストを取得
//        {
//            return stgName;
//        }
//        String^ getStgFullPath(int index) //stgフルパスを返す
//        {
//            return (index >= 0 && index < stgPath->Count) ? stgPath[index] : L"";
//        }
//        int AddStgToList(String^ _stgName) //ファイル名を追加、すでに存在するファイル名ならそのインデックスを返す
//        {
//            //_stgNameには拡張子なしのものを渡す
//            //すでに存在するか確認
//            for (int i = 0; i < stgName->Count; i++)
//                if (String::Compare(stgName[i], _stgName, true) == 0)
//                    return i;
//            stgPath->Add(Path::Combine(stgDir, _stgName + L".stg"));
//            stgName->Add(_stgName);
//            return stgName->Count - 1;
//        }
//        void DeleteFromList(int index) //インデックスのファイルを削除
//        {
//            stgPath->RemoveAt(index);
//            stgName->RemoveAt(index);
//        }
//    };


    value struct ExeControls
    {
        String^ Name;
        String^ Path;
        const char* args;
    };

    const int fcgTBQualityTimerLatency = 600;
    const int fcgTBQualityTimerPeriod = 40;
    const int fcgTXCmdfulloffset = 57;
    const int fcgCXAudioEncModeSmallWidth = 189;
    const int fcgCXAudioEncModeLargeWidth = 237;
};

static const WCHAR *use_default_exe_path = L"exe_files内の実行ファイルを自動選択";

//コンボボックスの表示名
const WCHAR * const x264_encodemode_desc[] = {
    L"シングルパス - ビットレート指定",
    L"シングルパス - 固定量子化量",
    L"シングルパス - 品質基準VBR(可変レート)",
    L"マルチパス - 1pass",
    L"マルチパス - Npass",
    L"自動マルチパス",
    L"上限確認付 品質基準VBR(可変レート)",
    NULL
};
const WCHAR * const x265_encodemode_desc[] = {
    L"シングルパス - ビットレート指定",
    L"シングルパス - 固定量子化量",
    L"シングルパス - 品質基準VBR(可変レート)",
    L"マルチパス - 1pass",
    L"マルチパス - Npass",
    L"自動マルチパス",
    L"上限確認付 品質基準VBR(可変レート)",
    NULL
};

#define STR_BITRATE_AUTO (L"-1: 自動 ")

const int x265_outputcsp_map[] = {
    OUT_CSP_NV12,
    OUT_CSP_NV16,
    OUT_CSP_YUV444,
    OUT_CSP_RGB,
    OUT_CSP_NV16,
    OUT_CSP_NV12,
    OUT_CSP_NV16,
};

const int x265_encmode_to_RCint[] = {
    X265_RC_BITRATE,
    X265_RC_QP,
    X265_RC_CRF,
    X265_RC_BITRATE,
    X265_RC_BITRATE,
    X265_RC_BITRATE,
    X265_RC_CRF,
    NULL
};

const int x265_encmode_to_passint[] = {
    0,
    0,
    0,
    1,
    3,
    3,
    NULL
};
const WCHAR *const x265_scenecut_mode_desc[] = {
    L"デフォルト",
    L"ヒストグラム",
    NULL
};

const WCHAR * const aspect_desc[] = {
    L"SAR比を指定 (デフォルト)",
    L"画面比から自動計算",
    NULL
};

const WCHAR * const tempdir_desc[] = {
    L"出力先と同じフォルダ (デフォルト)",
    L"システムの一時フォルダ",
    L"カスタム",
    NULL
};

const WCHAR * const audtempdir_desc[] = {
    L"変更しない",
    L"カスタム",
    NULL
};

const WCHAR * const mp4boxtempdir_desc[] = {
    L"指定しない",
    L"カスタム",
    NULL
};

const WCHAR * const interlaced_desc[] = {
    L"プログレッシブ",
    L"インタレ (tff)",
    L"インタレ (bff)",
    NULL
};

const WCHAR * const audio_enc_timing_desc[] = {
    L"後",
    L"前",
    L"同時",
    NULL
};

const WCHAR * const audio_delay_cut_desc[] = {
    L"補正なし",
    L"音声カット",
    L"映像追加",
    NULL
};

const WCHAR * const bit_depth_desc[] ={
    L"8bit",
    L"10bit",
    L"12bit",
    NULL
};
static int get_bit_depth(const WCHAR *str) {
    if (0 == wcscmp(str, bit_depth_desc[0])) return 8;
    if (0 == wcscmp(str, bit_depth_desc[1])) return 10;
    if (0 == wcscmp(str, bit_depth_desc[2])) return 12;
    return 8;
}
static int get_bit_depth(int index) {
    return get_bit_depth(bit_depth_desc[index]);
}
static int get_bit_depth_idx(int bit_depth) {
    switch (bit_depth) {
    case 10: return 1;
    case 12: return 2;
    case 8:
    default: return 0;
    }
}

//メモ表示用 RGB
const int StgNotesColor[][3] = {
    {  80,  72,  92 },
    { 120, 120, 120 }
};

const WCHAR * const DefaultStgNotes = L"メモ...";
const WCHAR * const DefaultStatusFilePath = L"%{savfile}.stats";
const WCHAR * const DefaultTcFilePath = L"%{savfile}_tc.txt";
const WCHAR * const DefaultAnalysisFilePath = L"%{savfile}_x265_analysis.dat";


typedef struct {
    WCHAR *string;
    WCHAR *desc;
} REPLACE_STRINGS;

const REPLACE_STRINGS REPLACE_STRINGS_LIST[] = {
    { L"%{vidpath}",          L"一時動画ファイル名(フルパス)" },
    { L"%{audpath}",          L"一時音声ファイル名(フルパス)" },
    { L"%{tmpdir}",           L"一時フォルダ名(最後の\\無し)" },
    { L"%{tmpfile}",          L"一時ファイル名(フルパス・拡張子除く)" },
    { L"%{tmpname}",          L"一時ファイル名(ファイル名のみ・拡張子除く)" },
    { L"%{savpath}",          L"出力ファイル名(フルパス)" },
    { L"%{savfile}",          L"出力ファイル名(フルパス・拡張子除く)" },
    { L"%{savname}",          L"出力ファイル名(ファイル名のみ・拡張子除く)" },
    { L"%{savdir}",           L"出力フォルダ名(最後の\\無し)" },
    { L"%{aviutldir}",        L"Aviutl.exeのフォルダ名(最後の\\無し)" },
    { L"%{chpath}",           L"チャプターファイル名(フルパス)" },
    { L"%{tcpath}",           L"タイムコードファイル名(フルパス)" },
    { L"%{muxout}",           L"muxで作成する一時ファイル名(フルパス)" },
    //{ L"%{x264path}",         L"指定された x264.exe のパス" },
    //{ L"%{x264_10path}",      L"指定された x264.exe(10bit版) のパス" },
    //{ L"%{audencpath}",       L"実行された音声エンコーダのパス" },
    //{ L"%{mp4muxerpath}",     L"mp4 muxerのパス" },
    //{ L"%{mkvmuxerpath}",     L"mkv muxerのパス" },
    { L"%{fps_scale}",        L"フレームレート(分母)" },
    { L"%{fps_rate}",         L"フレームレート(分子)" },
    { L"%{fps_rate_times_4}", L"フレームレート(分子)×4" },
    { L"%{sar_x}",            L"サンプルアスペクト比 (横)" },
    { L"%{sar_y}",            L"サンプルアスペクト比 (縦)" },
    { L"%{dar_x}",            L"画面アスペクト比 (横)" },
    { L"%{dar_y}",            L"画面アスペクト比 (縦)" },
    { NULL, NULL }
};
