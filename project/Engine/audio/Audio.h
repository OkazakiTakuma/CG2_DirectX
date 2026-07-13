#pragma once
#include "struct.h"

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include <wrl.h>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class Audio {
public:
	/// <summary>
	/// Audio の処理を行います。
	/// </summary>
	Audio();
	/// <summary>
	/// 破棄時に必要な解放処理を行います。
	/// </summary>
	~Audio();

	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	/// <returns>処理結果を返します。</returns>
	bool Initialize();
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize();
	/// <summary>
	/// Wave を読み込み、内部データへ反映します。
	/// </summary>
	/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
	/// <param name="outData">outData に使用する値を指定します。</param>
	/// <returns>処理結果を返します。</returns>
	bool LoadWave(const std::wstring& filename, SoundData& outData);
	/// <summary>
	/// 読み込まれた音声データを再生します。
	/// </summary>
	/// <param name="soundData">soundData に使用する値を指定します。</param>
	/// <param name="mode">mode に使用する値を指定します。</param>
	void Play(const SoundData& soundData, int mode = 0);

private:
	Microsoft::WRL::ComPtr<IXAudio2> pXAudio2 = nullptr;
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	bool isInitialized_ = false;
};
