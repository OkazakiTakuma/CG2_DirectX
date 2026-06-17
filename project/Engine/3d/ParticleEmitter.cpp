#include "ParticleEmitter.h"
#include "ParticleManager.h" // Managerの定義が必要
#include <fstream>
#include <iomanip> // std::setw用（JSONを綺麗に改行して保存するため）

// コンストラクタ
// 引数で受け取った値をメンバ変数に書き込む
ParticleEmitter::ParticleEmitter() : groupName_(""), count_(0), frequency_(0.0f), frequencyTimer_(0.0f), textureFilePath_(""), isActive_(true), blendMode_(kBlendModeNormal) {
    transform_.scale = { 1.0f, 1.0f, 1.0f };
    transform_.rotate = { 0.0f, 0.0f, 0.0f };
    transform_.translate = { 0.0f, 0.0f, 0.0f };

    // ─── すべてのパラメータに安全な初期値を入れておく ───
    emitParam_.scale = { 1.0f, 1.0f, 1.0f };
    emitParam_.endScale = { 0.0f, 0.0f, 0.0f };

    emitParam_.baseVelocity = { 0.0f, 0.0f, 0.0f };
    emitParam_.randomVelocityRange = { 0.0f, 0.0f, 0.0f };
    emitParam_.acceleration = { 0.0f, 0.0f, 0.0f };

    emitParam_.randomPositionRange = { 0.0f, 0.0f, 0.0f };
    emitParam_.lifeTime = 1.0f; // 寿命が0だと一瞬で消えてしまいます

    emitParam_.baseRotate = { 0.0f, 0.0f, 0.0f };
    emitParam_.isRandomRotate = false;
    emitParam_.randomRotateRange = { 0.0f, 0.0f, 0.0f };

    emitParam_.color = { 1.0f, 1.0f, 1.0f, 1.0f };
    emitParam_.endColor = { 1.0f, 1.0f, 1.0f, 0.0f };

    emitParam_.randomScaleRange = { 0.0f, 0.0f, 0.0f };
    emitParam_.count = 1;
    emitParam_.isBillboard = true;
}

void ParticleEmitter::Update(float deltaTime) {
    // アクティブではない（オフに設定されている）場合は更新・放出を行わない
    if (!isActive_) {
        return;
    }

    // 頻度に応じた自動放出処理
    frequencyTimer_ += deltaTime;
    if (frequency_ > 0.0f && frequencyTimer_ >= frequency_) {
        Emit();
        // タイマーをリセット（超過分を引くことで正確な間隔を保つ）
        frequencyTimer_ -= frequency_;
    }
}

void ParticleEmitter::Emit() {
    // Managerを通して実際にパーティクルを発生させる
    ParticleManager::GetInstance()->Emit(groupName_, transform_.translate, count_, emitParam_);
}

void ParticleEmitter::SetTexture(const std::string& textureFilePath) {
    textureFilePath_ = textureFilePath;
    ParticleManager::GetInstance()->SetGroupTexture(groupName_, textureFilePath_);
}

// =========================================================
// JSONへの保存処理
// =========================================================
void ParticleEmitter::SaveToJson(const std::string& filePath) {
    nlohmann::json root;

    // 既存のファイルがあれば一度読み込んでから上書きする（他のグループのデータを消さないため）
    std::ifstream ifs(filePath);
    if (ifs.is_open()) {
        ifs >> root;
        ifs.close();
    }

    // このグループ（例："Fire"など）のデータを作成
    nlohmann::json groupJson;
    groupJson["count"] = count_;
    groupJson["frequency"] = frequency_;
    groupJson["textureFilePath"] = textureFilePath_;
    groupJson["isActive"] = isActive_;
    groupJson["blendMode"] = static_cast<int>(blendMode_); // ★追加：ブレンドモードを保存

    // emitParam（構造体の中身）を格納
    nlohmann::json param;
    param["scale"] = { emitParam_.scale.x, emitParam_.scale.y, emitParam_.scale.z };
    param["endScale"] = { emitParam_.endScale.x, emitParam_.endScale.y, emitParam_.endScale.z };

    param["baseVelocity"] = { emitParam_.baseVelocity.x, emitParam_.baseVelocity.y, emitParam_.baseVelocity.z };
    param["randomVelocityRange"] = { emitParam_.randomVelocityRange.x, emitParam_.randomVelocityRange.y, emitParam_.randomVelocityRange.z };
    param["acceleration"] = { emitParam_.acceleration.x, emitParam_.acceleration.y, emitParam_.acceleration.z };

    param["randomPositionRange"] = { emitParam_.randomPositionRange.x, emitParam_.randomPositionRange.y, emitParam_.randomPositionRange.z };
    param["lifeTime"] = emitParam_.lifeTime;

    param["baseRotate"] = { emitParam_.baseRotate.x, emitParam_.baseRotate.y, emitParam_.baseRotate.z };
    param["isRandomRotate"] = emitParam_.isRandomRotate;
    param["randomRotateRange"] = { emitParam_.randomRotateRange.x, emitParam_.randomRotateRange.y, emitParam_.randomRotateRange.z };

    param["color"] = { emitParam_.color.x, emitParam_.color.y, emitParam_.color.z, emitParam_.color.w };
    param["endColor"] = { emitParam_.endColor.x, emitParam_.endColor.y, emitParam_.endColor.z, emitParam_.endColor.w };

    param["randomScaleRange"] = { emitParam_.randomScaleRange.x, emitParam_.randomScaleRange.y, emitParam_.randomScaleRange.z };
    param["count"] = emitParam_.count;
    param["isBillboard"] = emitParam_.isBillboard;

    groupJson["emitParam"] = param;

    // ルートにグループ名でセット
    root[groupName_] = groupJson;

    // ファイルに書き込み（インデント幅4で綺麗にフォーマット）
    std::ofstream ofs(filePath);
    if (ofs.is_open()) {
        ofs << std::setw(4) << root << std::endl;
        ofs.close();
    }
}

// =========================================================
// JSONからの読み込み処理
// =========================================================
void ParticleEmitter::LoadFromJson(const std::string& filePath) {
    std::ifstream ifs(filePath);
    if (!ifs.is_open()) {
        return; // ファイルがなければ何もしない
    }

    nlohmann::json root;
    ifs >> root;
    ifs.close();

    // 自分のグループ名がJSONの中に存在するかチェック
    if (!root.contains(groupName_)) {
        return;
    }

    auto& groupJson = root[groupName_];

    if (groupJson.contains("count")) { count_ = groupJson["count"]; }
    if (groupJson.contains("frequency")) { frequency_ = groupJson["frequency"]; }
    if (groupJson.contains("textureFilePath")) { textureFilePath_ = groupJson["textureFilePath"]; }
    if (groupJson.contains("isActive")) { isActive_ = groupJson["isActive"]; }

    // ★追加：ブレンドモードの読み込み
    if (groupJson.contains("blendMode")) {
        blendMode_ = static_cast<BlendMode>(groupJson["blendMode"].get<int>());
    }

    // emitParamの中身を読み込む
    if (groupJson.contains("emitParam")) {
        auto& param = groupJson["emitParam"];

        if (param.contains("scale")) {
            emitParam_.scale = { param["scale"][0], param["scale"][1], param["scale"][2] };
        }
        if (param.contains("endScale")) {
            emitParam_.endScale = { param["endScale"][0], param["endScale"][1], param["endScale"][2] };
        }
        if (param.contains("baseVelocity")) {
            emitParam_.baseVelocity = { param["baseVelocity"][0], param["baseVelocity"][1], param["baseVelocity"][2] };
        }
        if (param.contains("randomVelocityRange")) {
            emitParam_.randomVelocityRange = { param["randomVelocityRange"][0], param["randomVelocityRange"][1], param["randomVelocityRange"][2] };
        }
        if (param.contains("acceleration")) {
            emitParam_.acceleration = { param["acceleration"][0], param["acceleration"][1], param["acceleration"][2] };
        }
        if (param.contains("randomPositionRange")) {
            emitParam_.randomPositionRange = { param["randomPositionRange"][0], param["randomPositionRange"][1], param["randomPositionRange"][2] };
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
        if (param.contains("endColor")) {
            emitParam_.endColor = { param["endColor"][0], param["endColor"][1], param["endColor"][2], param["endColor"][3] };
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