#pragma once

#include "TrailRenderer.h"
#include "../../base/GameTime.h"
#include "../../flame/Component.h"
#include "../../flame/GameObject.h"
#include <algorithm>
#include <deque>
#include <vector>

class TrailRendererComponent : public Component {
public:
	void Initialize() override {
		Clear();
		if (GetOwner()) {
			points_.push_back({GetOwner()->GetTransform().translate - GetReferenceOrigin(), 0.0f});
		}
	}

	void Update() override {
		const float deltaTime = GameTime::GetDeltaTime();
		for (Point& point : points_) {
			point.age += deltaTime;
		}
		while (!points_.empty() && points_.front().age >= lifeTime_) {
			points_.pop_front();
		}

		if (!emitting_ || !GetOwner()) {
			return;
		}
		const Vector3 position = GetOwner()->GetTransform().translate + offset_ - GetReferenceOrigin();
		if (points_.empty() || Length(position - points_.back().position) >= minSegmentLength_) {
			points_.push_back({position, 0.0f});
			while (points_.size() > maxPointCount_) {
				points_.pop_front();
			}
		}
	}

	void Draw3D() override {
		if (points_.size() < 2 || lifeTime_ <= 0.0f) {
			return;
		}
		std::vector<TrailRenderPoint> renderPoints;
		renderPoints.reserve(points_.size());
		const Vector3 referenceOrigin = GetReferenceOrigin();
		for (const Point& point : points_) {
			renderPoints.push_back({point.position + referenceOrigin, 1.0f - (std::clamp)(point.age / lifeTime_, 0.0f, 1.0f)});
		}
		TrailRenderer::GetInstance()->Submit(renderPoints, width_, headColor_, tailColor_);
	}

	void Clear() { points_.clear(); }
	void SetEmitting(bool emitting) { emitting_ = emitting; }
	bool IsEmitting() const { return emitting_; }
	void SetWidth(float width) { width_ = (std::max)(0.0f, width); }
	float GetWidth() const { return width_; }
	void SetLifeTime(float lifeTime) { lifeTime_ = (std::max)(0.01f, lifeTime); }
	float GetLifeTime() const { return lifeTime_; }
	void SetMinSegmentLength(float length) { minSegmentLength_ = (std::max)(0.001f, length); }
	float GetMinSegmentLength() const { return minSegmentLength_; }
	void SetMaxPointCount(size_t count) { maxPointCount_ = (std::max)(size_t{2}, count); }
	void SetHeadColor(const Vector4& color) { headColor_ = color; }
	void SetTailColor(const Vector4& color) { tailColor_ = color; }
	void SetOffset(const Vector3& offset) { offset_ = offset; }
	void SetPositionReference(GameObject* referenceObject) {
		const Vector3 oldOrigin = GetReferenceOrigin();
		const Vector3 newOrigin = referenceObject ? referenceObject->GetTransform().translate : Vector3{};
		for (Point& point : points_) {
			point.position = point.position + oldOrigin - newOrigin;
		}
		referenceObject_ = referenceObject;
	}
	GameObject* GetPositionReference() const { return referenceObject_; }

private:
	struct Point {
		Vector3 position{};
		float age = 0.0f;
	};

	Vector3 GetReferenceOrigin() const {
		return referenceObject_ ? referenceObject_->GetTransform().translate : Vector3{};
	}

	std::deque<Point> points_;
	float width_ = 0.75f;
	float lifeTime_ = 0.35f;
	float minSegmentLength_ = 0.05f;
	size_t maxPointCount_ = 64;
	Vector4 headColor_{0.25f, 0.85f, 1.0f, 0.9f};
	Vector4 tailColor_{0.05f, 0.20f, 1.0f, 0.0f};
	Vector3 offset_{};
	GameObject* referenceObject_ = nullptr;
	bool emitting_ = true;
};
