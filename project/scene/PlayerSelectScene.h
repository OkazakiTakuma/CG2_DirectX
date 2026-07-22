#pragma once

#include "BaseScene.h"

/// <summary>プレイヤータイプの能力を表示し、ゲーム開始時のキャラクターを選択します。</summary>
class PlayerSelectScene : public BaseScene {
public:
	void Initialize() override;
	void Update() override;
	void Draw2D() override;
	void Draw3D() override {}
	void Finalize() override;
	void SetSceneManager(SceneManager* manager) override { sceneManager_ = manager; }

private:
	/// <summary>選択カード、説明文、操作案内などのUIを構築します。</summary>
	void CreateUi();
	/// <summary>能力値からカードに表示する説明文を生成します。</summary>
	std::string MakePlayerDescription(const PlayerStats& stats) const;

	/// <summary>決定後のシーン遷移を依頼するための参照です。</summary>
	SceneManager* sceneManager_ = nullptr;
	/// <summary>選択候補となるプレイヤー設定名の一覧です。</summary>
	std::vector<std::string> playerTypeNames_;
	/// <summary>現在フォーカスしている候補のインデックスです。</summary>
	int selectedPlayerIndex_ = 0;
	// 背景、カード、テキストを所有し、シーン終了時にまとめて破棄する。
	std::unique_ptr<Sprite> backgroundSprite_;
	std::vector<std::unique_ptr<Sprite>> cardBorderSprites_;
	std::vector<std::unique_ptr<Sprite>> cardSprites_;
	std::unique_ptr<GameObject> titleTextObject_;
	std::unique_ptr<GameObject> instructionTextObject_;
	std::vector<std::unique_ptr<GameObject>> playerTextObjects_;
};
