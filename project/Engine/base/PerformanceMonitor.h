#pragma once

#include <Windows.h>
#include <chrono>
#include <pdh.h>
#include <pdhmsg.h>

class PerformanceMonitor {
public:
	void Initialize();
	void Finalize();
	void Update();

	float GetCpuUsagePercent() const { return cpuUsagePercent_; }
	float GetGpuUsagePercent() const { return gpuUsagePercent_; }
	float GetGpu3DUsagePercent() const { return gpu3DUsagePercent_; }
	bool IsGpuUsageAvailable() const { return isGpuUsageAvailable_; }

private:
	void UpdateCpuUsage();
	void UpdateGpuUsage();
	static unsigned long long FileTimeToUint64(const FILETIME& fileTime);

	FILETIME previousIdleTime_{};
	FILETIME previousKernelTime_{};
	FILETIME previousUserTime_{};
	bool hasPreviousCpuTimes_ = false;
	float cpuUsagePercent_ = 0.0f;

	PDH_HQUERY gpuQuery_ = nullptr;
	PDH_HCOUNTER gpuCounter_ = nullptr;
	bool isGpuQueryInitialized_ = false;
	bool isGpuUsageAvailable_ = false;
	float gpuUsagePercent_ = 0.0f;
	float gpu3DUsagePercent_ = 0.0f;

	std::chrono::steady_clock::time_point previousUpdateTime_{};
};
