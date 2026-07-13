#pragma once
#include <string>
#include <Windows.h>
namespace StringUtility {
/// <summary>
/// ConvertString の処理を行います。
/// </summary>
/// <param name="wstr">wstr に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
std::string ConvertString(const std::wstring& wstr);
/// <summary>
/// ConvertString の処理を行います。
/// </summary>
/// <param name="str">str に使用する値を指定します。</param>
/// <returns>処理結果を返します。</returns>
std::wstring ConvertString(const std::string& str);
};
