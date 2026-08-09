#pragma once

#include <string>
#include <vector>

struct StageResultEquipment {
	std::string name;
	std::string level;
};

/// <summary>ゲームプレイ終了時に確定し、リザルトシーンへ渡す戦績です。</summary>
struct StageResultData {
	std::vector<StageResultEquipment> attacks;
	std::vector<StageResultEquipment> statuses;
	int defeatedEnemyCount = 0;
	float survivalTimeSeconds = 0.0f;
	bool stageCleared = false;
};
