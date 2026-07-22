#include "PlayerSelectScene.h"

#include "SceneManager.h"
#include "repositories/PlayerStatusRepository.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <Xinput.h>

namespace {
std::unique_ptr<Sprite> CreateColorSprite() {
	// 白テクスチャへ色を乗算し、カード背景や枠へ再利用する。
	auto sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/human/white.png");
	return sprite;
}

std::unique_ptr<GameObject> CreateTextObject(const std::string& text, float fontSize) {
	// TextComponentを持つUI用GameObjectを共通設定で生成する。
	auto object = std::make_unique<GameObject>();
	TextComponent* textComponent = object->AddComponent<TextComponent>();
	textComponent->SetText(text);
	textComponent->SetFontSize(fontSize);
	textComponent->SetAnchor(TextComponent::Anchor::Center);
	return object;
}

void DrawColorSprite(Sprite* sprite, float x, float y, float width, float height, const Vector4& color) {
	EulerTransform transform = sprite->GetTransform();
	transform.translate = {x, y, 0.0f};
	sprite->SetTransform(transform);
	sprite->SetSize({width, height});
	sprite->SetColor(color);
	sprite->Update();
	sprite->Draw();
}
}

void PlayerSelectScene::Initialize() {
	// リポジトリから選択候補を読み、背景とカードUIを構築する。
	playerTypeNames_ = LoadPlayerTypeNames();
	selectedPlayerIndex_ = 0;
	if (sceneManager_) {
		const std::string& currentType = sceneManager_->GetSelectedPlayerTypeName();
		const auto found = std::find(playerTypeNames_.begin(), playerTypeNames_.end(), currentType);
		if (found != playerTypeNames_.end()) {
			selectedPlayerIndex_ = static_cast<int>(std::distance(playerTypeNames_.begin(), found));
		}
	}
	CreateUi();
}

void PlayerSelectScene::CreateUi() {
	backgroundSprite_ = CreateColorSprite();
	cardBorderSprites_.clear();
	cardSprites_.clear();
	playerTextObjects_.clear();

	titleTextObject_ = CreateTextObject("プレイヤーを選択", 48.0f);
	instructionTextObject_ = CreateTextObject(
	    "A / D・左右キー・Pad左右: 選択    Space / Enter・Pad A: 決定    Q・Pad B: 戻る", 20.0f);
	for (const std::string& playerTypeName : playerTypeNames_) {
		cardBorderSprites_.push_back(CreateColorSprite());
		cardSprites_.push_back(CreateColorSprite());
		playerTextObjects_.push_back(CreateTextObject(MakePlayerDescription(LoadPlayerStats(playerTypeName)), 22.0f));
	}
}

std::string PlayerSelectScene::MakePlayerDescription(const PlayerStats& stats) const {
	std::ostringstream stream;
	stream << stats.name
	       << "\n\nHP: " << static_cast<int>(stats.baseHealth * stats.health / 100.0f)
	       << "\n攻撃: " << static_cast<int>(stats.attack)
	       << "\n移動速度: " << std::fixed << std::setprecision(2) << stats.baseSpeed * stats.speed / 100.0f
	       << "\n攻撃速度: " << static_cast<int>(stats.attackSpeed)
	       << "\n初期攻撃: " << stats.initialAttackName;
	return stream.str();
}

void PlayerSelectScene::Update() {
	// 左右入力で候補を循環し、決定時に選択結果を保持してゲームへ遷移する。
	if (playerTypeNames_.empty() || !sceneManager_) {
		return;
	}
	Input* input = Input::GetInstance();
	const int playerCount = static_cast<int>(playerTypeNames_.size());
	if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || input->TriggerGamepadLeft()) {
		selectedPlayerIndex_ = (selectedPlayerIndex_ + playerCount - 1) % playerCount;
	}
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || input->TriggerGamepadRight()) {
		selectedPlayerIndex_ = (selectedPlayerIndex_ + 1) % playerCount;
	}
	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) || input->TriggerGamepadButton(XINPUT_GAMEPAD_A)) {
		sceneManager_->SetSelectedPlayerTypeName(playerTypeNames_[selectedPlayerIndex_]);
		sceneManager_->ChangeScene("GAMEPLAY");
		return;
	}
	if (input->TriggerKey(DIK_Q) || input->TriggerGamepadButton(XINPUT_GAMEPAD_B)) {
		sceneManager_->ChangeScene("TITLE");
	}
}

void PlayerSelectScene::Draw2D() {
	// 選択中カードだけ色と枠を変え、現在のフォーカスを視覚的に示す。
	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (!dxCommon || !backgroundSprite_) {
		return;
	}
	const float screenWidth = static_cast<float>(dxCommon->GetRenderWidth());
	const float screenHeight = static_cast<float>(dxCommon->GetRenderHeight());
	const int playerCount = static_cast<int>(playerTypeNames_.size());
	const float sideMargin = 48.0f;
	const float gap = 22.0f;
	const float availableWidth = screenWidth - sideMargin * 2.0f;
	const float cardWidth = (std::min)(300.0f, (availableWidth - gap * static_cast<float>((std::max)(0, playerCount - 1))) / (std::max)(1, playerCount));
	const float cardHeight = (std::min)(390.0f, screenHeight - 220.0f);
	const float totalWidth = cardWidth * playerCount + gap * (std::max)(0, playerCount - 1);
	const float startX = (screenWidth - totalWidth) * 0.5f;
	const float cardY = (screenHeight - cardHeight) * 0.5f + 10.0f;

	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	DrawColorSprite(backgroundSprite_.get(), 0.0f, 0.0f, screenWidth, screenHeight, {0.025f, 0.035f, 0.065f, 1.0f});
	for (int index = 0; index < playerCount; ++index) {
		const float cardX = startX + static_cast<float>(index) * (cardWidth + gap);
		const bool selected = index == selectedPlayerIndex_;
		DrawColorSprite(cardBorderSprites_[index].get(), cardX - 5.0f, cardY - 5.0f, cardWidth + 10.0f, cardHeight + 10.0f,
		    selected ? Vector4{1.0f, 0.78f, 0.12f, 1.0f} : Vector4{0.22f, 0.25f, 0.32f, 1.0f});
		DrawColorSprite(cardSprites_[index].get(), cardX, cardY, cardWidth, cardHeight,
		    selected ? Vector4{0.10f, 0.30f, 0.56f, 0.98f} : Vector4{0.07f, 0.11f, 0.20f, 0.98f});
		GameObject* textObject = playerTextObjects_[index].get();
		textObject->GetTransform().translate = {cardX + cardWidth * 0.5f, cardY + cardHeight * 0.5f, 0.0f};
		textObject->GetComponent<TextComponent>()->SetColor(
		    selected ? Vector4{1.0f, 0.93f, 0.58f, 1.0f} : Vector4{0.92f, 0.94f, 1.0f, 1.0f});
		textObject->Draw2D();
	}

	titleTextObject_->GetTransform().translate = {screenWidth * 0.5f, 62.0f, 0.0f};
	instructionTextObject_->GetTransform().translate = {screenWidth * 0.5f, screenHeight - 34.0f, 0.0f};
	titleTextObject_->Draw2D();
	instructionTextObject_->Draw2D();
}

void PlayerSelectScene::Finalize() {
	// シーンが所有するUIをすべて破棄し、次回初期化時に作り直せるようにする。
	backgroundSprite_.reset();
	cardBorderSprites_.clear();
	cardSprites_.clear();
	titleTextObject_.reset();
	instructionTextObject_.reset();
	playerTextObjects_.clear();
	playerTypeNames_.clear();
	BaseScene::Finalize();
}
