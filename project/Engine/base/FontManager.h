#pragma once

#include <string>
#include <vector>

/// <summary>Resources/Fonts 内のフォントをプロセス専用で登録します。</summary>
class FontManager {
public:
	static FontManager* GetInstance();
	bool LoadFromDirectory(const std::wstring& directory);
	void Finalize();

private:
	FontManager() = default;
	FontManager(const FontManager&) = delete;
	FontManager& operator=(const FontManager&) = delete;
	std::vector<std::wstring> loadedFontPaths_;
};
