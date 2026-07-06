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

void Model::Initialize(ModelCommon* modelCommon, const std::string& directoryPath, const std::string& filename, const bool isAnimation) {
	this->modelCommon_ = modelCommon;
	this->isAnimation_ = isAnimation;
	modelData = LoadModelFile(directoryPath, filename);
	if (isAnimation) {
		animation = LoadAnimation(directoryPath, filename);
	}
	CreateVertexdata();
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

	vertexResource.Reset();
	materialResource.Reset();

	materialData = nullptr;
	modelCommon_ = nullptr;

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

	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	assert(scene != nullptr && scene->HasMeshes());

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; meshIndex++) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; faceIndex++) {
			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3);

			for (uint32_t element = 0; element < 3; element++) {
				uint32_t vertexIndex = face.mIndices[element];
				aiVector3D& position = mesh->mVertices[vertexIndex];
				aiVector3D& normal = mesh->mNormals[vertexIndex];
				aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];

				VertexData vertex;
				vertex.position = { position.x * -1.0f, position.y, position.z, 1.0f };
				vertex.normal = { normal.x * -1.0f, normal.y, normal.z };

				vertex.texcoord = { texcoord.x, texcoord.y };

				modelData.vertices.push_back(vertex);
			}
		}
	}

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
	aiMatrix4x4 aiLocalMatrix = aiNode->mTransformation;
	aiLocalMatrix.Transpose();

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


	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	VertexData* vertexDataModel = nullptr;
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexDataModel));
	std::memcpy(vertexDataModel, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}

void Model::CreateMaterialData() {
	materialResource = modelCommon_->GetDxCommon()->CreateBufferResource(sizeof(Material));

	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));

	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = 1;
	materialData->uvTransform = MakeIdentity4x4();

	materialData->shininess = 20.0f;
}
