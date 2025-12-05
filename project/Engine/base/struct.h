#pragma once
#include <string>
#include <vector>
#include "../3d/Matrix.h"

enum BlendMode {
	kBlendModeNone,
	kBlendModeNormal,
	kBlendModeAdd,
	kBlendModeSubtract,
	kBlendModeMultiply,
	kBlendModeScreen,
	kBlendCountblend,
};
struct VertexData {
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};
struct MaterialData {
	std::string textureFilePath; // テクスチャファイルのパス
	uint32_t textureIndex = 0;
};

struct ModelData {
	std::vector<VertexData> vertices; // 頂点データ
	MaterialData material;
};
