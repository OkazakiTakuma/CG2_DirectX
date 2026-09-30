#include "FontManager.h"

#include <Windows.h>
#include <filesystem>

FontManager* FontManager::GetInstance() {
	static FontManager instance;
	return &instance;
}

bool FontManager::LoadFromDirectory(const std::wstring& directory) {
	// AddFontResourceExWのFR_PRIVATEを使い、Windows全体ではなくゲームプロセス内だけに登録する。
	// そのため、ユーザーの環境へフォントをインストールする必要がない。
	namespace fs = std::filesystem;
	if (!fs::exists(directory)) {
		return false;
	}

	bool loadedAny = false;
	for (const fs::directory_entry& entry : fs::directory_iterator(directory)) {
		if (!entry.is_regular_file()) {
			continue;
		}
		const std::wstring extension = entry.path().extension().wstring();
		// TrueType、OpenType、TrueType Collectionを配布フォントとして扱う。
		if (extension != L".ttf" && extension != L".otf" && extension != L".ttc") {
			continue;
		}

		const std::wstring path = entry.path().wstring();
		if (AddFontResourceExW(path.c_str(), FR_PRIVATE, nullptr) > 0) {
			loadedFontPaths_.push_back(path);
			loadedAny = true;
		}
	}
	return loadedAny;
}

void FontManager::Finalize() {
	// 登録時と同じパスを使って解除し、次回起動や他のアプリへ状態を残さない。
	for (const std::wstring& path : loadedFontPaths_) {
		RemoveFontResourceExW(path.c_str(), FR_PRIVATE, nullptr);
	}
	loadedFontPaths_.clear();
}
