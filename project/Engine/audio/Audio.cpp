#include "Audio.h"

#include <cstring>

Audio::Audio() = default;

Audio::~Audio() { Finalize(); }

/// <summary>
/// 必要なリソースを準備し、オブジェクトを初期化します。
/// </summary>
/// <returns>処理結果を返します。</returns>
bool Audio::Initialize() {
	if (isInitialized_) {
		return true;
	}

	HRESULT hr = MFStartup(MF_VERSION);
	if (FAILED(hr)) {
		return false;
	}

	hr = XAudio2Create(&pXAudio2, 0, XAUDIO2_DEFAULT_PROCESSOR);
	if (FAILED(hr)) {
		MFShutdown();
		return false;
	}

	hr = pXAudio2->CreateMasteringVoice(&pMasterVoice);
	if (FAILED(hr)) {
		pXAudio2.Reset();
		MFShutdown();
		return false;
	}

	isInitialized_ = true;
	return true;
}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void Audio::Finalize() {
	if (pMasterVoice != nullptr) {
		pMasterVoice->DestroyVoice();
		pMasterVoice = nullptr;
	}

	pXAudio2.Reset();

	if (isInitialized_) {
		MFShutdown();
		isInitialized_ = false;
	}
}

/// <summary>
/// Wave を読み込み、内部データへ反映します。
/// </summary>
/// <param name="filename">読み込みまたは保存に使用するファイルパスを指定します。</param>
/// <param name="outData">outData に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
bool Audio::LoadWave(const std::wstring& filename, SoundData& outData) {
	Microsoft::WRL::ComPtr<IMFSourceReader> pReader = nullptr;
	HRESULT hr = MFCreateSourceReaderFromURL(filename.c_str(), nullptr, &pReader);
	if (FAILED(hr)) {
		return false;
	}

	Microsoft::WRL::ComPtr<IMFMediaType> pPartialType = nullptr;
	hr = MFCreateMediaType(&pPartialType);
	if (FAILED(hr)) {
		return false;
	}

	pPartialType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
	pPartialType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
	pReader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, pPartialType.Get());

	Microsoft::WRL::ComPtr<IMFMediaType> pUncompressedAudioType = nullptr;
	pReader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &pUncompressedAudioType);
	UINT32 cbFormat = 0;
	WAVEFORMATEX* pWav = nullptr;
	MFCreateWaveFormatExFromMFMediaType(pUncompressedAudioType.Get(), &pWav, &cbFormat);
	outData.wfx = *pWav;
	CoTaskMemFree(pWav);

	outData.buffer.clear();
	while (true) {
		Microsoft::WRL::ComPtr<IMFSample> pSample = nullptr;
		DWORD flags = 0;
		pReader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &pSample);
		if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
			break;
		}
		if (!pSample) {
			continue;
		}

		Microsoft::WRL::ComPtr<IMFMediaBuffer> pBuffer = nullptr;
		pSample->ConvertToContiguousBuffer(&pBuffer);
		BYTE* pAudioData = nullptr;
		DWORD cbCurrentLength = 0;
		pBuffer->Lock(&pAudioData, nullptr, &cbCurrentLength);

		size_t oldSize = outData.buffer.size();
		outData.buffer.resize(oldSize + cbCurrentLength);
		std::memcpy(&outData.buffer[oldSize], pAudioData, cbCurrentLength);

		pBuffer->Unlock();
	}

	return true;
}

/// <summary>
/// 読み込まれた音声データを再生します。
/// </summary>
/// <param name="soundData">soundData に使用する値を指定します。</param>
/// <param name="mode">mode に使用する値を指定します。</param>
void Audio::Play(const SoundData& soundData, int mode) {
	if (!pXAudio2) {
		return;
	}

	IXAudio2SourceVoice* pSourceVoice = nullptr;
	HRESULT hr = pXAudio2->CreateSourceVoice(&pSourceVoice, &soundData.wfx);
	if (FAILED(hr)) {
		return;
	}

	XAUDIO2_BUFFER buffer = {0};
	buffer.AudioBytes = static_cast<UINT32>(soundData.buffer.size());
	buffer.pAudioData = soundData.buffer.data();
	buffer.Flags = XAUDIO2_END_OF_STREAM;
	buffer.LoopCount = (mode == 1) ? XAUDIO2_LOOP_INFINITE : 0;

	pSourceVoice->SubmitSourceBuffer(&buffer);
	pSourceVoice->Start();
}
