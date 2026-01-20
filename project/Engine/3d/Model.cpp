#include "Model.h"
#include "../base/Logger.h"
#include "../3d/ModelCommon.h"
#include <fstream>
#include <sstream>
#include <cassert>
#include"../2d/TextureManager.h"
using namespace Logger;

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename) {
	this->modelCommon_ = modelCommon;
	modelData = LoadObjFile(directoryPath, filename);
	CreateVertexdata();
	CreateMaterialData();
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Finalize() {
	vertexResource.Reset();
	materialResource.Reset();
}

void Model::Draw() {
	modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSRVHandleGPU(modelData.material.textureFilePath));

	modelCommon_->GetDxCommon()->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);


}


ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	std::vector<Vector4> positions; // 頂点位置
	std::vector<Vector3> normals;   // 法線ベクトル
	std::vector<Vector2> texcoords; // テクスチャ座標
	std::string line;

	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open() && "Failed to open the OBJ file");

	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;
		if (identifier == "v") { // 頂点位置
			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.x *= -1; // X軸を反転

			position.w = 1.0f; // Homogeneous coordinate
			positions.push_back(position);
		} else if (identifier == "vt") { // テクスチャ座標
			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			texcoord.x = 1.0f - texcoord.x; // X軸はそのまま
			texcoord.y = 1.0f - texcoord.y; // Y軸を反転
			texcoords.push_back(texcoord);
		} else if (identifier == "vn") { // 法線ベクトル
			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			normal.x *= -1; // X軸を反転
			normals.push_back(normal);
		} else if (identifier == "f") { // 面情報
			// 面は三角形限定、他未対応
			for (int32_t faceVertex = 0; faceVertex < 3; faceVertex++) {
				std::string vertexDefinition;
				s >> vertexDefinition;
				// 頂点の情報を分解
				std::istringstream v(vertexDefinition);
				uint32_t elementsIndices[3]; // 頂点、テクスチャ座標、法線のインデックス
				for (int32_t element = 0; element < 3; element++) {
					std::string index;
					std::getline(v, index, '/'); // '/'で区切ってインデックスを取得
					elementsIndices[element] = std::stoi(index);
				}
				// 要素へのIndexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementsIndices[0] - 1];
				Vector2 texcoord = texcoords[elementsIndices[1] - 1];
				Vector3 normal = normals[elementsIndices[2] - 1];
				VertexData vertex = {position, texcoord, normal};
				modelData.vertices.push_back(vertex);
			}
		} else if (identifier == "mtllib") {
			std::string materialFilename;
			s >> materialFilename;
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}

	return modelData;
}

MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename) {
	MaterialData materialData;
	std::string line;
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	while (std::getline(file, line)) {
		std::string idenfire;
		std::istringstream s(line);
		s >> idenfire;

		if (idenfire == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}


void Model::CreateVertexdata() {
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());

	// 頂点バッファビューの作成

	// リソースの先頭のアドレスから使う
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress(); // GPU仮想アドレス
	// 使用するリソースのサイズは頂点のサイズ * 頂点数
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size()); // 頂点バッファのサイズ
	// 1頂点のサイズ
	vertexBufferView.StrideInBytes = sizeof(VertexData); // 1頂点のサイズ

	VertexData* vertexDataModel = nullptr;
	// 書き込むためのアドレスを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel));
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::CreateMaterialData() {
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
	// マテリアルリソースにデータを書き込む
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// マテリアルの色を設定
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f); // 赤色
	materialData->enableLighting = true;                   // ライティングを有効化
	materialData->uvTransform = MakeIdentity4x4();
}
