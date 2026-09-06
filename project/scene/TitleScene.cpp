#include "TitleScene.h"
#include "SceneManager.h"

#include <algorithm>
#include <cmath>
#include <Xinput.h>

namespace {
std::unique_ptr<Sprite> CreateColorSprite() {
	// 1枚の白テクスチャに色を乗算し、背景・パネル・ボタンへ使い回す。
	auto sprite = std::make_unique<Sprite>();
	sprite->Initialize("Resources/human/white.png");
	return sprite;
}

std::unique_ptr<GameObject> CreateTextObject(const std::string& text, float fontSize, const Vector4& color) {
	// タイトルUIの文字はすべて中央基準で配置できるよう設定を統一する。
	auto object = std::make_unique<GameObject>();
	TextComponent* textComponent = object->AddComponent<TextComponent>();
	textComponent->SetText(text);
	textComponent->SetFontSize(fontSize);
	textComponent->SetAnchor(TextComponent::Anchor::Center);
	textComponent->SetColor(color);
	return object;
}

void DrawColorSprite(Sprite* sprite, float x, float y, float width, float height, const Vector4& color) {
	// 単色矩形として描画するため、位置・大きさ・色を描画直前にまとめて反映する。
	EulerTransform transform = sprite->GetTransform();
	transform.translate = {x, y, 0.0f};
	sprite->SetTransform(transform);
	sprite->SetSize({width, height});
	sprite->SetColor(color);
	sprite->Update();
	sprite->Draw();
}
}

void TitleScene::Initialize() {
	// タイトルへ戻るたび、ゲーム開始が選ばれた初期状態から表示する。
	selectedMenuIndex_ = 0;
	isResetConfirmationOpen_ = false;
	pulseTime_ = 0.0f;
	CreateUi();
}

void TitleScene::CreateUi() {
	// 矩形用Spriteは個別に持たせ、同一フレーム内で異なる位置と色を描画できるようにする。
	backgroundSprite_ = CreateColorSprite();
	accentSprite_ = CreateColorSprite();
	menuPanelSprite_ = CreateColorSprite();
	startButtonSprite_ = CreateColorSprite();
	newGameButtonSprite_ = CreateColorSprite();
	shopButtonSprite_ = CreateColorSprite();
	confirmationBackdropSprite_ = CreateColorSprite();
	confirmationPanelSprite_ = CreateColorSprite();

	// 表示文字と基本色はここへ集約し、Draw2Dでは配置と選択状態だけを更新する。
	titleTextObject_ = CreateTextObject("FLAME SURVIVOR", 68.0f, {0.95f, 0.98f, 1.0f, 1.0f});
	subtitleTextObject_ = CreateTextObject("BURN THROUGH THE NIGHT", 19.0f, {0.30f, 0.86f, 1.0f, 1.0f});
	startTextObject_ = CreateTextObject("GAME START", 30.0f, {1.0f, 1.0f, 1.0f, 1.0f});
	newGameTextObject_ = CreateTextObject("初めから", 30.0f, {0.82f, 0.87f, 0.94f, 1.0f});
	shopTextObject_ = CreateTextObject("SHOP", 30.0f, {0.82f, 0.87f, 0.94f, 1.0f});
	moneyTextObject_ = CreateTextObject("", 21.0f, {1.0f, 0.80f, 0.22f, 1.0f});
	instructionTextObject_ = CreateTextObject(
	    "上下キー・Pad上下: 選択    Enter・Pad A: 決定    Space: ゲーム開始    S・Pad X: ショップ", 18.0f,
	    {0.68f, 0.75f, 0.86f, 1.0f});
	versionTextObject_ = CreateTextObject("PRESS ENTER / A BUTTON", 17.0f, {0.45f, 0.63f, 0.76f, 1.0f});
	confirmationTextObject_ = CreateTextObject("本当にゲームデータを初期化しますか？", 27.0f, {1.0f, 0.92f, 0.82f, 1.0f});
	confirmationInstructionTextObject_ = CreateTextObject(
	    "Enter・Pad A: 初期化する    Esc・Pad B: キャンセル", 18.0f, {0.76f, 0.84f, 0.94f, 1.0f});
}


/// <summary>
/// 毎フレームの状態更新を行います。
/// </summary>
void TitleScene::Update() {
	Input* input = Input::GetInstance();
	// 選択中ボタンの明滅に使用する時間。タイトル演出なので固定刻みで十分とする。
	pulseTime_ += 1.0f / 60.0f;
	if (isResetConfirmationOpen_) {
		// 確認中は通常メニューのショートカットを無効にし、誤操作による別画面への遷移を防ぐ。
		if (input->TriggerKey(DIK_ESCAPE) || input->TriggerGamepadButton(XINPUT_GAMEPAD_B)) {
			isResetConfirmationOpen_ = false;
			return;
		}
		if (input->TriggerKey(DIK_RETURN) || input->TriggerGamepadButton(XINPUT_GAMEPAD_A)) {
			sceneManager->ResetGameProgress();
			sceneManager->ChangeScene("PLAYER_SELECT");
		}
		return;
	}
	// 十字キーと左スティックのどちらも、キーボードの上下キーと同じ選択操作として扱う。
	if (input->TriggerKey(DIK_UP) || input->TriggerGamepadUp()) {
		selectedMenuIndex_ = (selectedMenuIndex_ + 2) % 3;
	}
	if (input->TriggerKey(DIK_DOWN) || input->TriggerGamepadDown()) {
		selectedMenuIndex_ = (selectedMenuIndex_ + 1) % 3;
	}
	if (input->TriggerKey(DIK_SPACE) || input->TriggerGamepadButton(XINPUT_GAMEPAD_START)) {
		// Space / START は選択位置に関係なくゲーム開始へ進むショートカットとする。
		sceneManager->ChangeScene("PLAYER_SELECT");
		return;
	}
	if (input->TriggerKey(DIK_S) || input->TriggerGamepadButton(XINPUT_GAMEPAD_X)) {
		// メニュー選択を経由しないショップ用ショートカット。
		sceneManager->ChangeScene("SHOP");
		return;
	}
	if (input->TriggerKey(DIK_RETURN) || input->TriggerGamepadButton(XINPUT_GAMEPAD_A)) {
		// Enter / Pad A は通常の決定操作として、現在選択中の項目を実行する。
		if (selectedMenuIndex_ == 1) {
			// 「初めから」は即時消去せず、もう一度明示的な決定操作を求める。
			isResetConfirmationOpen_ = true;
		} else {
			sceneManager->ChangeScene(selectedMenuIndex_ == 0 ? "PLAYER_SELECT" : "SHOP");
		}
	}
}

void TitleScene::Draw2D() {
	DirectXCommon* dxCommon = SpriteCommon::GetInstance()->GetDxCommon();
	if (!sceneManager || !dxCommon || !backgroundSprite_) return;
	const float width = static_cast<float>(dxCommon->GetRenderWidth());
	const float height = static_cast<float>(dxCommon->GetRenderHeight());
	// 実際の描画解像度を基準に、中央寄せを保ちながら横幅を画面内へ収める。
	const float panelWidth = (std::min)(440.0f, width - 80.0f);
	const float panelHeight = 232.0f;
	const float panelX = (width - panelWidth) * 0.5f;
	const float panelY = height * 0.40f;
	const float buttonX = panelX + 22.0f;
	const float buttonWidth = panelWidth - 44.0f;
	const float buttonHeight = 58.0f;
	const float buttonInterval = 70.0f;
	// 透明度を周期的に変化させ、選択中の項目を柔らかく明滅させる。
	const float pulse = 0.78f + std::sin(pulseTime_ * 4.0f) * 0.12f;

	SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
	// 背景、上下のアクセントライン、メニューパネルの順に奥から描画する。
	DrawColorSprite(backgroundSprite_.get(), 0.0f, 0.0f, width, height, {0.015f, 0.028f, 0.060f, 1.0f});
	DrawColorSprite(accentSprite_.get(), 0.0f, 0.0f, width, 7.0f, {0.10f, 0.76f, 1.0f, 1.0f});
	DrawColorSprite(accentSprite_.get(), 0.0f, height - 7.0f, width, 7.0f, {1.0f, 0.45f, 0.08f, 1.0f});
	DrawColorSprite(menuPanelSprite_.get(), panelX, panelY, panelWidth, panelHeight, {0.035f, 0.075f, 0.14f, 0.94f});

	const Vector4 selectedColor = {0.06f, 0.48f, 0.68f, pulse};
	const Vector4 idleColor = {0.06f, 0.11f, 0.19f, 1.0f};
	// 選択項目だけ背景色を明るくして、現在のフォーカスを視覚化する。
	DrawColorSprite(startButtonSprite_.get(), buttonX, panelY + 22.0f, buttonWidth, buttonHeight,
	    selectedMenuIndex_ == 0 ? selectedColor : idleColor);
	DrawColorSprite(newGameButtonSprite_.get(), buttonX, panelY + 22.0f + buttonInterval, buttonWidth, buttonHeight,
	    selectedMenuIndex_ == 1 ? selectedColor : idleColor);
	DrawColorSprite(shopButtonSprite_.get(), buttonX, panelY + 22.0f + buttonInterval * 2.0f, buttonWidth, buttonHeight,
	    selectedMenuIndex_ == 2 ? selectedColor : idleColor);

	// 各テキストは中央アンカーのため、画面またはパネルの中心座標を指定する。
	titleTextObject_->GetTransform().translate = {width * 0.5f, height * 0.22f, 0.0f};
	subtitleTextObject_->GetTransform().translate = {width * 0.5f, height * 0.22f + 62.0f, 0.0f};
	startTextObject_->GetTransform().translate = {width * 0.5f, panelY + 51.0f, 0.0f};
	newGameTextObject_->GetTransform().translate = {width * 0.5f, panelY + 51.0f + buttonInterval, 0.0f};
	shopTextObject_->GetTransform().translate = {width * 0.5f, panelY + 51.0f + buttonInterval * 2.0f, 0.0f};
	// ボタン背景だけでなく文字色も変え、選択状態を読み取りやすくする。
	startTextObject_->GetComponent<TextComponent>()->SetColor(
	    selectedMenuIndex_ == 0 ? Vector4{0.78f, 0.98f, 1.0f, 1.0f} : Vector4{0.70f, 0.76f, 0.84f, 1.0f});
	newGameTextObject_->GetComponent<TextComponent>()->SetColor(
	    selectedMenuIndex_ == 1 ? Vector4{0.78f, 0.98f, 1.0f, 1.0f} : Vector4{0.70f, 0.76f, 0.84f, 1.0f});
	shopTextObject_->GetComponent<TextComponent>()->SetColor(
	    selectedMenuIndex_ == 2 ? Vector4{0.78f, 0.98f, 1.0f, 1.0f} : Vector4{0.70f, 0.76f, 0.84f, 1.0f});

	// ショップへ入る前に現在の共有所持金を確認できるよう、毎フレーム最新値を表示する。
	moneyTextObject_->GetComponent<TextComponent>()->SetText("所持金: " + std::to_string(sceneManager->GetMoney()) + " G");
	moneyTextObject_->GetTransform().translate = {width * 0.5f, panelY + panelHeight + 32.0f, 0.0f};
	versionTextObject_->GetTransform().translate = {width * 0.5f, height - 74.0f, 0.0f};
	instructionTextObject_->GetTransform().translate = {width * 0.5f, height - 38.0f, 0.0f};

	// 矩形UIの手前へ文字を重ねる。
	titleTextObject_->Draw2D();
	subtitleTextObject_->Draw2D();
	startTextObject_->Draw2D();
	newGameTextObject_->Draw2D();
	shopTextObject_->Draw2D();
	moneyTextObject_->Draw2D();
	versionTextObject_->Draw2D();
	instructionTextObject_->Draw2D();

	if (isResetConfirmationOpen_) {
		// メニューを暗く覆った上に確認パネルを重ね、現在の入力対象を明確にする。
		const float confirmationWidth = (std::min)(560.0f, width - 48.0f);
		const float confirmationHeight = 160.0f;
		const float confirmationX = (width - confirmationWidth) * 0.5f;
		const float confirmationY = (height - confirmationHeight) * 0.5f;
		SpriteCommon::GetInstance()->SetDraw(kBlendModeNormal);
		DrawColorSprite(confirmationBackdropSprite_.get(), 0.0f, 0.0f, width, height, {0.0f, 0.0f, 0.0f, 0.72f});
		DrawColorSprite(confirmationPanelSprite_.get(), confirmationX, confirmationY,
		    confirmationWidth, confirmationHeight, {0.055f, 0.085f, 0.14f, 1.0f});
		confirmationTextObject_->GetTransform().translate = {width * 0.5f, confirmationY + 56.0f, 0.0f};
		confirmationInstructionTextObject_->GetTransform().translate = {width * 0.5f, confirmationY + 112.0f, 0.0f};
		confirmationTextObject_->Draw2D();
		confirmationInstructionTextObject_->Draw2D();
	}
}

void TitleScene::Draw3D() {}

/// <summary>
/// 確保したリソースを解放し、終了処理を行います。
/// </summary>
void TitleScene::Finalize() {
	// シーンが生成したUIリソースをすべて解放してから基底クラスの終了処理を呼ぶ。
	backgroundSprite_.reset();
	accentSprite_.reset();
	menuPanelSprite_.reset();
	startButtonSprite_.reset();
	newGameButtonSprite_.reset();
	shopButtonSprite_.reset();
	confirmationBackdropSprite_.reset();
	confirmationPanelSprite_.reset();
	titleTextObject_.reset();
	subtitleTextObject_.reset();
	startTextObject_.reset();
	newGameTextObject_.reset();
	shopTextObject_.reset();
	moneyTextObject_.reset();
	instructionTextObject_.reset();
	versionTextObject_.reset();
	confirmationTextObject_.reset();
	confirmationInstructionTextObject_.reset();
	BaseScene::Finalize();
}

void TitleScene::ImGuiUpdate() {}

