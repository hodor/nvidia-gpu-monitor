#pragma once

#include "gpu_monitor.h"
#include <vector>
#include <string>
#include <map>
#include <functional>

// Confirmation dialog state
struct ConfirmDialog {
    bool isOpen = false;
    std::string title;
    std::string message;
    std::string command;
    bool isDangerous = false;
};

// Per-GPU configuration (keyed by UUID)
struct GpuConfig {
    std::string uuid;           // GPU UUID (unique identifier)
    std::string nickname;       // User-defined nickname (e.g., "TOP", "Compute 1")
    int displayOrder = -1;      // User-defined display order (-1 = use default bus ID order)
};

// Quick launch preset
struct QuickLaunchPreset {
    std::string name;
    std::string command;
    std::string workingDir;
    // GPU selection stored as comma-separated UUIDs (empty = all GPUs)
    std::string selectedGpuUuids;
};

// Global settings
struct Settings {
    std::vector<QuickLaunchPreset> presets;  // Quick launch presets
    std::vector<GpuConfig> gpuConfigs;       // GPU configurations
};

// Drag-and-drop state
struct GpuDragState {
    bool isDragging = false;
    std::string draggedUuid;
    int dragSourceIndex = -1;
    int currentHoverIndex = -1;
    float firstCardY = 0.0f;
    float cardHeight = 0.0f;
};

// Per-card UI state
struct GpuCardState {
    bool settingsExpanded = false;
    bool focusNickname = false;  // Focus nickname input on next frame
};

class GpuMonitorUI {
public:
    GpuMonitorUI();
    void render(const std::vector<GpuStats>& gpuStats, const SystemInfo& sysInfo);

private:
    void loadSettings();
    void saveSettings();
    std::string getSettingsPath();

    // GPU configuration helpers
    GpuConfig* getGpuConfig(const std::string& uuid);
    GpuConfig* getOrCreateGpuConfig(const std::string& uuid);
    std::string getGpuDisplayName(const GpuStats& stats);
    std::vector<GpuStats> sortGpusByUserOrder(const std::vector<GpuStats>& gpuStats);

    void renderSystemHealth(const SystemInfo& sysInfo);
    void renderQuickLaunch(const std::vector<GpuStats>& gpuStats);
    void renderGpuCard(const GpuStats& stats, const std::vector<GpuStats>& allStats, int index);
    void renderBadge(const char* text, bool isTCC);
    void renderProcessesSection(const GpuStats& stats);
    void renderCommandsSection(const GpuStats& stats, const std::vector<GpuStats>& allStats);
    void renderConfirmDialog();

    // Drag-drop functions
    void renderDragHandle(const GpuStats& stats, const std::string& displayName, int index);
    void renderDropIndicator(int targetIndex);
    void commitReorder(int sourceIndex, int targetIndex, const std::vector<GpuStats>& sortedStats);
    GpuCardState& getCardState(const std::string& uuid);
    void killProcess(unsigned int pid);
    std::string buildGpuSelectionString(const QuickLaunchPreset& preset, const std::vector<GpuStats>& gpuStats);
    bool isGpuSelectedInPreset(const QuickLaunchPreset& preset, const std::string& uuid);
    void toggleGpuInPreset(QuickLaunchPreset& preset, const std::string& uuid);

    // Clipboard helper
    void copyToClipboard(const std::string& text);

    // Show toast notification for copied text
    void showCopiedToast(const std::string& label);

    // Build CUDA_VISIBLE_DEVICES string excluding certain indices
    std::string buildExcludeDevices(const std::vector<GpuStats>& allStats, unsigned int excludeIndex);

    // Build NVLink pair string (MIDDLE + BOTTOM)
    std::string buildNvlinkPair(const std::vector<GpuStats>& allStats);

    // Open PowerShell with CUDA_VISIBLE_DEVICES set
    void openTerminalWithGpu(const std::string& cudaDevices, const std::string& label);

    // Confirmation dialog
    ConfirmDialog m_confirmDialog;

    // Settings
    Settings m_settings;

    // Toast state
    float m_toastTimer = 0.0f;
    std::string m_toastMessage;

    // Drag-and-drop state
    GpuDragState m_dragState;

    // Per-card UI state (keyed by UUID)
    std::map<std::string, GpuCardState> m_cardStates;
};
