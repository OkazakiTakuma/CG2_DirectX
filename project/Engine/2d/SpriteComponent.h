#pragma once
#include "../flame/Component.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include <memory>
#include <string>

class SpriteComponent : public Component {
public:
	void Initialize() override {
		sprite_ = std::make_unique<Sprite>();
	}

	void Initialize(const std::string& textureFilePath) {
		if (!sprite_) {
			sprite_ = std::make_unique<Sprite>();
		}
		sprite_->Initialize(textureFilePath);
	}

	void Update() override {
		if (sprite_) {
			SyncOwnerTransformToSprite();
			sprite_->Update();
		}
	}

	void Draw2D() override {
		if (sprite_) {
			SpriteCommon::GetInstance()->SetDraw(kBlendModeNone);
			sprite_->Draw();
		}
	}

	void Finalize() override {
		sprite_.reset();
	}

	Sprite* GetSprite() const { return sprite_.get(); }

	const EulerTransform& GetTransform() { return sprite_->GetTransform(); }
	void SetTransform(const EulerTransform& transform) { sprite_->SetTransform(transform); }
	const EulerTransform& GetUVTransform() { return sprite_->GetUVTransform(); }
	void SetUVTransform(const EulerTransform& uvTransform) { sprite_->SetUVTransform(uvTransform); }
	const Vector4& GetColor() const { return sprite_->GetColor(); }
	void SetColor(const Vector4& color) { sprite_->SetColor(color); }
	const Vector2 GetSize() const { return sprite_->GetSize(); }
	void SetSize(const Vector2& size) { sprite_->SetSize(size); }
	const Vector2& GetAnchorPoint() const { return sprite_->GetAnchorPoint(); }
	void SetAnchorPoint(const Vector2& anchorPoint) { sprite_->SetAnchorPoint(anchorPoint); }
	const bool GetIsFlipX() const { return sprite_->GetIsFlipX(); }
	void SetIsFlipX(bool flipX) { sprite_->SetIsFlipX(flipX); }
	const bool GetIsFlipY() const { return sprite_->GetIsFlipY(); }
	void SetIsFlipY(bool flipY) { sprite_->SetIsFlipY(flipY); }
	const Vector2& GetTextureLeftTop() const { return sprite_->GetTextureLeftTop(); }
	void SetTextureLeftTop(const Vector2& leftTop) { sprite_->SetTextureLeftTop(leftTop); }
	const Vector2& GetTextureSize() const { return sprite_->GetTextureSize(); }
	void SetTextureSize(const Vector2& size) { sprite_->SetTextureSize(size); }

private:
	void SyncOwnerTransformToSprite() {
		if (!sprite_ || GetOwner() == nullptr) {
			return;
		}
		sprite_->SetTransform(GetOwner()->GetTransform());
	}

	std::unique_ptr<Sprite> sprite_;
};
