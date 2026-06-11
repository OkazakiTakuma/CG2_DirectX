#include "ParticleEmitter.h"
#include "ParticleManager.h" // Managerの定義が必要

// コンストラクタ
// 引数で受け取った値をメンバ変数に書き込む
ParticleEmitter::ParticleEmitter() : groupName_(""), count_(0), frequency_(0.0f), frequencyTimer_(0.0f), textureFilePath_("") {
	transform_.scale = { 1.0f, 1.0f, 1.0f };
	transform_.rotate = { 0.0f, 0.0f, 0.0f };
	transform_.translate = { 0.0f, 0.0f, 0.0f };

	// ─── ★追加：すべてのパラメータに安全な初期値を入れておく ───
	emitParam_.scale = { 1.0f, 1.0f, 1.0f };
	emitParam_.baseVelocity = { 0.0f, 0.0f, 0.0f };
	emitParam_.randomVelocityRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.randomPositionRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.lifeTime = 1.0f; // 寿命が0だと一瞬で消えてしまいます

	emitParam_.baseRotate = { 0.0f, 0.0f, 0.0f };
	emitParam_.isRandomRotate = false;
	emitParam_.randomRotateRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
	emitParam_.count = 1;
	emitParam_.randomScaleRange = { 0.0f, 0.0f, 0.0f };
	emitParam_.isBillboard = true;
}
void ParticleEmitter::Update(float deltaTime) {
	// 頻度が0以下の場合は処理しない（0除算防止）
	if (frequency_ <= 0.0f) {
		return;
	}

	// 1. 時刻を進める
	frequencyTimer_ += deltaTime;

	// 発生間隔（閾値）を計算 (例: frequency=10 なら 0.1秒)
	float interval = 1.0f / frequency_;

	// 2. 発生頻度より大きいなら発生（余剰時間も考慮してループ処理）
	// ラグなどで deltaTime が長く、一度に複数回分の時間が経過した場合でも
	// 適切な回数 Emit を呼ぶために while を使用します。
	while (frequencyTimer_ >= interval) {
		// 発生処理
		Emit();
		// 3. 余計に過ぎた時間込みで頻度計算をする
		// タイマーを0にするのではなく、閾値分だけ引くことでズレを防ぐ
		frequencyTimer_ -= interval;
	}
}

void ParticleEmitter::Emit() {
	// エミッタの規定値（現在の座標）に従ってパーティクルマネージャーを呼び出す
	// 引数：グループ名, 発生座標, 発生数
	ParticleManager::GetInstance()->Emit(groupName_, transform_.translate, emitParam_.count, emitParam_);
}

//---------------------------------------------------------
// JSONへの保存（1つのファイルに追記・更新する形）
//---------------------------------------------------------
void ParticleEmitter::SaveToJson(const std::string& filePath) {

	// 1. 保存先のフォルダが存在するか確認し、なければ作成する
	std::filesystem::path pathObj(filePath);
	std::filesystem::path dir = pathObj.parent_path(); // "Resources/Data" の部分を取得

	// フォルダパスが指定されていて、かつ存在しない場合は作成する
	if (!dir.empty() && !std::filesystem::exists(dir)) {
		std::filesystem::create_directories(dir);
	}

	nlohmann::json root;

	// 2. 既存のファイルを読み込んで、今までのデータを root に入れる（ファイルがあれば）
	std::ifstream inFile(filePath);
	if (inFile.is_open()) {
		try {
			inFile >> root;
		} catch (...) {
			// 中身が空だったり壊れている場合は何もしない（新規作成として扱う）
		}
		inFile.close();
	}

	// 3. このエミッター専用のデータを作る
	nlohmann::json emitterNode;
	emitterNode["count"] = count_;
	emitterNode["frequency"] = frequency_;
	emitterNode["textureFilePath"] = textureFilePath_;
	emitterNode["emitParam"]["scale"] = {emitParam_.scale.x, emitParam_.scale.y, emitParam_.scale.z};
	emitterNode["emitParam"]["baseVelocity"] = {emitParam_.baseVelocity.x, emitParam_.baseVelocity.y, emitParam_.baseVelocity.z};
	emitterNode["emitParam"]["randomVelocityRange"] = {emitParam_.randomVelocityRange.x, emitParam_.randomVelocityRange.y, emitParam_.randomVelocityRange.z};
	emitterNode["emitParam"]["randomPositionRange"] = {emitParam_.randomPositionRange.x, emitParam_.randomPositionRange.y, emitParam_.randomPositionRange.z};
	emitterNode["emitParam"]["lifeTime"] = emitParam_.lifeTime;
	emitterNode["emitParam"]["baseRotate"] = { emitParam_.baseRotate.x, emitParam_.baseRotate.y, emitParam_.baseRotate.z };
	emitterNode["emitParam"]["isRandomRotate"] = emitParam_.isRandomRotate;
	emitterNode["emitParam"]["randomRotateRange"] = { emitParam_.randomRotateRange.x, emitParam_.randomRotateRange.y, emitParam_.randomRotateRange.z };
	emitterNode["emitParam"]["randomScaleRange"] = { emitParam_.randomScaleRange.x, emitParam_.randomScaleRange.y, emitParam_.randomScaleRange.z };
	emitterNode["emitParam"]["count"] = emitParam_.count;
	emitterNode["emitParam"]["color"] = { emitParam_.color.x, emitParam_.color.y, emitParam_.color.z, emitParam_.color.w };	// 全体データ (root) の中に、自分の名前 (groupName_) でデータを格納する
	emitterNode["emitParam"]["isBillboard"] = emitParam_.isBillboard;
	root[groupName_] = emitterNode;

	// 4. ファイルに上書き保存する（ファイル自体はここで自動的に作成されます）
	std::ofstream outFile(filePath);
	if (outFile.is_open()) {
		outFile << root.dump(4); // インデント幅4で綺麗に保存
		outFile.close();
	}
}

//---------------------------------------------------------
// JSONからの読み込み（自分の名前のデータだけを探す）
//---------------------------------------------------------
void ParticleEmitter::LoadFromJson(const std::string& filePath) {
	std::ifstream file(filePath);
	if (!file.is_open()) {
		return; // ファイルがない場合は終了
	}

	nlohmann::json root;
	try {
		file >> root;
	} catch (...) {
		return; // 読み込みエラー時は終了
	}
	file.close();

	// ファイルの中に、自分の名前(groupName_)のデータが存在するかチェック
	if (!root.contains(groupName_)) {
		return; // 自分のデータがなければ何もしない
	}

	// 自分のデータだけを切り出す
	auto& emitterNode = root[groupName_];
	
	// 読み込んだ値をメンバ変数に反映
	if (emitterNode.contains("count"))
		count_ = emitterNode["count"];
	if (emitterNode.contains("frequency"))
		frequency_ = emitterNode["frequency"];

	if (emitterNode.contains("emitParam")) {
		auto& param = emitterNode["emitParam"];

		if (param.contains("scale")) {
			emitParam_.scale = {param["scale"][0], param["scale"][1], param["scale"][2]};
		}
		if (param.contains("baseVelocity")) {
			emitParam_.baseVelocity = {param["baseVelocity"][0], param["baseVelocity"][1], param["baseVelocity"][2]};
		}
		if (param.contains("randomVelocityRange")) {
			emitParam_.randomVelocityRange = {param["randomVelocityRange"][0], param["randomVelocityRange"][1], param["randomVelocityRange"][2]};
		}
		if (param.contains("randomPositionRange")) {
			emitParam_.randomPositionRange = {param["randomPositionRange"][0], param["randomPositionRange"][1], param["randomPositionRange"][2]};
		}
		if (param.contains("lifeTime")) {
			emitParam_.lifeTime = param["lifeTime"];
		}
		if (param.contains("baseRotate")) {
			emitParam_.baseRotate = { param["baseRotate"][0], param["baseRotate"][1], param["baseRotate"][2] };
		}
		if (param.contains("isRandomRotate")) {
			emitParam_.isRandomRotate = param["isRandomRotate"];
		}
		if (param.contains("randomRotateRange")) {
			emitParam_.randomRotateRange = { param["randomRotateRange"][0], param["randomRotateRange"][1], param["randomRotateRange"][2] };
		}
		if (param.contains("color")) {
			emitParam_.color = { param["color"][0], param["color"][1], param["color"][2], param["color"][3] };
		}
		if (param.contains("randomScaleRange")) {
			emitParam_.randomScaleRange = { param["randomScaleRange"][0], param["randomScaleRange"][1], param["randomScaleRange"][2] };
		}
		if (param.contains("count")) {
			emitParam_.count = param["count"];
		}
		if (param.contains("isBillboard")) {
			emitParam_.isBillboard = param["isBillboard"];
		}
	}
}
// 2. SetTexture の実装を追加
void ParticleEmitter::SetTexture(const std::string& textureFilePath) {
	textureFilePath_ = textureFilePath;
	// マネージャーに登録済みのグループ名があれば、テクスチャを更新する
	if (!groupName_.empty()) {
		ParticleManager::GetInstance()->SetGroupTexture(groupName_, textureFilePath_);
	}
}