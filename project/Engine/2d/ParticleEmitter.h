#pragma once
#pragma once
#include "DirectXTex.h"
#include "d3dx12.h"
#include "DirectXCommon.h"
#include "SrvManager.h"
#include "struct.h"
#include "TextureManager.h"
#include <numbers>
#include <random>
#include <string>
#include <vector>
#include <list>
#include "ParticleManager.h"
class ParticleEmitter {
public:
	Emitter data;          // あなたの構造体そのまま
	std::string groupName; // ParticleManager のグループ名
	
	bool isActive = true;

	void Update(float deltaTime);
};
