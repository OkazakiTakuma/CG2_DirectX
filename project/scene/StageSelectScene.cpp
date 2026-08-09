#include "StageSelectScene.h"

#include "SceneManager.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <Xinput.h>

namespace {
constexpr int kCardsPerRow = 3;
constexpr int kRowsPerPage = 2;
constexpr int kCardsPerPage = kCardsPerRow * kRowsPerPage;

std::unique_ptr<Sprite> CreateColorSprite() {
	auto sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/human/white.png");
	return sprite;
}

std::unique_ptr<GameObject> CreateTextObject(const std::string& text, float fontSize) {
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

void StageSelectScene::Initialize() {
	selectedStageIndex_ = 0;
	if (sceneManager_) {
		sceneManager_->RefreshGameplayStagePatterns();
		stageIds_ = sceneManager_->GetGameplayStageIds();
		const auto found = std::find(stageIds_.begin(), stageIds_.end(), sceneManager_->GetSelectedGameplayStageId());
		if (found != stageIds_.end()) {
			selectedStageIndex_ = static_cast<int>(std::distance(stageIds_.begin(), found));
		}
	}
	CreateUi();
}

void StageSelectScene::CreateUi() {
	backgroundSprite_ = CreateColorSprite();
	cardBorderSprites_.clear();
	cardSprites_.clear();
	stageTextObjects_.clear();
	titleTextObject_ = CreateTextObject("ステージを選択", 48.0f);
	const std::string selectedPlayer = sceneManager_ ? sceneManager_->GetSelectedPlayerTypeName() : "Unknown";
	selectedPlayerTextObject_ = CreateTextObject("選択中のプレイヤー: " + selectedPlayer, 22.0f);
	instructionTextObject_ = CreateTextObject(
	    "A / D・左右キー・Pad左右: 選択    Space / Enter・Pad A: 決定    Q・Pad B: プレイヤー選択へ戻る", 19.0f);
	pageTextObject_ = CreateTextObject("", 18.0f);
	for (const std::string& stageId : stageIds_) {
		cardBorderSprites_.push_back(CreateColorSprite());
		cardSprites_.push_back(CreateColorSprite());
		stageTextObjects_.push_back(CreateTextObject(MakeStageDescription(stageId), 25.0f));
	}
}

std::string StageSelectScene::MakeStageDescription(const std::string& stageId) const {
	std::string displayName = stageId;
	std::transform(displayName.begin(), displayName.end(), displayName.begin(), [](unsigned char character) {
		return static_cast<char>(std::toupper(character));
	});
	if (stageId == "default") {
		return displayName + "\n\n標準ステージ\nバランスのよい基本配置";
	}
	if (stageId == "wide") {
		return displayName + "\n\n広域ステージ\n広いフィールドで戦う配置";
	}
	if (stageId == "rush") {
		return displayName + "\n\nラッシュステージ\n敵が高密度で出現する配置";
	}
	if (stageId == "stage2") {
		return displayName + "\n\n第二ステージ\n強化された敵と自爆敵が出現";
	}
	return displayName + "\n\nカスタムステージ\n配置パターン: " + stageId;
}

void StageSelectScene::Update() {
	if (stageIds_.empty() || !sceneManager_) {
		return;
	}
	Input* input = Input::GetInstance();
	const int stageCount = static_cast<int>(stageIds_.size());
	if (input->TriggerKey(DIK_A) || input->TriggerKey(DIK_LEFT) || input->TriggerGamepadLeft()) {
		selectedStageIndex_ = (selectedStageIndex_ + stageCount - 1) % stageCount;
	}
	if (input->TriggerKey(DIK_D) || input->TriggerKey(DIK_RIGHT) || input->TriggerGamepadRight()) {
		selectedStageIndex_ = (selectedStageIndex_ + 1) % stageCount;
	}
	if (input->TriggerKey(DIK_SPACE) || input->TriggerKey(DIK_RETURN) || input->TriggerGamepadButton(XINPUT_GAMEPAD_A)) {
		sceneManager_->SetSelectedGameplayStageId(stageIds_[selectedStageIndex_]);
		sceneManager_->ChangeScene("GAMEPLAY");
		return;
	}
	if (input->TriggerKey(DIK_Q) || input->TriggerGamepadButton(XINPUT_GAMEPAD_B)) {
		sceneManager_->ChangeScene("PLAYER_SELECT");
	}
}

void StageSelectScene::Draw2D() {
	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (!dxCommon || !backgroundSprite_) {
		return;
	}
	const float screenWidth = static_cast<float>(dxCommon->GetRenderWidth());
	const float screenHeight = static_cast<float>(dxCommon->GetRenderHeight());
	const int stageCount = static_cast<int>(stageIds_.size());
	const int pageCount = (std::max)(1, (stageCount + kCardsPerPage - 1) / kCardsPerPage);
	const int currentPage = selectedStageIndex_ / kCardsPerPage;
	const int pageStart = currentPage * kCardsPerPage;
	const int pageEnd = (std::min)(pageStart + kCardsPerPage, stageCount);
	const float sideMargin = 70.0f;
	const float horizontalGap = 22.0f;
	const float verticalGap = 18.0f;
	const float availableWidth = screenWidth - sideMargin * 2.0f;
	const float cardWidth = (std::min)(300.0f, (availableWidth - horizontalGap * 2.0f) / 3.0f);
	const float cardHeight = (std::min)(190.0f, (screenHeight - 245.0f - verticalGap) / 2.0f);
	const float firstRowY = 145.0f;

	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	DrawColorSprite(backgroundSprite_.get(), 0.0f, 0.0f, screenWidth, screenHeight, {0.02f, 0.04f, 0.075f, 1.0f});
	for (int row = 0; row < kRowsPerPage; ++row) {
		const int rowStart = pageStart + row * kCardsPerRow;
		const int rowEnd = (std::min)(rowStart + kCardsPerRow, pageEnd);
		const int rowCount = rowEnd - rowStart;
		if (rowCount <= 0) {
			continue;
		}
		const float rowWidth = cardWidth * rowCount + horizontalGap * (rowCount - 1);
		const float startX = (screenWidth - rowWidth) * 0.5f;
		const float cardY = firstRowY + row * (cardHeight + verticalGap);
		for (int column = 0; column < rowCount; ++column) {
			const int index = rowStart + column;
			const float cardX = startX + column * (cardWidth + horizontalGap);
			const bool selected = index == selectedStageIndex_;
			DrawColorSprite(cardBorderSprites_[index].get(), cardX - 5.0f, cardY - 5.0f, cardWidth + 10.0f, cardHeight + 10.0f,
			    selected ? Vector4{0.24f, 0.92f, 1.0f, 1.0f} : Vector4{0.20f, 0.25f, 0.34f, 1.0f});
			DrawColorSprite(cardSprites_[index].get(), cardX, cardY, cardWidth, cardHeight,
			    selected ? Vector4{0.08f, 0.36f, 0.50f, 0.98f} : Vector4{0.06f, 0.12f, 0.22f, 0.98f});
			GameObject* textObject = stageTextObjects_[index].get();
			textObject->GetTransform().translate = {cardX + cardWidth * 0.5f, cardY + cardHeight * 0.5f, 0.0f};
			textObject->GetComponent<TextComponent>()->SetColor(
			    selected ? Vector4{0.72f, 0.97f, 1.0f, 1.0f} : Vector4{0.90f, 0.93f, 1.0f, 1.0f});
			textObject->Draw2D();
		}
	}

	titleTextObject_->GetTransform().translate = {screenWidth * 0.5f, 48.0f, 0.0f};
	selectedPlayerTextObject_->GetTransform().translate = {screenWidth * 0.5f, 100.0f, 0.0f};
	instructionTextObject_->GetTransform().translate = {screenWidth * 0.5f, screenHeight - 32.0f, 0.0f};
	std::ostringstream pageStream;
	pageStream << (currentPage + 1) << " / " << pageCount;
	pageTextObject_->GetComponent<TextComponent>()->SetText(pageStream.str());
	pageTextObject_->GetTransform().translate = {screenWidth * 0.5f, screenHeight - 65.0f, 0.0f};
	titleTextObject_->Draw2D();
	selectedPlayerTextObject_->Draw2D();
	instructionTextObject_->Draw2D();
	pageTextObject_->Draw2D();
}

void StageSelectScene::Finalize() {
	backgroundSprite_.reset();
	cardBorderSprites_.clear();
	cardSprites_.clear();
	titleTextObject_.reset();
	selectedPlayerTextObject_.reset();
	instructionTextObject_.reset();
	pageTextObject_.reset();
	stageTextObjects_.clear();
	stageIds_.clear();
	BaseScene::Finalize();
}
