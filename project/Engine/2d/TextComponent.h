#pragma once
#include "../flame/Component.h"
#include "Sprite.h"
#include "SpriteCommon.h"
#include "TextureManager.h"
#include "Vector.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <memory>
#include <string>
#include <vector>

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
		if (text_.empty() || !GetOwner()) {
			return;
		}
		EnsureTextSprite();
		if (!textSprite_) {
			return;
		}
		const EulerTransform& transform = GetOwner()->GetTransform();
		EulerTransform spriteTransform = textSprite_->GetTransform();
		spriteTransform.translate = transform.translate;
		textSprite_->SetTransform(spriteTransform);
		textSprite_->SetAnchorPoint(AnchorToRate(anchor_));
		textSprite_->SetColor(color_);
		textSprite_->Update();
		SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
		textSprite_->Draw();
	}

	void SetText(const std::string& text) { if (text_ != text) { text_ = text; isTextureDirty_ = true; } }
	const std::string& GetText() const { return text_; }
	void SetFontName(const std::string& fontName) { const std::string value = fontName.empty() ? "Default" : fontName; if (fontName_ != value) { fontName_ = value; isTextureDirty_ = true; } }
	const std::string& GetFontName() const { return fontName_; }
	void SetFontSize(float fontSize) { const float value = fontSize < 1.0f ? 1.0f : fontSize; if (fontSize_ != value) { fontSize_ = value; isTextureDirty_ = true; } }
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
	void Finalize() override { textSprite_.reset(); }

private:
	static std::wstring ToWideString(const std::string& text) {
		if (text.empty()) return {};
		const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
		if (length <= 0) return {};
		std::wstring result(static_cast<size_t>(length), L'\0');
		MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), result.data(), length);
		return result;
	}

	void EnsureTextSprite() {
		if (!isTextureDirty_ && textSprite_) return;
		const std::wstring wideText = ToWideString(text_);
		if (wideText.empty()) {
			textSprite_.reset();
			isTextureDirty_ = false;
			return;
		}
		HDC measureDc = CreateCompatibleDC(nullptr);
		const std::wstring requestedFont = fontName_ == "Default" ? L"Meiryo" : ToWideString(fontName_);
		HFONT font = CreateFontW(-static_cast<int>(std::round(fontSize_)), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
			DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, requestedFont.c_str());
		HGDIOBJ previousFont = SelectObject(measureDc, font);
		RECT measuredRect{0, 0, 2048, 0};
		DrawTextW(measureDc, wideText.c_str(), static_cast<int>(wideText.size()), &measuredRect, DT_CALCRECT | DT_LEFT | DT_NOPREFIX);
		const int width = (std::max)(1L, measuredRect.right - measuredRect.left + 4);
		const int height = (std::max)(1L, measuredRect.bottom - measuredRect.top + 4);

		BITMAPINFO bitmapInfo{};
		bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitmapInfo.bmiHeader.biWidth = width;
		bitmapInfo.bmiHeader.biHeight = -height;
		bitmapInfo.bmiHeader.biPlanes = 1;
		bitmapInfo.bmiHeader.biBitCount = 32;
		bitmapInfo.bmiHeader.biCompression = BI_RGB;
		void* bitmapPixels = nullptr;
		HBITMAP bitmap = CreateDIBSection(measureDc, &bitmapInfo, DIB_RGB_COLORS, &bitmapPixels, nullptr, 0);
		HGDIOBJ previousBitmap = SelectObject(measureDc, bitmap);
		std::memset(bitmapPixels, 0, static_cast<size_t>(width) * height * 4);
		SetBkMode(measureDc, TRANSPARENT);
		SetTextColor(measureDc, RGB(255, 255, 255));
		RECT drawRect{2, 2, width - 2, height - 2};
		DrawTextW(measureDc, wideText.c_str(), static_cast<int>(wideText.size()), &drawRect, DT_LEFT | DT_TOP | DT_NOPREFIX);

		std::vector<uint8_t> rgbaPixels(static_cast<size_t>(width) * height * 4);
		const uint8_t* bgraPixels = static_cast<const uint8_t*>(bitmapPixels);
		for (size_t pixel = 0; pixel < static_cast<size_t>(width) * height; ++pixel) {
			const uint8_t coverage = (std::max)({bgraPixels[pixel * 4], bgraPixels[pixel * 4 + 1], bgraPixels[pixel * 4 + 2]});
			rgbaPixels[pixel * 4] = 255;
			rgbaPixels[pixel * 4 + 1] = 255;
			rgbaPixels[pixel * 4 + 2] = 255;
			rgbaPixels[pixel * 4 + 3] = coverage;
		}

		SelectObject(measureDc, previousBitmap);
		SelectObject(measureDc, previousFont);
		DeleteObject(bitmap);
		DeleteObject(font);
		DeleteDC(measureDc);

		const std::string cacheSource = text_ + "|" + fontName_ + "|" + std::to_string(static_cast<int>(std::round(fontSize_)));
		const std::string textureKey = "__runtime_text_" + std::to_string(std::hash<std::string>{}(cacheSource));
		TextureManager::GetInstance()->CreateTextureFromRGBA(textureKey, static_cast<uint32_t>(width), static_cast<uint32_t>(height), rgbaPixels);
		textSprite_ = std::make_unique<Sprite>();
		textSprite_->Initialize(textureKey);
		textSprite_->SetSize({static_cast<float>(width), static_cast<float>(height)});
		isTextureDirty_ = false;
	}

	std::string text_ = "Text";
	std::string fontName_ = "Default";
	float fontSize_ = 32.0f;
	Vector4 color_ = {1.0f, 1.0f, 1.0f, 1.0f};
	Anchor anchor_ = Anchor::TopLeft;
	bool isTextureDirty_ = true;
	std::unique_ptr<Sprite> textSprite_;
};
