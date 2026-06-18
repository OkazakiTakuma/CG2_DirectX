#pragma once
#include "Vector.h"
#include <vector>
#include <random>


// --- ランダムな小数を返す便利関数（-1.0 ～ 1.0） ---
inline float RandomFloatRange(float min, float max) {
    float r = (float)rand() / (float)RAND_MAX; // 0.0 ~ 1.0
    return min + r * (max - min);
}

// --- 中点変位法の再帰関数 ---
inline void GenerateLightningRecursive(const Vector3& start, const Vector3& end, float displacement, int generation, std::vector<Vector3>& outPath) {
    if (generation <= 0) {
        return; // 指定回数繰り返したら終了
    }

    // 1. 中点を計算
    Vector3 mid = {
        (start.x + end.x) * 0.5f,
        (start.y + end.y) * 0.5f,
        (start.z + end.z) * 0.5f
    };

    // 2. 中点をランダムにずらす（変位）
    mid.x += RandomFloatRange(-displacement, displacement);
    mid.y += RandomFloatRange(-displacement, displacement);
    mid.z += RandomFloatRange(-displacement, displacement);

    // 3. 左半分の線分でさらに再帰（分割）
    // （※次回はずらす幅を半分にする）
    GenerateLightningRecursive(start, mid, displacement * 0.5f, generation - 1, outPath);

    // 4. 計算された中点をリストに追加
    outPath.push_back(mid);

    // 5. 右半分の線分でさらに再帰（分割）
    GenerateLightningRecursive(mid, end, displacement * 0.5f, generation - 1, outPath);
}

// --- 雷の経路（頂点リスト）を作成するメイン関数 ---
inline std::vector<Vector3> CreateLightningPath(const Vector3& start, const Vector3& end, float initialDisplacement, int generations) {
    std::vector<Vector3> path;

    path.push_back(start); // 始点
    GenerateLightningRecursive(start, end, initialDisplacement, generations, path); // 中間のギザギザ
    path.push_back(end);   // 終点

    return path;
}

inline std::vector<VertexData> GenerateRingVertices(uint32_t segments, float outerRadius, float innerRadius) {
	std::vector<VertexData> vertices;
	vertices.reserve(segments * 6);

	for (uint32_t i = 0; i < segments; ++i) {
		// 円周をどれくらい進んだかの割合（0.0 ～ 1.0）
		float ratio1 = static_cast<float>(i) / segments;
		float ratio2 = static_cast<float>(i + 1) / segments;

		float angle1 = ratio1 * 2.0f * std::numbers::pi_v<float>;
		float angle2 = ratio2 * 2.0f * std::numbers::pi_v<float>;

		// 座標の計算
		Vector4 outer1 = { outerRadius * std::cos(angle1), outerRadius * std::sin(angle1), 0.0f, 1.0f };
		Vector4 inner1 = { innerRadius * std::cos(angle1), innerRadius * std::sin(angle1), 0.0f, 1.0f };
		Vector4 outer2 = { outerRadius * std::cos(angle2), outerRadius * std::sin(angle2), 0.0f, 1.0f };
		Vector4 inner2 = { innerRadius * std::cos(angle2), innerRadius * std::sin(angle2), 0.0f, 1.0f };

		// ─── ★ここがポイント！UV座標の設定 ───
		// 外側は V = 0.0f (テクスチャの上), 内側は V = 1.0f (テクスチャの下)
		// Uは円周に沿って 0.0f から 1.0f へ進む
		Vector2 uvOuter1 = { ratio1, 0.0f };
		Vector2 uvInner1 = { ratio1, 1.0f };
		Vector2 uvOuter2 = { ratio2, 0.0f };
		Vector2 uvInner2 = { ratio2, 1.0f };

		Vector3 normal = { 0.0f, 0.0f, -1.0f };

		// 1つ目の三角形 (外1, 内1, 外2)
		vertices.push_back({ outer1, uvOuter1, normal });
		vertices.push_back({ inner1, uvInner1, normal });
		vertices.push_back({ outer2, uvOuter2, normal });

		// 2つ目の三角形 (内1, 内2, 外2)
		vertices.push_back({ inner1, uvInner1, normal });
		vertices.push_back({ inner2, uvInner2, normal });
		vertices.push_back({ outer2, uvOuter2, normal });
	}

	return vertices;
}

// --- 追加: 円筒側面用の頂点生成関数 ---
inline std::vector<VertexData> GenerateCylinderSideVertices(uint32_t segments, float radius, float height) {
    std::vector<VertexData> vertices;
    vertices.reserve(segments * 6);

    for (uint32_t i = 0; i < segments; ++i) {
        float ratio1 = static_cast<float>(i) / segments;
        float ratio2 = static_cast<float>(i + 1) / segments;

        float angle1 = ratio1 * 2.0f * std::numbers::pi_v<float>;
        float angle2 = ratio2 * 2.0f * std::numbers::pi_v<float>;

        // 座標計算
        Vector4 p1_b = { radius * std::cos(angle1), 0.0f, radius * std::sin(angle1), 1.0f };
        Vector4 p1_t = { radius * std::cos(angle1), height, radius * std::sin(angle1), 1.0f };
        Vector4 p2_b = { radius * std::cos(angle2), 0.0f, radius * std::sin(angle2), 1.0f };
        Vector4 p2_t = { radius * std::cos(angle2), height, radius * std::sin(angle2), 1.0f };

        // 法線計算（各頂点の円周上の方向）
        Vector3 n1 = { std::cos(angle1), 0.0f, std::sin(angle1) };
        Vector3 n2 = { std::cos(angle2), 0.0f, std::sin(angle2) };

        // 三角形1
        vertices.push_back({ p1_b, {ratio1, 1.0f}, n1 });
        vertices.push_back({ p1_t, {ratio1, 0.0f}, n1 });
        vertices.push_back({ p2_t, {ratio2, 0.0f}, n2 });
        // 三角形2
        vertices.push_back({ p1_b, {ratio1, 1.0f}, n1 });
        vertices.push_back({ p2_t, {ratio2, 0.0f}, n2 });
        vertices.push_back({ p2_b, {ratio2, 1.0f}, n2 });
    }
    return vertices;
}