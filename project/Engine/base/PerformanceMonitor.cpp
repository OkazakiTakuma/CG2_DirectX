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
	hasPreviousCpuTimes_ = false;
	cpuUsagePercent_ = 0.0f;
	gpuUsagePercent_ = 0.0f;
	gpu3DUsagePercent_ = 0.0f;
	isGpuUsageAvailable_ = false;
	previousUpdateTime_ = std::chrono::steady_clock::now() - kUpdateInterval;

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
	if (gpuQuery_) {
		PdhCloseQuery(gpuQuery_);
	}
	gpuQuery_ = nullptr;
	gpuCounter_ = nullptr;
	isGpuQueryInitialized_ = false;
	isGpuUsageAvailable_ = false;
}

void PerformanceMonitor::Update() {
	const auto now = std::chrono::steady_clock::now();
	if (now - previousUpdateTime_ < kUpdateInterval) {
		return;
	}
	previousUpdateTime_ = now;

	UpdateCpuUsage();
	UpdateGpuUsage();
}

void PerformanceMonitor::UpdateCpuUsage() {
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
		cpuUsagePercent_ = static_cast<float>(std::clamp(used, 0.0, kPercentMax));
	}

	previousIdleTime_ = idleTime;
	previousKernelTime_ = kernelTime;
	previousUserTime_ = userTime;
}

void PerformanceMonitor::UpdateGpuUsage() {
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

	gpuUsagePercent_ = static_cast<float>(std::clamp(totalUsage, 0.0, kPercentMax));
	gpu3DUsagePercent_ = static_cast<float>(std::clamp(usage3D, 0.0, kPercentMax));
	isGpuUsageAvailable_ = true;
}

unsigned long long PerformanceMonitor::FileTimeToUint64(const FILETIME& fileTime) {
	ULARGE_INTEGER value{};
	value.LowPart = fileTime.dwLowDateTime;
	value.HighPart = fileTime.dwHighDateTime;
	return value.QuadPart;
}
