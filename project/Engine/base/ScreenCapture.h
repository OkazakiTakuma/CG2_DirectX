#pragma once

#include <d3d12.h>
#include "DirectXTex.h"
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <mfidl.h>
#include <wrl.h>

struct ID3D12CommandQueue;
struct ID3D12Resource;
struct IMFSinkWriter;

/// <summary>
/// バックバッファを PNG または H.264 MP4 として CaptureFiles に保存します。
/// </summary>
class ScreenCapture {
public:
	ScreenCapture() = default;
	~ScreenCapture();

	void RequestScreenshot();
	void ToggleRecording(uint32_t width, uint32_t height);
	void ProcessFrame(ID3D12CommandQueue* commandQueue, ID3D12Resource* backBuffer, uint32_t width, uint32_t height);
	void Finalize();

	bool IsRecording() const { return recording_; }

private:
	bool StartRecording(uint32_t width, uint32_t height);
	void StopRecording();
	bool WriteVideoFrame(const DirectX::Image& image);
	std::filesystem::path MakeCapturePath(const wchar_t* prefix, const wchar_t* extension) const;

	bool screenshotRequested_ = false;
	bool recording_ = false;
	bool mediaFoundationStarted_ = false;
	uint32_t recordingWidth_ = 0;
	uint32_t recordingHeight_ = 0;
	uint64_t videoFrameIndex_ = 0;
	LONGLONG lastSampleTime_ = -1;
	std::chrono::steady_clock::time_point recordingStartTime_{};
	DWORD videoStreamIndex_ = 0;
	Microsoft::WRL::ComPtr<IMFSinkWriter> sinkWriter_;
};
