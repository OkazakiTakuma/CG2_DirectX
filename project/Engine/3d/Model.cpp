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

namespace {
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Quaternion& rotate, const Vector3& translate) {
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateMatrix = MakeRotateMatrix(rotate);
	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
	return Multiply(Multiply(scaleMatrix, rotateMatrix), translateMatrix);
}

Matrix4x4 ConvertAssimpMatrix(const aiMatrix4x4& matrix) {
	Matrix4x4 result{};
	result.m[0][0] = matrix.a1;
	result.m[0][1] = matrix.b1;
	result.m[0][2] = matrix.c1;
	result.m[0][3] = matrix.d1;
	result.m[1][0] = matrix.a2;
	result.m[1][1] = matrix.b2;
	result.m[1][2] = matrix.c2;
	result.m[1][3] = matrix.d2;
	result.m[2][0] = matrix.a3;
	result.m[2][1] = matrix.b3;
	result.m[2][2] = matrix.c3;
	result.m[2][3] = matrix.d3;
	result.m[3][0] = matrix.a4;
	result.m[3][1] = matrix.b4;
	result.m[3][2] = matrix.c4;
	result.m[3][3] = matrix.d4;

	const Matrix4x4 mirrorX = MakeScaleMatrix({-1.0f, 1.0f, 1.0f});
	return Multiply(mirrorX, Multiply(result, mirrorX));
}

Vector3 TransformNormal(const Vector3& normal, const Matrix4x4& matrix) {
	Vector3 result{};
	result.x = normal.x * matrix.m[0][0] + normal.y * matrix.m[1][0] + normal.z * matrix.m[2][0];
	result.y = normal.x * matrix.m[0][1] + normal.y * matrix.m[1][1] + normal.z * matrix.m[2][1];
	result.z = normal.x * matrix.m[0][2] + normal.y * matrix.m[1][2] + normal.z * matrix.m[2][2];
	return result;
}
}

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename, const bool isAnimation) {
	this->modelCommon_ = modelCommon;
	this->isAnimation_ = isAnimation;
	modelData = LoadModelFile(directoryPath, filename);
	if (isAnimation) {
		animation = LoadAnimation(directoryPath, filename);
	}
	CreateVertexdata();
	CreateIndexData();
	CreateMaterialData();
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

Model::~Model() {
	Finalize();
}

void Model::Finalize() {
	if (vertexResource) {
		vertexResource->Unmap(0, nullptr);
	}
	if (materialResource) {
		materialResource->Unmap(0, nullptr);
	}
	if (indexResource) {
		indexResource->Unmap(0, nullptr);
	}

	vertexResource.Reset();
	indexResource.Reset();
	materialResource.Reset();

	materialData = nullptr;
	vertexData = nullptr;
	modelCommon_ = nullptr;

	vertexBufferView = {};
	indexBufferView = {};
}

void Model::Draw() {
	modelCommon_->GetDxCommon()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	modelCommon_->GetDxCommon()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	modelCommon_->GetDxCommon()->GetCommandList()->SetGraphicsRootDescriptorTable(3, TextureManager::GetInstance()->GetSRVHandleGPU(modelData.material.textureFilePath));

	modelCommon_->GetDxCommon()->GetCommandList()->DrawIndexedInstanced(UINT(modelData.indices.size()), 1, 0, 0, 0);
}

ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {
	ModelData modelData;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene != nullptr && scene->HasMeshes());

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		const uint32_t vertexOffset = static_cast<uint32_t>(modelData.vertices.size());
		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; vertexIndex++) {
			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

			VertexData vertex;
			vertex.position = { position.x * -1.0f, position.y, position.z, 1.0f };
			vertex.normal = { normal.x * -1.0f, normal.y, normal.z };
			vertex.texcoord = { texcoord.x, texcoord.y };

			modelData.vertices.push_back(vertex);
		}

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; boneIndex++) {
			aiBone* bone = mesh->mBones[boneIndex];
			JointWeghtData& jointWeightData = modelData.skincluserData[bone->mName.C_Str()];
			jointWeightData.inverseBindPoseMatrix = ConvertAssimpMatrix(bone->mOffsetMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; weightIndex++) {
				const aiVertexWeight& weight = bone->mWeights[weightIndex];
				jointWeightData.vertexWeights.push_back({
				    weight.mWeight,
				    vertexOffset + weight.mVertexId
				});
			}
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < 3; element++) {
				modelData.indices.push_back(vertexOffset + face.mIndices[element]);
			}
		}
	}

	for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; materialIndex++) {
		aiMaterial* material = scene->mMaterials[materialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
			aiString texturePath;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
			modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
			break;
		}
		if (material->GetTextureCount(aiTextureType_BASE_COLOR) != 0) {
			aiString texturePath;
			material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath);
			modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
			break;
		}
	}
	if (modelData.material.textureFilePath.empty()) {
		modelData.material.textureFilePath = "Resources/uvChecker.png";
	}

	if (scene->mRootNode != nullptr) {
		modelData.rootNode = ReadNode(scene->mRootNode);
	}

	return modelData;
}

Animation Model::LoadAnimation(const std::string& directoryPath, const std::string& filename)
{
	Animation animation;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;

	const aiScene* scene = importer.ReadFile(filePath.c_str(), 0);
	assert(scene->mNumAnimations != 0);

	aiAnimation* animationAssimp = scene->mAnimations[0];
	double ticksPerSecond = animationAssimp->mTicksPerSecond != 0.0 ? animationAssimp->mTicksPerSecond : 25.0;
	animation.duration = static_cast<float>(animationAssimp->mDuration / ticksPerSecond);

	for (uint32_t channeIndex = 0; channeIndex < animationAssimp->mNumChannels; channeIndex++) {
		aiNodeAnim* nodeAnimtionAssimp = animationAssimp->mChannels[channeIndex];
		NodeAnimation& nodeAnimation = animation.nodeAnimations[nodeAnimtionAssimp->mNodeName.C_Str()];

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimtionAssimp->mNumPositionKeys; keyIndex++)
		{
			aiVectorKey& keyAssimp = nodeAnimtionAssimp->mPositionKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime / ticksPerSecond);
			keyframe.value = { -keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.translate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimtionAssimp->mNumRotationKeys; keyIndex++)
		{
			aiQuatKey& keyAssimp = nodeAnimtionAssimp->mRotationKeys[keyIndex];
			KeyframeQuaternion keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime / ticksPerSecond);

			keyframe.value = { keyAssimp.mValue.x, -keyAssimp.mValue.y, -keyAssimp.mValue.z, keyAssimp.mValue.w };
			nodeAnimation.rotate.keyframes.push_back(keyframe);
		}

		for (uint32_t keyIndex = 0; keyIndex < nodeAnimtionAssimp->mNumScalingKeys; keyIndex++)
		{
			aiVectorKey& keyAssimp = nodeAnimtionAssimp->mScalingKeys[keyIndex];
			KeyframeVector3 keyframe;
			keyframe.time = static_cast<float>(keyAssimp.mTime / ticksPerSecond);

			keyframe.value = { keyAssimp.mValue.x, keyAssimp.mValue.y, keyAssimp.mValue.z };
			nodeAnimation.scale.keyframes.push_back(keyframe);
		}
	}

	return animation;
}
Node Model::ReadNode(aiNode* aiNode) {
	Node result;
	aiVector3D scale;
	aiQuaternion rotate;
	aiVector3D translate;
	aiNode->mTransformation.Decompose(scale, rotate, translate);

	result.transform.scale = {scale.x, scale.y, scale.z};
	result.transform.rotate = {rotate.x, -rotate.y, -rotate.z, rotate.w};
	result.transform.translate = {-translate.x, translate.y, translate.z};
	result.localMatrix = MakeAffineMatrix(result.transform.scale, result.transform.rotate, result.transform.translate);

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
	originalVertices_ = modelData.vertices;
	vertexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());


	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::CreateIndexData() {
	indexResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());

	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = UINT(sizeof(uint32_t) * modelData.indices.size());
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	uint32_t* indexDataModel = nullptr;
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexDataModel));
	std::memcpy(indexDataModel, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());
}

void Model::CreateMaterialData() {
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = 1;
	materialData->uvTransform = MakeIdentity4x4();

	materialData->shininess = 20.0f;
}

void Model::ApplySkinning(const Skeleton& skeleton) {
	if (modelData.skincluserData.empty() || originalVertices_.empty() || !vertexData) {
		return;
	}

	std::vector<VertexData> skinnedVertices = originalVertices_;
	std::vector<float> totalWeights(originalVertices_.size(), 0.0f);

	for (VertexData& vertex : skinnedVertices) {
		vertex.position = {0.0f, 0.0f, 0.0f, 0.0f};
		vertex.normal = {0.0f, 0.0f, 0.0f};
	}

	for (const auto& [jointName, jointWeightData] : modelData.skincluserData) {
		const auto jointItr = skeleton.jointMap.find(jointName);
		if (jointItr == skeleton.jointMap.end()) {
			continue;
		}

		const int32_t jointIndex = jointItr->second;
		if (jointIndex < 0 || jointIndex >= static_cast<int32_t>(skeleton.joints.size())) {
			continue;
		}

		const Matrix4x4 skinningMatrix = Multiply(jointWeightData.inverseBindPoseMatrix, skeleton.joints[jointIndex].skeletonSpaceMatrix);
		for (const VertexWeghtData& vertexWeight : jointWeightData.vertexWeights) {
			if (vertexWeight.vertexIndex >= originalVertices_.size()) {
				continue;
			}

			const VertexData& sourceVertex = originalVertices_[vertexWeight.vertexIndex];
			const Vector3 skinnedPosition = Transformation(
			    {sourceVertex.position.x, sourceVertex.position.y, sourceVertex.position.z},
			    skinningMatrix
			);
			const Vector3 skinnedNormal = TransformNormal(sourceVertex.normal, skinningMatrix);

			VertexData& destinationVertex = skinnedVertices[vertexWeight.vertexIndex];
			destinationVertex.position.x += skinnedPosition.x * vertexWeight.weght;
			destinationVertex.position.y += skinnedPosition.y * vertexWeight.weght;
			destinationVertex.position.z += skinnedPosition.z * vertexWeight.weght;
			destinationVertex.position.w += sourceVertex.position.w * vertexWeight.weght;
			destinationVertex.normal.x += skinnedNormal.x * vertexWeight.weght;
			destinationVertex.normal.y += skinnedNormal.y * vertexWeight.weght;
			destinationVertex.normal.z += skinnedNormal.z * vertexWeight.weght;
			totalWeights[vertexWeight.vertexIndex] += vertexWeight.weght;
		}
	}

	for (size_t vertexIndex = 0; vertexIndex < skinnedVertices.size(); vertexIndex++) {
		if (totalWeights[vertexIndex] <= 0.0f) {
			skinnedVertices[vertexIndex] = originalVertices_[vertexIndex];
			continue;
		}

		skinnedVertices[vertexIndex].position.w = 1.0f;
		skinnedVertices[vertexIndex].normal = NormalizeReturnVector(skinnedVertices[vertexIndex].normal);
	}

	modelData.vertices = skinnedVertices;
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}
