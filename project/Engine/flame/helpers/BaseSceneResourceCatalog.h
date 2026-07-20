#pragma once

#include "../../2d/TextureManager.h"
#include "../../3d/model/Model.h"
#include "../../3d/model/ModelManager.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

/// <summary>
/// BaseSceneエディターで選択可能なリソース一覧を構築します。
/// </summary>
namespace {

inline std::vector<std::string> CollectResourceTexturePaths() {
	std::vector<std::string> paths = TextureManager::GetInstance()->GetLoadedTextureNames();
	const std::filesystem::path resourceRoot = "Resources";
	if (std::filesystem::exists(resourceRoot)) {
		for (const auto& entry : std::filesystem::recursive_directory_iterator(resourceRoot)) {
			if (!entry.is_regular_file()) {
				continue;
			}
			const std::string extension = entry.path().extension().string();
			if (extension != ".png" && extension != ".jpg" && extension != ".jpeg" && extension != ".dds") {
				continue;
			}
			std::string path = entry.path().generic_string();
			if (std::find(paths.begin(), paths.end(), path) == paths.end()) {
				paths.push_back(path);
			}
		}
	}
	std::sort(paths.begin(), paths.end());
	return paths;
}

inline std::vector<std::string> CollectResourceDdsTexturePaths() {
	std::vector<std::string> paths;
	for (const std::string& path : CollectResourceTexturePaths()) {
		if (std::filesystem::path(path).extension() == ".dds") {
			paths.push_back(path);
		}
	}
	return paths;
}

inline std::vector<std::string> CollectLoadedModelNames(bool isAnimation) {
	std::vector<std::string> result;
	const std::vector<std::string> loadedModels = ModelManager::GetInstance()->GetLoadedModelNames();
	for (const std::string& modelName : loadedModels) {
		Model* model = ModelManager::GetInstance()->FindModel(modelName);
		if (model && model->GetIsAnimation() == isAnimation) {
			result.push_back(modelName);
		}
	}
	return result;
}

inline std::vector<std::string> CollectAllLoadedModelNames() {
	std::vector<std::string> result = ModelManager::GetInstance()->GetLoadedModelNames();
	std::sort(result.begin(), result.end());
	return result;
}

inline std::vector<const char*> MakeLabelPointers(const std::vector<std::string>& labels) {
	std::vector<const char*> result;
	result.reserve(labels.size());
	for (const std::string& label : labels) {
		result.push_back(label.c_str());
	}
	return result;
}

}
