#pragma once
#include "struct.h"

#include <Windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <string>
#include <vector>
#include <xaudio2.h>

#pragma comment(lib, "xaudio2.lib")
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")

class Audio {
public:
	Audio();
	~Audio();

	Audio(const Audio&) = delete;
	Audio& operator=(const Audio&) = delete;

	bool Initialize();
	void Finalize();
	bool LoadWave(const std::wstring& filename, SoundData& outData);
	void Play(const SoundData& soundData, int mode = 0);

private:
	IXAudio2* pXAudio2 = nullptr;
	IXAudio2MasteringVoice* pMasterVoice = nullptr;
	bool isInitialized_ = false;
};
