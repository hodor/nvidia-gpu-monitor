#pragma once

#include <string>
#include <cstddef>

namespace Platform {

// Get the directory path for storing application settings
// Windows: %USERPROFILE%\.gpu_monitor
// Linux: $HOME/.config/gpu_monitor
std::string getSettingsDirectory();

// Copy text to the system clipboard
void copyToClipboard(const std::string& text);

// Open a terminal/console with an environment variable set
// envName: name of the environment variable (e.g., "CUDA_VISIBLE_DEVICES")
// envValue: value to set
// label: descriptive label shown to user
// workingDir: optional working directory
void openTerminalWithEnv(const std::string& envName, const std::string& envValue,
                         const std::string& label, const std::string& workingDir = "");

// Execute a command with optional environment variable and working directory
void executeCommand(const std::string& cmd, const std::string& workingDir = "",
                    const std::string& envName = "", const std::string& envValue = "");

// Terminate a process by PID
// Returns true on success
bool killProcess(unsigned int pid);

// Get the name of a process by PID
// Returns "Unknown" if the process cannot be found
std::string getProcessName(unsigned int pid);

// Open a folder browser dialog
// Returns the selected path, or empty string if cancelled
std::string browseForFolder(const std::string& title = "");

// Safe string copy (cross-platform replacement for strncpy_s)
void safeCopy(char* dest, size_t destSize, const char* src);

} // namespace Platform
