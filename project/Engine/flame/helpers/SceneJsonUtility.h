#pragma once

#include "../../math/Vector.h"

#pragma warning(push)
#pragma warning(disable: 26495)
#include <json.hpp>
#pragma warning(pop)

/// <summary>
/// シーンデータで使用するベクトルとJSON配列の相互変換を提供します。
/// </summary>
namespace SceneJsonUtility {

inline nlohmann::json Vector3ToJson(const Vector3& value) {
	return nlohmann::json::array({value.x, value.y, value.z});
}

inline nlohmann::json Vector4ToJson(const Vector4& value) {
	return nlohmann::json::array({value.x, value.y, value.z, value.w});
}

inline Vector3 JsonToVector3(const nlohmann::json& value, const Vector3& fallback) {
	if (!value.is_array() || value.size() < 3) {
		return fallback;
	}

	return {
	    value.at(0).get<float>(),
	    value.at(1).get<float>(),
	    value.at(2).get<float>()
	};
}

inline Vector4 JsonToVector4(const nlohmann::json& value, const Vector4& fallback) {
	if (!value.is_array() || value.size() < 4) {
		return fallback;
	}

	return {
	    value.at(0).get<float>(),
	    value.at(1).get<float>(),
	    value.at(2).get<float>(),
	    value.at(3).get<float>()
	};
}

} // namespace SceneJsonUtility
