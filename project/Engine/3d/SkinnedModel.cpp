#include "SkinnedModel.h"
#include "../2d/TextureManager.h"
#include "../3d/ModelCommon.h"
#include "../base/Logger.h"
#include "SkinnedObject3dCommon.h"
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



void SkinnedModel::CreateGpuSkinningBuffers() {
	// Create structured buffer on default heap for input (InVertex) and output (OutVertex)
	// Build InVertex array from modelData.vertices (which currently contains VertexDataSkinned)
	struct InVertex {
		float position[3];
		float texcoord[2];
		float normal[3];
		int boneIndices[4];
		float boneWeights[4];
	};
	struct OutVertex {
		float position[3];
		float texcoord[2];
		float normal[3];
	};

	size_t vertexCount = modelData.vertices.size();
	std::vector<InVertex> inVerts(vertexCount);
	for (size_t i = 0; i < vertexCount; ++i) {
		const auto& v = modelData.vertices[i];
		inVerts[i].position[0] = v.position.x; inVerts[i].position[1] = v.position.y; inVerts[i].position[2] = v.position.z;
		inVerts[i].texcoord[0] = v.texcoord.x; inVerts[i].texcoord[1] = v.texcoord.y;
		inVerts[i].normal[0] = v.normal.x; inVerts[i].normal[1] = v.normal.y; inVerts[i].normal[2] = v.normal.z;
		for (int k = 0; k < 4; ++k) { inVerts[i].boneIndices[k] = v.boneIndices[k]; inVerts[i].boneWeights[k] = v.boneWeights[k]; }
	}

	// Create default buffers using DirectXCommon helper
	auto dx = modelCommon_->GetDxCommon();
	vertexStructuredResource = dx->CreateDefaultBufferWithData(inVerts.data(), sizeof(InVertex) * vertexCount);

	// Output buffer: allocate zeroed buffer
	std::vector<OutVertex> outInit(vertexCount);
	skinnedOutputResource = dx->CreateDefaultBufferWithData(outInit.data(), sizeof(OutVertex) * vertexCount);

	// Create SRV/UAV descriptors via SrvManager (shader-visible descriptor heap)
	// Allocate indices and create descriptors
	uint32_t srvIndex = SrvManager::GetInstance()->Allocate();
	SrvManager::GetInstance()->CreateSRVforStructuredBuffer(srvIndex, vertexStructuredResource.Get(), static_cast<UINT>(vertexCount), sizeof(InVertex));
	vertexStructuredSrvIndex = srvIndex;

	uint32_t uavIndex = SrvManager::GetInstance()->Allocate();
	SrvManager::GetInstance()->CreateUAVforStructuredBuffer(uavIndex, skinnedOutputResource.Get(), static_cast<UINT>(vertexCount), sizeof(OutVertex));
	skinnedOutputUavIndex = uavIndex;
}

SkinnedModel::~SkinnedModel() {
	Finalize();
}

void SkinnedModel::Finalize() {
	if (vertexResource) {
		vertexResource->Unmap(0, nullptr);
	}
	if (indexResource) {
		indexResource->Unmap(0, nullptr);
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
	// コマンドリスト取得
	ID3D12GraphicsCommandList* commandList = modelCommon_->GetDxCommon()->GetCommandList().Get();

	// GPUスキニングが準備されているなら Dispatch を呼び、出力バッファを頂点バッファとして使う
	if (skinnedOutputResource && vertexStructuredResource && mappedSkinCluster) {
		// SkinnedObject3dCommon 側で DispatchSkinning を実装している前提
		// Get GPU descriptor handles from SrvManager
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandle = SrvManager::GetInstance()->GetGPUDescriptorHandle(vertexStructuredSrvIndex);
		D3D12_GPU_DESCRIPTOR_HANDLE uavHandle = SrvManager::GetInstance()->GetGPUDescriptorHandle(skinnedOutputUavIndex);

		// Transition skinned output to UNORDERED_ACCESS for compute write
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = skinnedOutputResource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_GENERIC_READ; // created as GENERIC_READ
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			commandList->ResourceBarrier(1, &barrier);
		}

		// Dispatch compute shader to perform skinning
		SkinnedObject3dCommon::GetInstance()->DispatchSkinning(commandList, vertexStructuredResource.Get(), skinnedOutputResource.Get(), boneResource->GetGPUVirtualAddress(), static_cast<UINT>(modelData.vertices.size()), srvHandle, uavHandle);

		// Transition skinned output from UAV to VERTEX_AND_CONSTANT_BUFFER for use as vertex buffer
		{
			D3D12_RESOURCE_BARRIER barrier{};
			barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
			barrier.Transition.pResource = skinnedOutputResource.Get();
			barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
			barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
			barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
			commandList->ResourceBarrier(1, &barrier);
		}

		// 出力バッファを頂点バッファとしてセット
		D3D12_VERTEX_BUFFER_VIEW skinnedVBV{};
		skinnedVBV.BufferLocation = skinnedOutputResource->GetGPUVirtualAddress();
		skinnedVBV.SizeInBytes = UINT(sizeof(float) * (3 + 2 + 3) * modelData.vertices.size()); // position(3) + tex(2) + normal(3)
		skinnedVBV.StrideInBytes = sizeof(float) * (3 + 2 + 3);
		commandList->IASetVertexBuffers(0, 1, &skinnedVBV);
	}
	else {
		// フォールバック: 従来の頂点バッファを使用
		commandList->IASetVertexBuffers(0, 1, &vertexBufferView);
	}

	// もしインデックスがあるならインデックスバッファをセットしてIndexedDraw
	if (!modelData.indices.empty()) {
		commandList->IASetIndexBuffer(&indexBufferView);
		commandList->DrawIndexedInstanced(static_cast<UINT>(modelData.indices.size()), 1, 0, 0, 0);
	}
	else {
		commandList->DrawInstanced(static_cast<UINT>(modelData.vertices.size()), 1, 0, 0);
	}
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

		// 各頂点を一度だけ追加してインデックスを作成する
		uint32_t baseVertex = static_cast<uint32_t>(modelData.vertices.size());
		for (uint32_t v = 0; v < mesh->mNumVertices; ++v) {
			aiVector3D& position = mesh->mVertices[v];
			aiVector3D& normal = mesh->mNormals[v];
			aiVector3D& texcoord = mesh->mTextureCoords[0][v];

			VertexDataSkinned vertex;
			vertex.position = { position.x * -1.0f, position.y, position.z };
			vertex.normal = { normal.x * -1.0f, normal.y, normal.z };
			vertex.texcoord = { texcoord.x, texcoord.y };

			for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
				vertex.boneIndices[i] = weightData[v].boneIndices[i];
				vertex.boneWeights[i] = weightData[v].boneWeights[i];
			}

			modelData.vertices.push_back(vertex);
		}

		// フェースからインデックスを作成（baseVertex を加算）
		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);
			modelData.indices.push_back(baseVertex + face.mIndices[0]);
			modelData.indices.push_back(baseVertex + face.mIndices[1]);
			modelData.indices.push_back(baseVertex + face.mIndices[2]);
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
	// 頂点バッファ作成
	size_t vertexBufferSize = sizeof(VertexDataSkinned) * modelData.vertices.size();
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(vertexBufferSize);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(vertexBufferSize);
	vertexBufferView.StrideInBytes = sizeof(VertexDataSkinned);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), vertexBufferSize);

	// インデックスバッファ作成（存在する場合）
	if (!modelData.indices.empty()) {
		size_t indexBufferSize = sizeof(uint32_t) * modelData.indices.size();
		indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(indexBufferSize);
		// Map に渡すポインタは uint32_t*、一時変数を用意
		uint32_t* mappedIndices = nullptr;
		indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndices));
		std::memcpy(mappedIndices, modelData.indices.data(), indexBufferSize);
		// 注意: 永続的にマップしているので Unmap は Finalize で行う
		indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
		indexBufferView.Format = DXGI_FORMAT_R32_UINT;
		indexBufferView.SizeInBytes = UINT(indexBufferSize);
	}
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

	// 二分探索で区間を見つける
	size_t left = 0;
	size_t right = keyframes.size() - 1;
	while (left + 1 < right) {
		size_t mid = (left + right) / 2;
		if (time < keyframes[mid].time) right = mid;
		else left = mid;
	}
	const auto& k1 = keyframes[left];
	const auto& k2 = keyframes[left + 1];
	float t = (time - k1.time) / (k2.time - k1.time);
	return { k1.value.x + (k2.value.x - k1.value.x) * t,
			 k1.value.y + (k2.value.y - k1.value.y) * t,
			 k1.value.z + (k2.value.z - k1.value.z) * t };
}

Quaternion SkinnedModel::CalculateRotationValue(const std::vector<KeyframeQuaternion>& keyframes, float time) {
	if (keyframes.empty()) return { 0.0f, 0.0f, 0.0f, 1.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;
	if (time >= keyframes.back().time) return keyframes.back().value;

	// 二分探索で区間を見つける
	size_t left = 0;
	size_t right = keyframes.size() - 1;
	while (left + 1 < right) {
		size_t mid = (left + right) / 2;
		if (time < keyframes[mid].time) right = mid;
		else left = mid;
	}
	const auto& k1 = keyframes[left];
	const auto& k2 = keyframes[left + 1];
	float t = (time - k1.time) / (k2.time - k1.time);
	return Slerp(k1.value, k2.value, t);
}

Vector3 SkinnedModel::CalculateScaleValue(const std::vector<KeyframeVector3>& keyframes, float time) {
	if (keyframes.empty()) return { 1.0f, 1.0f, 1.0f };
	if (keyframes.size() == 1 || time <= keyframes[0].time) return keyframes[0].value;
	if (time >= keyframes.back().time) return keyframes.back().value;

	// 二分探索で区間を見つける
	size_t left = 0;
	size_t right = keyframes.size() - 1;
	while (left + 1 < right) {
		size_t mid = (left + right) / 2;
		if (time < keyframes[mid].time) right = mid;
		else left = mid;
	}
	const auto& k1 = keyframes[left];
	const auto& k2 = keyframes[left + 1];
	float t = (time - k1.time) / (k2.time - k1.time);
	return { k1.value.x + (k2.value.x - k1.value.x) * t,
			 k1.value.y + (k2.value.y - k1.value.y) * t,
			 k1.value.z + (k2.value.z - k1.value.z) * t };
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