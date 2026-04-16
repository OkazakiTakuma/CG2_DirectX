#include "Audio.h"
#include <iostream>

Audio::Audio() {}

Audio::~Audio() { Finalize(); }

bool Audio::Initialize() {
	// COMの初期化
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// XAudio2エンジンの作成
	hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr))
		return false;

	// マスターボイス（最終的な出力先）の作成
	hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
	if (FAILED(hr))
		return false;

	// Media Foundationの初期化
	hr = MFStartup(MF_VERSION);
	return SUCCEEDED(hr);
}

void Audio::Finalize() {
	// 1. マスターボイスの破棄
	if (pMasterVoice != nullptr) {
		pMasterVoice->DestroyVoice();
		pMasterVoice = nullptr; // 解放したら必ずヌルを入れる
	}

	// 2. XAudio2エンジンの解放
	if (pXAudio2 != nullptr) {
		pXAudio2->Release();
		pXAudio2 = nullptr;
	}

	// 3. Media Foundationの終了処理（複数回呼んでも安全な設計ですが念のため）
	MFShutdown();

	// COMの終了
	// CoUninitialize(); // ※メインスレッドの動作状況によっては注意が必要
}
bool Audio::LoadWave(const std::wstring& filename, SoundData& outData) {
	IMFSourceReader* pReader = nullptr;
	HRESULT hr = MFCreateSourceReaderFromURL(filename.c_str(), nullptr, &pReader);
	if (FAILED(hr))
		return false;

	// 出力形式をPCM（非圧縮）に設定
	IMFMediaType* pPartialType = nullptr;
	MFCreateMediaType(&pPartialType);
	pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPartialType);
	pPartialType->Release();

	// 最終的なフォーマットを取得
	IMFMediaType* pUncompressedAudioType = nullptr;
	pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType);
	UINT32 cbFormat = 0;
	WAVEFORMATEX* pWav = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType, &pWav, &cbFormat);
	outData.wfx = *pWav;
	CoTaskMemFree(pWav);
	pUncompressedAudioType->Release();

	// データの読み込み
	while (true) {
		IMFSample* pSample = nullptr;
		DWORD flags = 0;
		pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &pSample);
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM)
			break;
		if (!pSample)
			continue;

		IMFMediaBuffer* pBuffer = nullptr;
		pSample->ConvertToContiguousBuffer(&pBuffer);
		BYTE* pAudioData = nullptr;
		DWORD cbCurrentLength = 0;
		pBuffer->Lock(&pAudioData, nullptr, &cbCurrentLength);

		// バッファにデータを追加
		size_t oldSize = outData.buffer.size();
		outData.buffer.resize(oldSize + cbCurrentLength);
		memcpy(&outData.buffer[oldSize], pAudioData, cbCurrentLength);

		pBuffer->Unlock();
		pBuffer->Release();
		pSample->Release();
	}
	pReader->Release();
	return true;
}

void Audio::Play(const SoundData& soundData, int mode) {
	if (!pXAudio2)
		return;

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	HRESULT hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfx);
	if (FAILED(hr))
		return;

	XAUDIO2_BUFFER buffer = {0};
	buffer.AudioBytes = (UINT32)soundData.buffer.size();
	buffer.pAudioData = soundData.buffer.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;

	// --- 引数による分岐 ---
	if (mode == 1) {
		// 1のときは無限ループ
		buffer.LoopCount = XAUDIO2_LOOP_INFINITE;
	} else {
		// 0（またはそれ以外）のときはループなし
		buffer.LoopCount = 0;
	}
	// -----------------------

	pSourceVoice->SubmitSourceBuffer(&buffer);
	pSourceVoice->Start();
}