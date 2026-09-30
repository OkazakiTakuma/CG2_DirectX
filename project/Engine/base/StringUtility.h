#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <Windows.h>
namespace StringUtility {
std::string ConvertString(const std::wstring& wstr);
std::wstring ConvertString(const std::string& str);

// エンジン内のstd::stringパスはUTF-8とし、Windows APIとの境界だけUTF-16へ変換します。
std::filesystem::path Utf8ToPath(std::string_view utf8Path);
std::string PathToUtf8(const std::filesystem::path& path);
}
