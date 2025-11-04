#include "logger.h"

#include <windows.h>

#include <algorithm>
#include <cwchar>
#include <cwctype>
#include <mutex>
#include <vector>

namespace Logging
{
    namespace
    {
        std::mutex gMutex;
        HANDLE gLogFile = INVALID_HANDLE_VALUE;
        bool gInitialized = false;
        Level gCurrentLevel = Level::Info;
        std::filesystem::path gLogPath;

        std::wstring_view LevelToStringInternal(Level level)
        {
            switch (level)
            {
            case Level::Debug:
                return L"DEBUG";
            case Level::Info:
                return L"INFO";
            case Level::Warning:
                return L"WARN";
            case Level::Error:
                return L"ERROR";
            default:
                return L"INFO";
            }
        }

        std::wstring BuildPrefix(Level level)
        {
            SYSTEMTIME st{};
            GetLocalTime(&st);

            wchar_t buffer[64] = {};
            swprintf_s(buffer, L"%04u-%02u-%02u %02u:%02u:%02u.%03u", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

            std::wstring prefix;
            prefix.reserve(48);
            prefix.append(L"[");
            prefix.append(buffer);
            prefix.append(L"] [");
            prefix.append(LevelToStringInternal(level));
            prefix.append(L"] ");
            return prefix;
        }

        void WriteLineUnlocked(std::wstring_view line)
        {
            if (gLogFile == INVALID_HANDLE_VALUE)
                return;

            if (line.empty())
                return;

            int requiredSize = WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), nullptr, 0, nullptr, nullptr);
            if (requiredSize <= 0)
                return;

            std::vector<char> utf8(static_cast<size_t>(requiredSize));
            if (WideCharToMultiByte(CP_UTF8, 0, line.data(), static_cast<int>(line.size()), utf8.data(), requiredSize, nullptr, nullptr) <= 0)
                return;

            DWORD bytesToWrite = static_cast<DWORD>(utf8.size());
            DWORD bytesWritten = 0;
            WriteFile(gLogFile, utf8.data(), bytesToWrite, &bytesWritten, nullptr);
        }

        Level LevelFromString(std::wstring_view levelName)
        {
            std::wstring lower(levelName.begin(), levelName.end());
            std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

            if (lower == L"debug")
                return Level::Debug;
            if (lower == L"warn" || lower == L"warning")
                return Level::Warning;
            if (lower == L"error" || lower == L"err")
                return Level::Error;
            return Level::Info;
        }
    }

    void Initialize(const std::filesystem::path& moduleDirectory)
    {
        std::lock_guard lock(gMutex);
        if (gInitialized)
            return;

        if (!moduleDirectory.empty())
        {
            gLogPath = moduleDirectory / L"Ultimate-ASI-Loader.log";
        }
        else
        {
            wchar_t modulePathBuffer[MAX_PATH] = {};
            if (GetModuleFileNameW(nullptr, modulePathBuffer, MAX_PATH) > 0)
            {
                std::filesystem::path modulePath(modulePathBuffer);
                gLogPath = modulePath.parent_path() / L"Ultimate-ASI-Loader.log";
            }
        }

        gLogFile = CreateFileW(gLogPath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (gLogFile != INVALID_HANDLE_VALUE)
        {
            gInitialized = true;

            const std::wstring header = L"=== Ultimate ASI Loader logging started ===\r\n";
            WriteLineUnlocked(header);
        }
    }

    void Shutdown()
    {
        std::lock_guard lock(gMutex);
        if (gLogFile != INVALID_HANDLE_VALUE)
        {
            const std::wstring footer = L"=== Ultimate ASI Loader logging finished ===\r\n";
            WriteLineUnlocked(footer);
            CloseHandle(gLogFile);
            gLogFile = INVALID_HANDLE_VALUE;
        }
        gInitialized = false;
    }

    void SetLevel(Level level)
    {
        std::lock_guard lock(gMutex);
        gCurrentLevel = level;
    }

    Level SetLevelFromString(std::wstring_view levelName)
    {
        Level parsedLevel = LevelFromString(levelName);
        SetLevel(parsedLevel);
        return parsedLevel;
    }

    Level GetLevel()
    {
        std::lock_guard lock(gMutex);
        return gCurrentLevel;
    }

    std::wstring_view ToString(Level level)
    {
        return LevelToStringInternal(level);
    }

    std::filesystem::path GetLogFilePath()
    {
        std::lock_guard lock(gMutex);
        return gLogPath;
    }

    void Log(Level level, std::wstring_view message)
    {
        Level currentLevel = GetLevel();
        if (static_cast<int>(level) < static_cast<int>(currentLevel))
            return;

        std::wstring line = BuildPrefix(level);
        line.append(message);
        line.append(L"\r\n");

        OutputDebugStringW(line.c_str());

        std::lock_guard lock(gMutex);
        WriteLineUnlocked(line);
    }

    void Log(Level level, const std::wstring& message)
    {
        Log(level, std::wstring_view(message));
    }

    bool IsInitialized()
    {
        std::lock_guard lock(gMutex);
        return gInitialized;
    }
}
