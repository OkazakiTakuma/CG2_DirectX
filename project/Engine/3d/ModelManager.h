#pragma once
#include "struct.h"
#include <map>
#include <memory>
#include <string>


class Model;
class ModelCommon;
class DirectXCommon;
class ModelManager {
public:
	void Inithialize(DirectXCommon* dxCommon);
	static ModelManager* GetInstance();
	void Finalize();
	void LoadModel(const std::string& filePath,bool isAnimation=false,const std::string& directoryPath="");
	Model* FindModel(const std::string& filePath);



private:
	std::map<std::string, std::unique_ptr<Model>> models;
	static ModelManager* instance;
	ModelManager() = default;
	~ModelManager() = default;
	ModelManager(ModelManager&) = default;
	ModelManager& operator=(const ModelManager&) = delete;
	ModelCommon* modelCommon = nullptr;
};