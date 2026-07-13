#include "StringUtility.h"

namespace StringUtility{
/// <summary>
/// ConvertString の処理を行います。
/// </summary>
/// <param name="wstr">wstr に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
std::string ConvertString(const std::wstring& wstr) {
	if (wstr.empty())
		return {};

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

/// <summary>
/// ConvertString の処理を行います。
/// </summary>
/// <param name="str">str に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
std::wstring ConvertString(const std::string& str) {
	if (str.empty())
		return {};
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), sizeNeeded);
	return result;
}
}
