#pragma once
#include <string>
#include <Windows.h>
namespace StringUtility {
std::string ConvertString(const std::wstring& wstr);
std::wstring ConvertString(const std::string& str);
};
