#pragma once
#include"BaseScene.h"
#include<string>

class AbstractSceneFactory {
public:

	virtual ~AbstractSceneFactory() = default;

	virtual std::unique_ptr<BaseScene> CreateScene(const std::string& sceneName) = 0;
};