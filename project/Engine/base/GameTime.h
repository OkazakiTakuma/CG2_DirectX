#pragma once

#include <chrono>

class GameTime final {
public:
	static void Initialize() {
		previousTime_ = Clock::now();
		deltaTime_ = 1.0f / 60.0f;
		isInitialized_ = true;
	}

	static void Update() {
		const Clock::time_point now = Clock::now();
		if (!isInitialized_) {
			Initialize();
			return;
		}

		const float elapsed = std::chrono::duration<float>(now - previousTime_).count();
		previousTime_ = now;
		if (elapsed > 0.0f) {
			// Prevent a debugger pause or window operation from advancing the game excessively.
			deltaTime_ = elapsed > 0.25f ? 0.25f : elapsed;
		}
	}

	static void SetPaused(bool paused) { isPaused_ = paused; }
	static bool IsPaused() { return isPaused_; }
	static float GetDeltaTime() { return isPaused_ ? 0.0f : deltaTime_; }
	static float GetFrameRate() { return deltaTime_ > 0.0f ? 1.0f / deltaTime_ : 0.0f; }
	static float GetFrameScale60() { return GetDeltaTime() * 60.0f; }

private:
	using Clock = std::chrono::steady_clock;
	static inline Clock::time_point previousTime_{};
	static inline float deltaTime_ = 1.0f / 60.0f;
	static inline bool isInitialized_ = false;
	static inline bool isPaused_ = false;
};
