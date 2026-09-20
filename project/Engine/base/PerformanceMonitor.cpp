#include "PerformanceMonitor.h"

#include <algorithm>
#include <cwchar>
#include <vector>

#pragma comment(lib, "pdh.lib")

namespace {
constexpr double kPercentMax = 100.0;
constexpr auto kUpdateInterval = std::chrono::milliseconds(250);
}

void PerformanceMonitor::Initialize() {
	// CPU差分計算用の初期値を取得し、GPU用PDHクエリを準備する。
	hasPreviousCpuTimes_ = false;
	cpuUsagePercent_ = 0.0f;
	gpuUsagePercent_ = 0.0f;
	gpu3DUsagePercent_ = 0.0f;
	isGpuUsageAvailable_ = false;
	previousUpdateTime_ = std::chrono::steady_clock::now() - kUpdateInterval;
	isUpdateRunning_ = false;

	if (PdhOpenQueryW(nullptr, 0, &gpuQuery_) != ERROR_SUCCESS) {
		gpuQuery_ = nullptr;
		return;
	}

	const PDH_STATUS addStatus = PdhAddEnglishCounterW(gpuQuery_, L"\\GPU Engine(*)\\Utilization Percentage", 0, &gpuCounter_);
	if (addStatus != ERROR_SUCCESS) {
		PdhCloseQuery(gpuQuery_);
		gpuQuery_ = nullptr;
		gpuCounter_ = nullptr;
		return;
	}

	isGpuQueryInitialized_ = PdhCollectQueryData(gpuQuery_) == ERROR_SUCCESS;
}

void PerformanceMonitor::Finalize() {
	// PDHハンドルを閉じる前に、実行中の収集処理だけは完了させる。
	if (updateFuture_.valid()) {
		updateFuture_.wait();
		updateFuture_.get();
	}
	isUpdateRunning_ = false;
	// PDHハンドルは生成と逆順に閉じ、再初期化できる状態へ戻す。
	if (gpuQuery_) {
		PdhCloseQuery(gpuQuery_);
	}
	gpuQuery_ = nullptr;
	gpuCounter_ = nullptr;
	isGpuQueryInitialized_ = false;
	isGpuUsageAvailable_ = false;
}

void PerformanceMonitor::Update() {
	// 完了確認はブロックせず行い、PDHの重いインスタンス列挙を描画スレッドへ持ち込まない。
	if (isUpdateRunning_ && updateFuture_.valid() &&
		updateFuture_.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		updateFuture_.get();
		isUpdateRunning_ = false;
	}

	// カウンター取得コストを抑えるため、一定間隔に達したときだけ1処理を起動する。
	const auto now = std::chrono::steady_clock::now();
	if (isUpdateRunning_ || now - previousUpdateTime_ < kUpdateInterval) {
		return;
	}
	previousUpdateTime_ = now;
	isUpdateRunning_ = true;
	updateFuture_ = std::async(std::launch::async, [this]() {
		UpdateCpuUsage();
		UpdateGpuUsage();
	});
}

void PerformanceMonitor::UpdateCpuUsage() {
	// Windowsの累積時間を前回値と比較し、アイドル以外の割合を算出する。
	FILETIME idleTime{};
	FILETIME kernelTime{};
	FILETIME userTime{};
	if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
		return;
	}

	if (!hasPreviousCpuTimes_) {
		previousIdleTime_ = idleTime;
		previousKernelTime_ = kernelTime;
		previousUserTime_ = userTime;
		hasPreviousCpuTimes_ = true;
		return;
	}

	const unsigned long long idle = FileTimeToUint64(idleTime) - FileTimeToUint64(previousIdleTime_);
	const unsigned long long kernel = FileTimeToUint64(kernelTime) - FileTimeToUint64(previousKernelTime_);
	const unsigned long long user = FileTimeToUint64(userTime) - FileTimeToUint64(previousUserTime_);
	const unsigned long long total = kernel + user;

	if (total > 0) {
		const double used = static_cast<double>(total - idle) / static_cast<double>(total) * kPercentMax;
		cpuUsagePercent_.store(static_cast<float>(std::clamp(used, 0.0, kPercentMax)), std::memory_order_relaxed);
	}

	previousIdleTime_ = idleTime;
	previousKernelTime_ = kernelTime;
	previousUserTime_ = userTime;
}

void PerformanceMonitor::UpdateGpuUsage() {
	// GPU Engineカウンターの全インスタンスを収集し、利用可能な値を集計する。
	if (!isGpuQueryInitialized_ || !gpuCounter_) {
		isGpuUsageAvailable_ = false;
		return;
	}

	if (PdhCollectQueryData(gpuQuery_) != ERROR_SUCCESS) {
		isGpuUsageAvailable_ = false;
		return;
	}

	DWORD bufferSize = 0;
	DWORD itemCount = 0;
	PDH_STATUS status = PdhGetFormattedCounterArrayW(gpuCounter_, PDH_FMT_DOUBLE, &bufferSize, &itemCount, nullptr);
	if (status != PDH_MORE_DATA || bufferSize == 0 || itemCount == 0) {
		isGpuUsageAvailable_ = false;
		return;
	}

	std::vector<unsigned char> buffer(bufferSize);
	auto* items = reinterpret_cast<PDH_FMT_COUNTERVALUE_ITEM_W*>(buffer.data());
	status = PdhGetFormattedCounterArrayW(gpuCounter_, PDH_FMT_DOUBLE, &bufferSize, &itemCount, items);
	if (status != ERROR_SUCCESS) {
		isGpuUsageAvailable_ = false;
		return;
	}

	double totalUsage = 0.0;
	double usage3D = 0.0;
	for (DWORD index = 0; index < itemCount; ++index) {
		if (items[index].FmtValue.CStatus != ERROR_SUCCESS) {
			continue;
		}

		const double value = items[index].FmtValue.doubleValue;
		totalUsage += value;
		if (items[index].szName && std::wcsstr(items[index].szName, L"engtype_3D")) {
			usage3D += value;
		}
	}

	gpuUsagePercent_.store(static_cast<float>(std::clamp(totalUsage, 0.0, kPercentMax)), std::memory_order_relaxed);
	gpu3DUsagePercent_.store(static_cast<float>(std::clamp(usage3D, 0.0, kPercentMax)), std::memory_order_relaxed);
	isGpuUsageAvailable_.store(true, std::memory_order_relaxed);
}

unsigned long long PerformanceMonitor::FileTimeToUint64(const FILETIME& fileTime) {
	ULARGE_INTEGER value{};
	value.LowPart = fileTime.dwLowDateTime;
	value.HighPart = fileTime.dwHighDateTime;
	return value.QuadPart;
}
