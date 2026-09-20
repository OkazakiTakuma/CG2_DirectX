#pragma once

#include <Windows.h>
#include <atomic>
#include <chrono>
#include <future>
#include <pdh.h>
#include <pdhmsg.h>

/// <summary>WindowsのCPU・GPUカウンターから実行時負荷を収集します。</summary>
class PerformanceMonitor {
public:
	void Initialize();
	void Finalize();
	void Update();

	float GetCpuUsagePercent() const { return cpuUsagePercent_.load(std::memory_order_relaxed); }
	float GetGpuUsagePercent() const { return gpuUsagePercent_.load(std::memory_order_relaxed); }
	float GetGpu3DUsagePercent() const { return gpu3DUsagePercent_.load(std::memory_order_relaxed); }
	bool IsGpuUsageAvailable() const { return isGpuUsageAvailable_.load(std::memory_order_relaxed); }

private:
	/// <summary>前回値との差分からシステム全体のCPU使用率を更新します。</summary>
	void UpdateCpuUsage();
	/// <summary>PDHカウンターからGPU使用率を更新します。</summary>
	void UpdateGpuUsage();
	/// <summary>FILETIMEを差分計算可能な64bit整数へ変換します。</summary>
	static unsigned long long FileTimeToUint64(const FILETIME& fileTime);

	// CPU使用率の差分計算に使用する前回の累積時間。
	FILETIME previousIdleTime_{};
	FILETIME previousKernelTime_{};
	FILETIME previousUserTime_{};
	bool hasPreviousCpuTimes_ = false;
	std::atomic<float> cpuUsagePercent_ = 0.0f;

	// Windows PDHへ発行するGPU使用率クエリと、その有効状態。
	PDH_HQUERY gpuQuery_ = nullptr;
	PDH_HCOUNTER gpuCounter_ = nullptr;
	bool isGpuQueryInitialized_ = false;
	std::atomic<bool> isGpuUsageAvailable_ = false;
	std::atomic<float> gpuUsagePercent_ = 0.0f;
	std::atomic<float> gpu3DUsagePercent_ = 0.0f;

	/// <summary>負荷計測の更新間隔を制御するための前回更新時刻です。</summary>
	std::chrono::steady_clock::time_point previousUpdateTime_{};
	// PDH列挙は環境によって数ms停止するため、描画スレッドではなく1本だけ非同期実行する。
	std::future<void> updateFuture_;
	bool isUpdateRunning_ = false;
};
