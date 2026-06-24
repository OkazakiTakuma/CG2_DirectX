#include "SkinnedModel.h"
#include "../2d/TextureManager.h"
#include "../3d/ModelCommon.h"
#include "../base/Logger.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <cassert>

using namespace Logger;

void SkinnedModel::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename) {
	this->modelCommon_ = modelCommon;
	modelData = LoadModelFile(directoryPath, filename);
	CreateVertexdata();
	CreateMaterialData();
	CreateBoneData(); // アニメーション用のボーンバッファ作成

	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

SkinnedModel::~SkinnedModel() {
	Finalize();
}

void SkinnedModel::Finalize() {
	if (vertexResource) {
		vertexResource->Unmap(0, nullptr);
	}
	if (materialResource) {
		materialResource->Unmap(0, nullptr);
	}
	if (boneResource) {
		boneResource->Unmap(0, nullptr);
	}
}

// SkinnedModel.cpp
void SkinnedModel::Draw() {
	// ★追加：自分が持っている modelCommon_ 経由でコマンドリストを取得する
	ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList().Get();

	// 頂点バッファをセットして描画
	commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	commandList->DrawInstanced(static_cast<UINT>(modelData.vertices.size()), 1, 0, 0);
}
SkinnedModelData SkinnedModel::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
	SkinnedModelData modelData;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene != nullptr && scene->HasMeshes());

	// ウェイト計算用の一時データ構造体
	struct WeightData {
		int32_t boneIndices[MAX_BONE_INFLUENCE] = { 0, 0, 0, 0 };
		float boneWeights[MAX_BONE_INFLUENCE] = { 0.0f, 0.0f, 0.0f, 0.0f };
		int currentWeightCount = 0;
	};

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		// メッシュの元の頂点数分のウェイトデータを準備
		std::vector<WeightData> weightData(mesh->mNumVertices);

		// ボーンとウェイトの読み込み
		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string boneName = bone->mName.C_Str();

			if (modelData.skeleton.boneNameToIndexMap.find(boneName) == modelData.skeleton.boneNameToIndexMap.end()) {
				Bone newBone;
				newBone.name = boneName;
				aiMatrix4x4 mat = bone->mOffsetMatrix;
				mat.Transpose();
				newBone.offsetMatrix.m[0][0] = mat.a1; newBone.offsetMatrix.m[0][1] = mat.a2; newBone.offsetMatrix.m[0][2] = mat.a3; newBone.offsetMatrix.m[0][3] = mat.a4;
				newBone.offsetMatrix.m[1][0] = mat.b1; newBone.offsetMatrix.m[1][1] = mat.b2; newBone.offsetMatrix.m[1][2] = mat.b3; newBone.offsetMatrix.m[1][3] = mat.b4;
				newBone.offsetMatrix.m[2][0] = mat.c1; newBone.offsetMatrix.m[2][1] = mat.c2; newBone.offsetMatrix.m[2][2] = mat.c3; newBone.offsetMatrix.m[2][3] = mat.c4;
				newBone.offsetMatrix.m[3][0] = mat.d1; newBone.offsetMatrix.m[3][1] = mat.d2; newBone.offsetMatrix.m[3][2] = mat.d3; newBone.offsetMatrix.m[3][3] = mat.d4;

				modelData.skeleton.bones.push_back(newBone);
				modelData.skeleton.boneNameToIndexMap[boneName] = static_cast<uint32_t>(modelData.skeleton.bones.size() - 1);
			}

			uint32_t jointIndex = modelData.skeleton.boneNameToIndexMap[boneName];

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
				const aiVertexWeight& weight = bone->mWeights[weightIndex];
				uint32_t vertexId = weight.mVertexId;
				if (weightData[vertexId].currentWeightCount < MAX_BONE_INFLUENCE) {
					weightData[vertexId].boneIndices[weightData[vertexId].currentWeightCount] = jointIndex;
					weightData[vertexId].boneWeights[weightData[vertexId].currentWeightCount] = weight.mWeight;
					weightData[vertexId].currentWeightCount++;
				}
			}
		}

		// 頂点の読み込み（VertexDataSkinnedを使う）
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < 3; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexDataSkinned vertex;
				vertex.position = { position.x * -1.0f, position.y, position.z, 1.0f };
				vertex.normal = { normal.x * -1.0f, normal.y, normal.z };
				vertex.texcoord = { texcoord.x, texcoord.y };

				// 事前に集計したウェイト情報をコピー
				for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
					vertex.boneIndices[i] = weightData[vertexIndex].boneIndices[i];
					vertex.boneWeights[i] = weightData[vertexIndex].boneWeights[i];
				}

				modelData.vertices.push_back(vertex);
			}
		}
	}

	// マテリアルの読み込み
	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString texturePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
			modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
		}
	}

	if (scene->mRootNode != nullptr) {
		modelData.rootNode = ReadNode(scene->mRootNode);
	}

	return modelData;
}

Node SkinnedModel::ReadNode(aiNode* aiNode) {
	Node node;
	node.name = aiNode->mName.C_Str();

	aiMatrix4x4 mat = aiNode->mTransformation;
	mat.Transpose();
	node.localMatrix.m[0][0] = mat.a1; node.localMatrix.m[0][1] = mat.a2; node.localMatrix.m[0][2] = mat.a3; node.localMatrix.m[0][3] = mat.a4;
	node.localMatrix.m[1][0] = mat.b1; node.localMatrix.m[1][1] = mat.b2; node.localMatrix.m[1][2] = mat.b3; node.localMatrix.m[1][3] = mat.b4;
	node.localMatrix.m[2][0] = mat.c1; node.localMatrix.m[2][1] = mat.c2; node.localMatrix.m[2][2] = mat.c3; node.localMatrix.m[2][3] = mat.c4;
	node.localMatrix.m[3][0] = mat.d1; node.localMatrix.m[3][1] = mat.d2; node.localMatrix.m[3][2] = mat.d3; node.localMatrix.m[3][3] = mat.d4;

	for (uint32_t i = 0; i < aiNode->mNumChildren; ++i) {
		node.children.push_back(ReadNode(aiNode->mChildren[i]));
	}

	return node;
}

void SkinnedModel::CreateVertexdata() {
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexDataSkinned) * modelData.vertices.size());
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexDataSkinned) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexDataSkinned);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexDataSkinned) * modelData.vertices.size());
}

void SkinnedModel::CreateMaterialData() {
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = 1;
	materialData->uvTransform = MakeIdentity4x4();
	materialData->shininess = 50.0f;
}

void SkinnedModel::CreateBoneData() {
	boneResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(SkinCluster));
	boneResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedSkinCluster));
	for (int i = 0; i < MAX_BONES; i++) {
		mappedSkinCluster->bones[i] = MakeIdentity4x4();
	}
}

Animation SkinnedModel::LoadAnimationFile(const std::string& directoryPath, const std::string& filename) {
	Animation animation;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	if (!scene || scene->mNumAnimations == 0) {
		assert(false && "アニメーションが見つかりませんでした");
		return animation;
	}

	aiAnimation* aiAnim = scene->mAnimations[0];
	float ticksPerSecond = static_cast<float>(aiAnim->mTicksPerSecond != 0.0 ? aiAnim->mTicksPerSecond : 25.0f);
	animation.duration = static_cast<float>(aiAnim->mDuration) / ticksPerSecond;

	for (uint32_t channelIndex = 0; channelIndex < aiAnim->mNumChannels; channelIndex++) {
		aiNodeAnim* nodeAnim = aiAnim->mChannels[channelIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnim->mNodeName.C_Str()];

		for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumPositionKeys; keyIndex++) {
			aiVectorKey& key = nodeAnim->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(key.mTime) / ticksPerSecond;
			keyframe.value = { key.mValue.x * -1.0f, key.mValue.y, key.mValue.z };
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumRotationKeys; keyIndex++) {
			aiQuatKey& key = nodeAnim->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = static_cast<float>(key.mTime) / ticksPerSecond;
			keyframe.value = { key.mValue.x, -key.mValue.y, -key.mValue.z, key.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnim->mNumScalingKeys; keyIndex++) {
			aiVectorKey& key = nodeAnim->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(key.mTime) / ticksPerSecond;
			keyframe.value = { key.mValue.x, key.mValue.y, key.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}
	return animation;
}

Vector3 SkinnedModel::CalculateTranslateValue(const std::vector<KeyframeVector3>& keyframes, float time) {
	if (keyframes.empty()) return { 0.0f, 0.0f, 0.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;
	if (time >= keyframes.back().time) return keyframes.back().value;

	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
			float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
			Vector3 p1 = keyframes[i].value;
			Vector3 p2 = keyframes[i + 1].value;
			return { p1.x + (p2.x - p1.x) * t, p1.y + (p2.y - p1.y) * t, p1.z + (p2.z - p1.z) * t };
		}
	}
	return keyframes[0].value;
}

Quaternion SkinnedModel::CalculateRotationValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {
	if (keyframes.empty()) return { 0.0f, 0.0f, 0.0f, 1.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;
	if (time >= keyframes.back().time) return keyframes.back().value;

	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
			float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
			return Slerp(keyframes[i].value, keyframes[i + 1].value, t);
		}
	}
	return keyframes[0].value;
}

Vector3 SkinnedModel::CalculateScaleValue(const std::vector<KeyframeVector3>& keyframes, float time) {
	if (keyframes.empty()) return { 1.0f, 1.0f, 1.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;
	if (time >= keyframes.back().time) return keyframes.back().value;

	for (size_t i = 0; i < keyframes.size() - 1; ++i) {
		if (time >= keyframes[i].time && time <= keyframes[i + 1].time) {
			float t = (time - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time);
			Vector3 s1 = keyframes[i].value;
			Vector3 s2 = keyframes[i + 1].value;
			return { s1.x + (s2.x - s1.x) * t, s1.y + (s2.y - s1.y) * t, s1.z + (s2.z - s1.z) * t };
		}
	}
	return keyframes[0].value;
}

void SkinnedModel::UpdateAnimation(const Animation& animation, float time) {
	Matrix4x4 identity = MakeIdentity4x4();
	UpdateNodeAnimation(&modelData.rootNode, animation, time, identity);
}

void SkinnedModel::UpdateNodeAnimation(Node* node, const Animation& animation, float time, const Matrix4x4& parentGlobalMatrix) {
	Matrix4x4 localMatrix = node->localMatrix;

	auto it = animation.nodeAnimations.find(node->name);
	if (it != animation.nodeAnimations.end()) {
		const NodeAnimation& nodeAnim = it->second;

		// テンプレートで作成されていた AnimationCurve に対応するため、.keyframes を渡す
		Vector3 translate = CalculateTranslateValue(nodeAnim.translate.keyframes, time);
		Quaternion rotate = CalculateRotationValue(nodeAnim.rotate.keyframes, time);
		Vector3 scale = CalculateScaleValue(nodeAnim.scale.keyframes, time);

		Matrix4x4 scaleMat = MakeScaleMatrix(scale);
		Matrix4x4 rotateMat = MakeRotateMatrix(rotate);
		Matrix4x4 translateMat = MakeTranslateMatrix(translate);
		localMatrix = Multiply(Multiply(scaleMat, rotateMat), translateMat);
	}

	Matrix4x4 globalMatrix = Multiply(localMatrix, parentGlobalMatrix);

	if (modelData.skeleton.boneNameToIndexMap.find(node->name) != modelData.skeleton.boneNameToIndexMap.end()) {
		uint32_t boneIndex = modelData.skeleton.boneNameToIndexMap[node->name];
		modelData.skeleton.bones[boneIndex].globalTransform = globalMatrix;
	}

	for (Node& child : node->children) {
		UpdateNodeAnimation(&child, animation, time, globalMatrix);
	}
}

void SkinnedModel::UpdateBoneMatrix() {
	for (size_t i = 0; i < modelData.skeleton.bones.size(); ++i) {
		mappedSkinCluster->bones[i] = Multiply(modelData.skeleton.bones[i].offsetMatrix, modelData.skeleton.bones[i].globalTransform);
	}
}