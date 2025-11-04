#pragma once

#include <filesystem>
#include <string>
#include <string_view>
#include <sstream>

namespace Logging
{
    enum class Level
    {
        Debug = 0,
        Info,
        Warning,
        Error
    };

    void Initialize(const std::filesystem::path& moduleDirectory);
    void Shutdown();

    void SetLevel(Level level);
    Level SetLevelFromString(std::wstring_view levelName);
    Level GetLevel();

    std::wstring_view ToString(Level level);
    std::filesystem::path GetLogFilePath();

    void Log(Level level, std::wstring_view message);
    void Log(Level level, const std::wstring& message);

    bool IsInitialized();
}

#define LOG_MESSAGE(level, message)                                                  \
    do                                                                               \
    {                                                                                \
        std::wostringstream _ualLoggingStream;                                       \
        _ualLoggingStream << message;                                                \
        Logging::Log(level, _ualLoggingStream.str());                                \
    } while (0)

#define LOG_DEBUG(message) LOG_MESSAGE(Logging::Level::Debug, message)
#define LOG_INFO(message) LOG_MESSAGE(Logging::Level::Info, message)
#define LOG_WARNING(message) LOG_MESSAGE(Logging::Level::Warning, message)
#define LOG_ERROR(message) LOG_MESSAGE(Logging::Level::Error, message)
