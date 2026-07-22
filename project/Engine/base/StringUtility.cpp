#include "StringUtility.h"

namespace StringUtility{
std::string ConvertString(const std::wstring& wstr) {
	if (wstr.empty())
		return {};

	int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), result.data(), sizeNeeded, nullptr, nullptr);
	return result;
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty())
		return {};
	int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), nullptr, 0);
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, str.data(), (int)str.size(), result.data(), sizeNeeded);
	return result;
}
}
