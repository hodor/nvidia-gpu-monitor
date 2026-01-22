#include "platform.h"

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <array>

namespace Platform {

std::string getSettingsDirectory() {
    const char* home = std::getenv("HOME");
    if (home) {
        return std::string(home) + "/.config/gpu_monitor";
    }
    return ".";
}

void copyToClipboard(const std::string& text) {
    // Try xclip first, fall back to xsel
    FILE* pipe = popen("xclip -selection clipboard 2>/dev/null", "w");
    if (!pipe) {
        pipe = popen("xsel --clipboard --input 2>/dev/null", "w");
    }
    if (pipe) {
        fwrite(text.c_str(), 1, text.size(), pipe);
        pclose(pipe);
    }
}

int openTerminalWithEnv(const std::string& envName, const std::string& envValue,
                        const std::string& label, const std::string& workingDir) {
    // Try common terminal emulators in order of preference
    std::string cmd;
    std::string cdCmd = workingDir.empty() ? "" : "cd '" + workingDir + "' && ";

    // Build the shell command to run
    std::string shellCmd = cdCmd +
        "export " + envName + "='" + envValue + "' && "
        "echo '' && "
        "echo '  " + envName + " = " + envValue + "  (" + label + ")' && "
        "echo '' && "
        "exec $SHELL";

    // Try gnome-terminal first
    cmd = "gnome-terminal -- bash -c \"" + shellCmd + "\" 2>/dev/null";
    int result = system(cmd.c_str());
    if (result == 0) return 0;

    // Try konsole
    cmd = "konsole -e bash -c \"" + shellCmd + "\" 2>/dev/null";
    result = system(cmd.c_str());
    if (result == 0) return 0;

    // Try xfce4-terminal
    cmd = "xfce4-terminal -e \"bash -c \\\"" + shellCmd + "\\\"\" 2>/dev/null";
    result = system(cmd.c_str());
    if (result == 0) return 0;

    // Fall back to xterm
    cmd = "xterm -e bash -c \"" + shellCmd + "\" 2>/dev/null &";
    return system(cmd.c_str());
}

int executeCommand(const std::string& cmd, const std::string& workingDir,
                   const std::string& envName, const std::string& envValue) {
    std::string fullCmd;

    if (!workingDir.empty()) {
        fullCmd = "cd '" + workingDir + "' && ";
    }
    if (!envName.empty()) {
        fullCmd += "export " + envName + "='" + envValue + "' && ";
    }
    fullCmd += cmd + " &";

    return system(fullCmd.c_str());
}

bool killProcess(unsigned int pid) {
    return kill(static_cast<pid_t>(pid), SIGTERM) == 0;
}

std::string getProcessName(unsigned int pid) {
    std::string commPath = "/proc/" + std::to_string(pid) + "/comm";
    std::ifstream file(commPath);
    if (file.is_open()) {
        std::string name;
        std::getline(file, name);
        if (!name.empty()) {
            return name;
        }
    }
    return "Unknown";
}

std::string browseForFolder(const std::string& title) {
    // Try zenity first (GTK)
    std::string cmd = "zenity --file-selection --directory";
    if (!title.empty()) {
        cmd += " --title='" + title + "'";
    }
    cmd += " 2>/dev/null";

    std::array<char, 512> buffer;
    std::string result;

    FILE* pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        int status = pclose(pipe);
        if (status == 0 && !result.empty()) {
            // Remove trailing newline
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            return result;
        }
    }

    // Try kdialog as fallback (KDE)
    cmd = "kdialog --getexistingdirectory";
    if (!title.empty()) {
        cmd += " --title '" + title + "'";
    }
    cmd += " 2>/dev/null";

    pipe = popen(cmd.c_str(), "r");
    if (pipe) {
        result.clear();
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
            result += buffer.data();
        }
        int status = pclose(pipe);
        if (status == 0 && !result.empty()) {
            if (!result.empty() && result.back() == '\n') {
                result.pop_back();
            }
            return result;
        }
    }

    return "";
}

void safeCopy(char* dest, size_t destSize, const char* src) {
    if (dest && destSize > 0 && src) {
        strncpy(dest, src, destSize - 1);
        dest[destSize - 1] = '\0';
    }
}

} // namespace Platform
