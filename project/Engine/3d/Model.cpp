#include "Model.h"
#include "../2d/TextureManager.h"
#include "../3d/ModelCommon.h"
#include "../base/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>
#include <fstream>
#include <sstream>
using namespace Logger;

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename) {
	this->modelCommon_ = modelCommon;
	modelData = LoadModelFile(directoryPath, filename);
	CreateVertexdata();
	CreateMaterialData();
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

Model::~Model() {
	// デストラクタで Finalize を呼ぶことで、
	// 手動で呼び忘れても delete 時にリソースが解放されるようにする
	Finalize();
}

void Model::Finalize() {
	// 1. マップ解除 (Unmap)
	// Mapしたリソースが生きている場合のみUnmapする
	if (vertexResource) {
		vertexResource->Unmap(0, nullptr);
	}
	if (materialResource) {
		materialResource->Unmap(0, nullptr);
	}

	// 2. ComPtr の解放 (Reset)
	vertexResource.Reset();
	materialResource.Reset();

	// 3. メンバ変数のクリア
	materialData = nullptr;
	modelCommon_ = nullptr;

	// vertexBufferView などは構造体なので Reset は不要だが、
	// 安全のためにゼロクリアしておくとデバッグしやすい
	vertexBufferView = {};
}

void Model::Draw() {
	SrvManager::GetInstance()->PreDraw();

	modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSRVHandleGPU(modelData.material.textureFilePath));

	modelCommon_->GetDxCommon()->GetCommandList()->DrawInstanced(UINT(modelData.vertices.size()), 1, 0, 0);
}

ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	// aiProcess_FlipUVs はそのまま残し、Assimpに反転を任せます
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene != nullptr && scene->HasMeshes());

	// すべてのメッシュを順番に処理するようにループの構造を変更しました
	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0)); // 0はUVチャンネルのインデックス

		// このメッシュのすべての面（ポリゴン）を処理します
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形であることを確認

			for (uint32_t element = 0; element < 3; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;
				// X軸の反転（右手座標系から左手座標系への変換など）はそのまま維持します
				vertex.position = {position.x * -1.0f, position.y, position.z, 1.0f};
				vertex.normal = {normal.x * -1.0f, normal.y, normal.z};

				// 【修正ポイント】手動でのY軸反転をやめ、そのままの値を代入します
				vertex.texcoord = {texcoord.x, texcoord.y};

				modelData.vertices.push_back(vertex);
			}
		}
	}

	// マテリアル（テクスチャ）の読み込み
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString texturePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath); // 0はテクスチャのインデックス
			modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
		}
	}

	// ノード階層の読み込み
	if (scene->mRootNode != nullptr) {
		modelData.rootNode = ReadNode(scene->mRootNode);
	}

	return modelData;
}

Node Model::ReadNode(aiNode* aiNode) {
	Node result;
	aiMatrix4x4 aiLocalMatrix = aiNode->mTransformation;
	aiLocalMatrix.Transpose(); // 行列を転置

	// 1要素ずつ代入する（最も安全）
	result.localMatrix.m[0][0] = aiLocalMatrix.a1;
	result.localMatrix.m[0][1] = aiLocalMatrix.a2;
	result.localMatrix.m[0][2] = aiLocalMatrix.a3;
	result.localMatrix.m[0][3] = aiLocalMatrix.a4;

	result.localMatrix.m[1][0] = aiLocalMatrix.b1;
	result.localMatrix.m[1][1] = aiLocalMatrix.b2;
	result.localMatrix.m[1][2] = aiLocalMatrix.b3;
	result.localMatrix.m[1][3] = aiLocalMatrix.b4;

	result.localMatrix.m[2][0] = aiLocalMatrix.c1;
	result.localMatrix.m[2][1] = aiLocalMatrix.c2;
	result.localMatrix.m[2][2] = aiLocalMatrix.c3;
	result.localMatrix.m[2][3] = aiLocalMatrix.c4;

	result.localMatrix.m[3][0] = aiLocalMatrix.d1;
	result.localMatrix.m[3][1] = aiLocalMatrix.d2;
	result.localMatrix.m[3][2] = aiLocalMatrix.d3;
	result.localMatrix.m[3][3] = aiLocalMatrix.d4;

	result.name = aiNode->mName.C_Str();
	result.children.reserve(aiNode->mNumChildren);
	for (uint32_t childIndex = 0; childIndex < aiNode->mNumChildren; childIndex++) {
		result.children.push_back(ReadNode(aiNode->mChildren[childIndex]));
	}

	return result;
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
	// リソースを作成（新しく作った DirectXCommon のメソッドを利用してもOKです）
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	// 書き込むためのアドレスを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	// 既存の設定
	materialData->color = {1.0f, 1.0f, 1.0f, 1.0f};
	materialData->enableLighting = 1; // 1にするとライティング有効
	materialData->uvTransform = MakeIdentity4x4();

	// ★追加：光沢の強さを設定
	// 値が大きいほど、ハイライトが鋭く（小さく）なります（例：20.0f 〜 100.0f）
	materialData->shininess = 20.0f;
}