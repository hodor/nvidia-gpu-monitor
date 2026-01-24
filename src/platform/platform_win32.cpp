#include "platform.h"

#define NOMINMAX
#include <Windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <memory>
#include <cstring>

namespace Platform {

std::string getSettingsDirectory() {
    char* userProfile = nullptr;
    size_t len = 0;
    _dupenv_s(&userProfile, &len, "USERPROFILE");

    std::unique_ptr<char, decltype(&free)> profileGuard(userProfile, free);

    std::string path;
    if (userProfile) {
        path = std::string(userProfile) + "\\.gpu_monitor";
    } else {
        path = ".";
    }
    return path;
}

void copyToClipboard(const std::string& text) {
    if (!OpenClipboard(nullptr)) return;

    EmptyClipboard();

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, text.size() + 1);
    if (hGlob) {
        char* pGlob = static_cast<char*>(GlobalLock(hGlob));
        if (pGlob) {
            memcpy(pGlob, text.c_str(), text.size() + 1);
            GlobalUnlock(hGlob);
            SetClipboardData(CF_TEXT, hGlob);
        }
    }

    CloseClipboard();
}

int openTerminalWithEnv(const std::string& envName, const std::string& envValue,
                        const std::string& label, const std::string& workingDir) {
    // Build PowerShell command that sets env var and shows a message
    std::string psCommand =
        "$env:" + envName + "='" + envValue + "'; "
        "Write-Host ''; "
        "Write-Host '  " + envName + " = " + envValue + "  (" + label + ")' -ForegroundColor Green; "
        "Write-Host ''";

    // Launch PowerShell with -NoExit to keep it open
    std::string args = "-NoExit -Command \"" + psCommand + "\"";

    const char* workDir = workingDir.empty() ? nullptr : workingDir.c_str();
    HINSTANCE result = ShellExecuteA(nullptr, "open", "powershell.exe", args.c_str(), workDir, SW_SHOWNORMAL);
    // ShellExecuteA returns > 32 on success, <= 32 on error
    return (reinterpret_cast<intptr_t>(result) > 32) ? 0 : static_cast<int>(reinterpret_cast<intptr_t>(result));
}

std::string normalizeCommand(const std::string& cmd) {
    std::string result = cmd;
    // Replace \r\n first, then remaining \n, with "; " for PowerShell
    size_t pos = 0;
    while ((pos = result.find("\r\n", pos)) != std::string::npos) {
        result.replace(pos, 2, "; ");
        pos += 2;
    }
    pos = 0;
    while ((pos = result.find('\n', pos)) != std::string::npos) {
        result.replace(pos, 1, "; ");
        pos += 2;
    }
    return result;
}

int executeCommand(const std::string& cmd, const std::string& workingDir,
                   const std::string& envName, const std::string& envValue) {
    std::string psCommand;
    if (!envName.empty()) {
        psCommand = "$env:" + envName + "='" + envValue + "'; ";
    }
    psCommand += normalizeCommand(cmd);

    std::string args = "-NoExit -Command \"" + psCommand + "\"";
    const char* workDir = workingDir.empty() ? nullptr : workingDir.c_str();
    HINSTANCE result = ShellExecuteA(nullptr, "open", "powershell.exe", args.c_str(), workDir, SW_SHOWNORMAL);
    // ShellExecuteA returns > 32 on success, <= 32 on error
    return (reinterpret_cast<intptr_t>(result) > 32) ? 0 : static_cast<int>(reinterpret_cast<intptr_t>(result));
}

bool killProcess(unsigned int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
    if (hProcess != nullptr) {
        BOOL result = TerminateProcess(hProcess, 1);
        CloseHandle(hProcess);
        return result != 0;
    }
    return false;
}

std::string getProcessName(unsigned int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!hProcess) {
        return "Unknown";
    }

    char processName[MAX_PATH] = "";
    DWORD size = MAX_PATH;
    if (QueryFullProcessImageNameA(hProcess, 0, processName, &size)) {
        CloseHandle(hProcess);
        // Extract just the filename
        std::string fullPath(processName);
        size_t pos = fullPath.find_last_of("\\/");
        if (pos != std::string::npos) {
            return fullPath.substr(pos + 1);
        }
        return fullPath;
    }

    CloseHandle(hProcess);
    return "Unknown";
}

std::string browseForFolder(const std::string& title) {
    BROWSEINFOA bi = {};
    bi.lpszTitle = title.empty() ? "Select Folder" : title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;

    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != nullptr) {
        char path[MAX_PATH];
        if (SHGetPathFromIDListA(pidl, path)) {
            CoTaskMemFree(pidl);
            return std::string(path);
        }
        CoTaskMemFree(pidl);
    }
    return "";
}

void safeCopy(char* dest, size_t destSize, const char* src) {
    if (dest && destSize > 0 && src) {
        strncpy_s(dest, destSize, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
}

} // namespace Platform
