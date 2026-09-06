#pragma once

#include "BaseScene.h"
/// <summary>ゲーム開始とショップへの入口を表示するタイトルシーンです。</summary>
class TitleScene: public BaseScene {
public:
	/// <summary>
	/// 必要なリソースを準備し、オブジェクトを初期化します。
	/// </summary>
	void Initialize() override;
	/// <summary>
	/// 毎フレームの状態更新を行います。
	/// </summary>
	void Update() override;
	/// <summary>
	/// 2D 要素の描画処理を行います。
	/// </summary>
	void Draw2D() override;
	/// <summary>
	/// 3D 要素の描画処理を行います。
	/// </summary>
	void Draw3D() override;
	/// <summary>
	/// 確保したリソースを解放し、終了処理を行います。
	/// </summary>
	void Finalize() override;
	/// <summary>シーン遷移と共有所持金の参照に使うSceneManagerを設定します。</summary>
	void SetSceneManager(SceneManager* manager) override { sceneManager = manager; }

private:
	/// <summary>タイトル画面用の2D UIを生成します。</summary>
	void CreateUi();
	/// <summary>
	/// ImGui によるデバッグ用 UI の表示と編集処理を行います。
	/// </summary>
	void ImGuiUpdate();
	/// <summary>SceneManagerが所有するインスタンスへの非所有参照です。</summary>
	SceneManager* sceneManager = nullptr;
	/// <summary>画面全体を塗る背景です。</summary>
	std::unique_ptr<Sprite> backgroundSprite_;
	/// <summary>画面上下のアクセントライン描画に使い回す単色スプライトです。</summary>
	std::unique_ptr<Sprite> accentSprite_;
	/// <summary>開始・初めから・ショップボタンをまとめる中央パネルです。</summary>
	std::unique_ptr<Sprite> menuPanelSprite_;
	/// <summary>ゲーム開始項目の選択状態を示す背景です。</summary>
	std::unique_ptr<Sprite> startButtonSprite_;
	/// <summary>初めから項目の選択状態を示す背景です。</summary>
	std::unique_ptr<Sprite> newGameButtonSprite_;
	/// <summary>ショップ項目の選択状態を示す背景です。</summary>
	std::unique_ptr<Sprite> shopButtonSprite_;
	/// <summary>初期化確認中に背面のメニューを暗くするスプライトです。</summary>
	std::unique_ptr<Sprite> confirmationBackdropSprite_;
	/// <summary>初期化確認メッセージを囲むパネルです。</summary>
	std::unique_ptr<Sprite> confirmationPanelSprite_;
	/// <summary>タイトルロゴ文字です。</summary>
	std::unique_ptr<GameObject> titleTextObject_;
	/// <summary>タイトル直下の副題です。</summary>
	std::unique_ptr<GameObject> subtitleTextObject_;
	/// <summary>ゲーム開始メニューの文字です。</summary>
	std::unique_ptr<GameObject> startTextObject_;
	/// <summary>進行をリセットして始めるメニューの文字です。</summary>
	std::unique_ptr<GameObject> newGameTextObject_;
	/// <summary>ショップメニューの文字です。</summary>
	std::unique_ptr<GameObject> shopTextObject_;
	/// <summary>SceneManagerが保持する現在の所持金表示です。</summary>
	std::unique_ptr<GameObject> moneyTextObject_;
	/// <summary>キーボードとゲームパッドの操作説明です。</summary>
	std::unique_ptr<GameObject> instructionTextObject_;
	/// <summary>画面下部で決定操作を促す文字です。</summary>
	std::unique_ptr<GameObject> versionTextObject_;
	/// <summary>ゲームデータ初期化前の確認メッセージです。</summary>
	std::unique_ptr<GameObject> confirmationTextObject_;
	/// <summary>初期化の実行・キャンセル操作を案内する文字です。</summary>
	std::unique_ptr<GameObject> confirmationInstructionTextObject_;
	/// <summary>0がゲーム開始、1が初めから、2がショップを表す現在の選択位置です。</summary>
	int selectedMenuIndex_ = 0;
	/// <summary>ゲームデータ初期化の確認画面を表示中かどうかです。</summary>
	bool isResetConfirmationOpen_ = false;
	/// <summary>選択中ボタンの明滅に使う累積秒数です。</summary>
	float pulseTime_ = 0.0f;
};
