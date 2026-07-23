#include "ScreenCapture.h"

#include "Logger.h"
#include <Windows.h>
#include <d3d12.h>
#include <wincodec.h>
#include <algorithm>
#include <cstring>
#include <format>
#include <mfapi.h>
#include <mferror.h>
#include <mfreadwrite.h>
#include <propvarutil.h>

#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#pragma comment(lib, "windowscodecs.lib")

namespace {
constexpr uint32_t kRecordingFrameRate = 60;
constexpr uint32_t kRecordingBitRate = 12'000'000;
constexpr LONGLONG kHundredNanosecondsPerSecond = 10'000'000;
}

ScreenCapture::~ScreenCapture() {
	Finalize();
}

void ScreenCapture::RequestScreenshot() {
	screenshotRequested_ = true;
}

void ScreenCapture::ToggleRecording(uint32_t width, uint32_t height) {
	if (recording_) {
		StopRecording();
		return;
	}
	StartRecording(width, height);
}

void ScreenCapture::ProcessFrame(
	ID3D12CommandQueue* commandQueue,
	ID3D12Resource* backBuffer,
	uint32_t width,
	uint32_t height) {
	if ((!screenshotRequested_ && !recording_) || !commandQueue || !backBuffer) {
		return;
	}

	if (recording_ && (width != recordingWidth_ || height != recordingHeight_)) {
		Logger::Log("Screen recording stopped because the render size changed.\n");
		StopRecording();
	}

	DirectX::ScratchImage capturedImage;
	const HRESULT captureResult = DirectX::CaptureTexture(
		commandQueue,
		backBuffer,
		false,
		capturedImage,
		D3D12_RESOURCE_STATE_PRESENT,
		D3D12_RESOURCE_STATE_PRESENT);
	if (FAILED(captureResult)) {
		Logger::Log(std::format("Screen capture failed: 0x{:08X}\n", static_cast<uint32_t>(captureResult)));
		screenshotRequested_ = false;
		return;
	}

	const DirectX::Image* image = capturedImage.GetImage(0, 0, 0);
	if (!image) {
		Logger::Log("Screen capture failed: captured image is empty.\n");
		screenshotRequested_ = false;
		return;
	}

	if (screenshotRequested_) {
		const std::filesystem::path path = MakeCapturePath(L"Screenshot", L".png");
		const HRESULT saveResult = DirectX::SaveToWICFile(
			*image,
			DirectX::WIC_FLAGS_NONE,
			GUID_ContainerFormatPng,
			path.c_str());
		if (SUCCEEDED(saveResult)) {
			Logger::Log("Screenshot saved: " + path.string() + "\n");
		} else {
			Logger::Log(std::format("Screenshot save failed: 0x{:08X}\n", static_cast<uint32_t>(saveResult)));
		}
		screenshotRequested_ = false;
	}

	if (recording_ && !WriteVideoFrame(*image)) {
		Logger::Log("Screen recording stopped because a video frame could not be written.\n");
		StopRecording();
	}
}

void ScreenCapture::Finalize() {
	StopRecording();
	if (mediaFoundationStarted_) {
		MFShutdown();
		mediaFoundationStarted_ = false;
	}
	screenshotRequested_ = false;
}

bool ScreenCapture::StartRecording(uint32_t width, uint32_t height) {
	if (width == 0 || height == 0) {
		return false;
	}

	if (!mediaFoundationStarted_) {
		const HRESULT startupResult = MFStartup(MF_VERSION);
		if (FAILED(startupResult)) {
			Logger::Log(std::format("Media Foundation startup failed: 0x{:08X}\n", static_cast<uint32_t>(startupResult)));
			return false;
		}
		mediaFoundationStarted_ = true;
	}

	const std::filesystem::path path = MakeCapturePath(L"Recording", L".mp4");
	HRESULT hr = MFCreateSinkWriterFromURL(path.c_str(), nullptr, nullptr, &sinkWriter_);
	if (FAILED(hr)) {
		Logger::Log(std::format("Screen recording file creation failed: 0x{:08X}\n", static_cast<uint32_t>(hr)));
		return false;
	}

	Microsoft::WRL::ComPtr<IMFMediaType> outputType;
	hr = MFCreateMediaType(&outputType);
	if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (SUCCEEDED(hr)) hr = outputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_H264);
	if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_AVG_BITRATE, kRecordingBitRate);
	if (SUCCEEDED(hr)) hr = outputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	if (SUCCEEDED(hr)) hr = MFSetAttributeSize(outputType.Get(), MF_MT_FRAME_SIZE, width, height);
	if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(outputType.Get(), MF_MT_FRAME_RATE, kRecordingFrameRate, 1);
	if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(outputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (SUCCEEDED(hr)) hr = sinkWriter_->AddStream(outputType.Get(), &videoStreamIndex_);

	Microsoft::WRL::ComPtr<IMFMediaType> inputType;
	if (SUCCEEDED(hr)) hr = MFCreateMediaType(&inputType);
	if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
	if (SUCCEEDED(hr)) hr = inputType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
	if (SUCCEEDED(hr)) hr = inputType->SetUINT32(MF_MT_INTERLACE_MODE, MFVideoInterlace_Progressive);
	if (SUCCEEDED(hr)) hr = MFSetAttributeSize(inputType.Get(), MF_MT_FRAME_SIZE, width, height);
	if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(inputType.Get(), MF_MT_FRAME_RATE, kRecordingFrameRate, 1);
	if (SUCCEEDED(hr)) hr = MFSetAttributeRatio(inputType.Get(), MF_MT_PIXEL_ASPECT_RATIO, 1, 1);
	if (SUCCEEDED(hr)) hr = sinkWriter_->SetInputMediaType(videoStreamIndex_, inputType.Get(), nullptr);
	if (SUCCEEDED(hr)) hr = sinkWriter_->BeginWriting();
	if (FAILED(hr)) {
		Logger::Log(std::format("Screen recording initialization failed: 0x{:08X}\n", static_cast<uint32_t>(hr)));
		sinkWriter_.Reset();
		return false;
	}

	recordingWidth_ = width;
	recordingHeight_ = height;
	videoFrameIndex_ = 0;
	lastSampleTime_ = -1;
	recordingStartTime_ = std::chrono::steady_clock::now();
	recording_ = true;
	Logger::Log("Screen recording started: " + path.string() + "\n");
	return true;
}

void ScreenCapture::StopRecording() {
	if (sinkWriter_) {
		const HRESULT hr = sinkWriter_->Finalize();
		if (FAILED(hr)) {
			Logger::Log(std::format("Screen recording finalization failed: 0x{:08X}\n", static_cast<uint32_t>(hr)));
		}
	}
	sinkWriter_.Reset();
	if (recording_) {
		Logger::Log("Screen recording stopped.\n");
	}
	recording_ = false;
	recordingWidth_ = 0;
	recordingHeight_ = 0;
	videoFrameIndex_ = 0;
	lastSampleTime_ = -1;
}

bool ScreenCapture::WriteVideoFrame(const DirectX::Image& image) {
	if (!sinkWriter_ || image.width != recordingWidth_ || image.height != recordingHeight_) {
		return false;
	}

	DirectX::ScratchImage convertedImage;
	const DirectX::Image* source = &image;
	if (image.format != DXGI_FORMAT_B8G8R8A8_UNORM) {
		const HRESULT convertResult = DirectX::Convert(
			image,
			DXGI_FORMAT_B8G8R8A8_UNORM,
			DirectX::TEX_FILTER_DEFAULT,
			0.0f,
			convertedImage);
		if (FAILED(convertResult)) {
			return false;
		}
		source = convertedImage.GetImage(0, 0, 0);
	}
	if (!source) {
		return false;
	}

	const DWORD rowBytes = recordingWidth_ * 4;
	const DWORD bufferSize = rowBytes * recordingHeight_;
	Microsoft::WRL::ComPtr<IMFMediaBuffer> mediaBuffer;
	HRESULT hr = MFCreateMemoryBuffer(bufferSize, &mediaBuffer);
	if (FAILED(hr)) {
		return false;
	}

	BYTE* destination = nullptr;
	DWORD maximumLength = 0;
	hr = mediaBuffer->Lock(&destination, &maximumLength, nullptr);
	if (FAILED(hr)) {
		return false;
	}

	// Media Foundation の RGB32 は正のストライドでは下から上の並びです。
	for (uint32_t y = 0; y < recordingHeight_; ++y) {
		const uint8_t* sourceRow = source->pixels + (static_cast<size_t>(recordingHeight_ - 1 - y) * source->rowPitch);
		std::memcpy(destination + (static_cast<size_t>(y) * rowBytes), sourceRow, rowBytes);
	}
	mediaBuffer->Unlock();
	hr = mediaBuffer->SetCurrentLength(bufferSize);
	if (FAILED(hr)) {
		return false;
	}

	Microsoft::WRL::ComPtr<IMFSample> sample;
	hr = MFCreateSample(&sample);
	if (SUCCEEDED(hr)) hr = sample->AddBuffer(mediaBuffer.Get());

	const auto elapsed = std::chrono::steady_clock::now() - recordingStartTime_;
	LONGLONG sampleTime = std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed).count() / 100;
	const LONGLONG minimumTime = (lastSampleTime_ < 0) ? 0 : lastSampleTime_ + 1;
	sampleTime = (std::max)(sampleTime, minimumTime);
	const LONGLONG sampleDuration = kHundredNanosecondsPerSecond / kRecordingFrameRate;
	if (SUCCEEDED(hr)) hr = sample->SetSampleTime(sampleTime);
	if (SUCCEEDED(hr)) hr = sample->SetSampleDuration(sampleDuration);
	if (SUCCEEDED(hr)) hr = sinkWriter_->WriteSample(videoStreamIndex_, sample.Get());
	if (FAILED(hr)) {
		return false;
	}

	lastSampleTime_ = sampleTime;
	++videoFrameIndex_;
	return true;
}

std::filesystem::path ScreenCapture::MakeCapturePath(const wchar_t* prefix, const wchar_t* extension) const {
	wchar_t executablePath[MAX_PATH]{};
	const DWORD executablePathLength = GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
	const std::filesystem::path executableDirectory =
		executablePathLength > 0 ? std::filesystem::path(executablePath).parent_path() : std::filesystem::current_path();
	const std::filesystem::path directory = executableDirectory / L"CaptureFiles";
	std::error_code error;
	std::filesystem::create_directories(directory, error);

	SYSTEMTIME time{};
	GetLocalTime(&time);
	const std::wstring filename = std::format(
		L"{}_{:04}{:02}{:02}_{:02}{:02}{:02}_{:03}{}",
		prefix,
		time.wYear,
		time.wMonth,
		time.wDay,
		time.wHour,
		time.wMinute,
		time.wSecond,
		time.wMilliseconds,
		extension);
	return directory / filename;
}
