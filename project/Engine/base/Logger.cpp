#include "Logger.h"
namespace Logger {

/// <summary>Visual Studioのデバッグ出力へメッセージを送ります。</summary>
void Log(const std::string& message) { OutputDebugStringA(message.c_str()); }

} // namespace Logger
