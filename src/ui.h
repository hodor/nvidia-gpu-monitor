#pragma once

#include "gpu_monitor.h"
#include "imgui.h"
#include <vector>
#include <string>
#include <map>
#include <functional>
#include <algorithm>

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
    // Per-card positions (cardEndY[i] = Y position at bottom of card i)
    std::vector<float> cardStartY;
    std::vector<float> cardEndY;
};

// Per-card UI state
struct GpuCardState {
    bool settingsExpanded = false;
    bool focusNickname = false;  // Focus nickname input on next frame
    bool collapsed = false;      // Minimize GPU card to single line
};

// History buffer for sparklines (circular buffer)
struct GpuMetricHistory {
    // At 60fps: 36000 samples = 600 seconds (10 minutes) of history
    static constexpr size_t HISTORY_SIZE = 36000;
    static constexpr int DEFAULT_DISPLAY_SECONDS = 60;
    static constexpr int MIN_DISPLAY_SECONDS = 5;
    static constexpr int MAX_DISPLAY_SECONDS = 600;

    // All metrics as fractions (0-1)
    float vramHistory[HISTORY_SIZE] = {};       // VRAM usage fraction
    float gpuUtilHistory[HISTORY_SIZE] = {};    // GPU utilization fraction
    float powerHistory[HISTORY_SIZE] = {};      // Power as fraction of limit
    float coreClockHistory[HISTORY_SIZE] = {};  // Core clock as fraction of max
    float memClockHistory[HISTORY_SIZE] = {};   // Mem clock as fraction of max
    float tempHistory[HISTORY_SIZE] = {};       // Temperature as fraction (0-100C mapped to 0-1)
    float fanHistory[HISTORY_SIZE] = {};        // Fan speed as fraction (0-100%)

    size_t writeIndex = 0;
    size_t sampleCount = 0;  // How many samples we've collected (up to HISTORY_SIZE)
    int displaySeconds = DEFAULT_DISPLAY_SECONDS;  // How many seconds to show (zoom level)
    float totalElapsedTime = 0.0f;  // Total time since first sample (for calculating sample rate)

    // Add a sample every frame - tracks elapsed time to calculate actual sample rate
    void addSample(float deltaTime, float vram, float gpuUtil, float power,
                   float coreClock, float memClock, float temp, float fan) {
        totalElapsedTime += deltaTime;

        vramHistory[writeIndex] = vram;
        gpuUtilHistory[writeIndex] = gpuUtil;
        powerHistory[writeIndex] = power;
        coreClockHistory[writeIndex] = coreClock;
        memClockHistory[writeIndex] = memClock;
        tempHistory[writeIndex] = temp;
        fanHistory[writeIndex] = fan;
        writeIndex = (writeIndex + 1) % HISTORY_SIZE;
        if (sampleCount < HISTORY_SIZE) sampleCount++;
    }

    // Get effective samples per second based on actual timing
    float getSamplesPerSecond() const {
        if (totalElapsedTime < 0.1f || sampleCount < 2) return 60.0f;  // Default assumption
        return static_cast<float>(sampleCount) / totalElapsedTime;
    }

    // Get ordered data for a single metric
    // Uses actual sample rate to calculate how many samples represent displaySeconds
    void getOrderedMetric(const float* source, float* out, size_t& outCount) const {
        float sps = getSamplesPerSecond();
        size_t samplesForTimeWindow = static_cast<size_t>(displaySeconds * sps);
        // Cap to available samples
        outCount = std::min({samplesForTimeWindow, sampleCount, HISTORY_SIZE});
        if (outCount == 0) return;

        size_t startIdx;
        if (sampleCount <= outCount) {
            startIdx = (sampleCount < HISTORY_SIZE) ? 0 : writeIndex;
            outCount = sampleCount;  // Can only show what we have
        } else {
            startIdx = (writeIndex + HISTORY_SIZE - outCount) % HISTORY_SIZE;
        }

        for (size_t i = 0; i < outCount; i++) {
            size_t idx = (startIdx + i) % HISTORY_SIZE;
            out[i] = source[idx];
        }
    }

    void resetZoom() {
        displaySeconds = DEFAULT_DISPLAY_SECONDS;
    }
};

// Sparkline zoom drag state
struct SparklineZoomState {
    bool isDragging = false;
    std::string dragGpuUuid;      // Which GPU's sparklines are being adjusted
    float dragStartX = 0.0f;      // Mouse X position at drag start
    int originalDisplaySeconds = 0;  // Display seconds at drag start
    int previewDisplaySeconds = 0;   // Preview value during drag
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

    // Metric history for sparklines (keyed by UUID)
    std::map<std::string, GpuMetricHistory> m_metricHistory;

    // Sparkline zoom state
    SparklineZoomState m_zoomState;

    // Render compact metrics section with sparklines (grid layout)
    void renderCompactMetrics(const GpuStats& stats);

    // Get health status for a single metric: 0=green, 1=yellow, 2=red
    int getMetricHealth(float frac);      // For Power/Core/Mem (70%/90% thresholds)
    int getVramHealth(float frac);        // For VRAM/GPU (40%/70% thresholds)

    // 4-level health indicators: 0=green, 1=yellow, 2=orange, 3=red
    int getTempHealth(unsigned int tempC);
    int getFanHealth(unsigned int fanPercent);
    ImVec4 getHealthColor4(int health);

    // Check if UI is in a modal state (should block other interactions)
    bool isModalActive() const {
        return m_dragState.isDragging || m_zoomState.isDragging;
    }

    // Render darkening overlay for modal states
    void renderModalOverlay();
};
