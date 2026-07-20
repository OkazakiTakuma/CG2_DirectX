#include "PrimitiveMeshGenerator.h"

#include <cassert>
#include <cmath>
#include <numbers>

namespace PrimitiveMeshGenerator {

MeshData GenerateCylinder(float radius, float height, uint32_t subdivision, bool createTopCap, bool createBottomCap) {
	assert(subdivision >= 3);
	if (subdivision < 3) {
		return {};
	}

	MeshData mesh;
	const float halfHeight = height * 0.5f;
	const float fullTurn = 2.0f * std::numbers::pi_v<float>;

	for (uint32_t index = 0; index <= subdivision; ++index) {
		const float ratio = static_cast<float>(index) / static_cast<float>(subdivision);
		const float theta = ratio * fullTurn;
		const float cosTheta = std::cos(theta);
		const float sinTheta = std::sin(theta);
		const Vector3 normal = {cosTheta, 0.0f, sinTheta};

		mesh.vertices.push_back({{cosTheta * radius, halfHeight, sinTheta * radius, 1.0f}, {ratio, 0.0f}, normal});
		mesh.vertices.push_back({{cosTheta * radius, -halfHeight, sinTheta * radius, 1.0f}, {ratio, 1.0f}, normal});
	}

	for (uint32_t index = 0; index < subdivision; ++index) {
		const uint32_t top1 = index * 2;
		const uint32_t bottom1 = top1 + 1;
		const uint32_t top2 = (index + 1) * 2;
		const uint32_t bottom2 = top2 + 1;
		mesh.indices.insert(mesh.indices.end(), {top1, top2, bottom1, bottom1, top2, bottom2});
	}

	if (createTopCap) {
		const uint32_t centerIndex = static_cast<uint32_t>(mesh.vertices.size());
		mesh.vertices.push_back({{0.0f, halfHeight, 0.0f, 1.0f}, {0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}});
		for (uint32_t index = 0; index <= subdivision; ++index) {
			const float theta = static_cast<float>(index) / static_cast<float>(subdivision) * fullTurn;
			const float cosTheta = std::cos(theta);
			const float sinTheta = std::sin(theta);
			mesh.vertices.push_back({
			    {cosTheta * radius, halfHeight, sinTheta * radius, 1.0f},
			    {cosTheta * 0.5f + 0.5f, -sinTheta * 0.5f + 0.5f},
			    {0.0f, 1.0f, 0.0f}});
		}
		for (uint32_t index = 0; index < subdivision; ++index) {
			mesh.indices.insert(mesh.indices.end(), {centerIndex, centerIndex + 1 + index, centerIndex + 2 + index});
		}
	}

	if (createBottomCap) {
		const uint32_t centerIndex = static_cast<uint32_t>(mesh.vertices.size());
		mesh.vertices.push_back({{0.0f, -halfHeight, 0.0f, 1.0f}, {0.5f, 0.5f}, {0.0f, -1.0f, 0.0f}});
		for (uint32_t index = 0; index <= subdivision; ++index) {
			const float theta = static_cast<float>(index) / static_cast<float>(subdivision) * fullTurn;
			const float cosTheta = std::cos(theta);
			const float sinTheta = std::sin(theta);
			mesh.vertices.push_back({
			    {cosTheta * radius, -halfHeight, sinTheta * radius, 1.0f},
			    {cosTheta * 0.5f + 0.5f, sinTheta * 0.5f + 0.5f},
			    {0.0f, -1.0f, 0.0f}});
		}
		for (uint32_t index = 0; index < subdivision; ++index) {
			mesh.indices.insert(mesh.indices.end(), {centerIndex, centerIndex + 2 + index, centerIndex + 1 + index});
		}
	}

	return mesh;
}

} // namespace PrimitiveMeshGenerator
