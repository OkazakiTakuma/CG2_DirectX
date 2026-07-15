#pragma once
#include "../flame/Component.h"
#include "ImGuiManager.h"
#include "Vector.h"
#include <cfloat>
#include <string>

class TextComponent : public Component {
public:
	enum class Anchor {
		TopLeft,
		TopCenter,
		TopRight,
		CenterLeft,
		Center,
		CenterRight,
		BottomLeft,
		BottomCenter,
		BottomRight
	};

	void Draw2D() override {
#ifdef USE_IMGUI
		if (text_.empty() || !GetOwner()) {
			return;
		}

		ImFont* font = ImGuiManager::GetInstance()->GetFont(fontName_);
		const float size = fontSize_ <= 0.0f ? ImGui::GetFontSize() : fontSize_;
		const ImVec2 textSize = font->CalcTextSizeA(size, FLT_MAX, 0.0f, text_.c_str());
		const Vector2 anchor = AnchorToRate(anchor_);
		const EulerTransform& transform = GetOwner()->GetTransform();
		const ImVec2 gameViewPos = ImGuiManager::GetInstance()->GetGameViewContentPosition();
		const ImVec2 position(
		    gameViewPos.x + transform.translate.x - textSize.x * anchor.x,
		    gameViewPos.y + transform.translate.y - textSize.y * anchor.y
		);
		ImGui::GetForegroundDrawList()->AddText(font, size, position, ImGui::ColorConvertFloat4ToU32(ImVec4(color_.x, color_.y, color_.z, color_.w)), text_.c_str());
#endif
	}

	void SetText(const std::string& text) { text_ = text; }
	const std::string& GetText() const { return text_; }
	void SetFontName(const std::string& fontName) { fontName_ = fontName.empty() ? "Default" : fontName; }
	const std::string& GetFontName() const { return fontName_; }
	void SetFontSize(float fontSize) { fontSize_ = fontSize < 1.0f ? 1.0f : fontSize; }
	float GetFontSize() const { return fontSize_; }
	void SetColor(const Vector4& color) { color_ = color; }
	const Vector4& GetColor() const { return color_; }
	void SetAnchor(Anchor anchor) { anchor_ = anchor; }
	Anchor GetAnchor() const { return anchor_; }

	static Vector2 AnchorToRate(Anchor anchor) {
		switch (anchor) {
		case Anchor::TopCenter:
			return {0.5f, 0.0f};
		case Anchor::TopRight:
			return {1.0f, 0.0f};
		case Anchor::CenterLeft:
			return {0.0f, 0.5f};
		case Anchor::Center:
			return {0.5f, 0.5f};
		case Anchor::CenterRight:
			return {1.0f, 0.5f};
		case Anchor::BottomLeft:
			return {0.0f, 1.0f};
		case Anchor::BottomCenter:
			return {0.5f, 1.0f};
		case Anchor::BottomRight:
			return {1.0f, 1.0f};
		case Anchor::TopLeft:
		default:
			return {0.0f, 0.0f};
		}
	}

private:
	std::string text_ = "Text";
	std::string fontName_ = "Default";
	float fontSize_ = 32.0f;
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	Anchor anchor_ = Anchor::TopLeft;
};
