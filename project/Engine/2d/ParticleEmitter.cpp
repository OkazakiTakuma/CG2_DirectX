#include "ParticleEmitter.h"
#include "Camera.h"



void ParticleEmitter::Update(float deltaTime) {
	{
		if (!isActive)
			return;

		// タイマー更新
		data.frequencyTimer += deltaTime;

		// 発生間隔 = 1 / frequency
		float interval = 1.0f / data.frequency;

		// 必要な回数 Emit
		while (data.frequencyTimer >= interval) {
			data.frequencyTimer -= interval;

			ParticleManager::GetInstance()->Emit(groupName, data.transform.translate, data.count);
		}
	}
}
