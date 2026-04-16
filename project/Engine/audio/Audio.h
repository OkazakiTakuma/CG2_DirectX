#pragma once
#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include <xaudio2.h>
#include "struct.h"

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")


class Audio {
public:
	// シングルトンインスタンスの取得
	static Audio& GetInstance() {
		static Audio instance;
		return instance;
	}

	// コピーと代入を禁止
	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	bool Initialize();
	void Finalize();
	bool LoadWave(const std::wstring& filename, SoundData& outData);
	void Play(const SoundData& soundData, int mode = 0);

private:
	// コンストラクタを隠蔽
	Audio();
	~Audio();

	IXAudio2* pXAudio2 = nullptr;
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
};