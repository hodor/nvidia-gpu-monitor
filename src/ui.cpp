#include "ui.h"
#include "platform/platform.h"
#include "imgui.h"
#include <format>
#include <ranges>
#include <fstream>
#include <filesystem>

namespace fs = std::filesystem;

// Helper for ImGui::InputText with std::string
static bool InputTextString(const char* label, std::string& str, ImGuiInputTextFlags flags = 0) {
    constexpr size_t bufferSize = 512;
    char buffer[bufferSize];
    Platform::safeCopy(buffer, bufferSize, str.c_str());

    bool changed = ImGui::InputText(label, buffer, bufferSize, flags);
    if (changed) {
        str = buffer;
    }
    return changed;
}

GpuMonitorUI::GpuMonitorUI() {
    loadSettings();
}

std::string GpuMonitorUI::getSettingsPath() {
    return Platform::getSettingsDirectory() + "/presets.json";
}

void GpuMonitorUI::loadSettings() {
    std::string path = getSettingsPath();
    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    bool inPresets = false;
    bool inGpuConfigs = false;
    QuickLaunchPreset* currentPreset = nullptr;
    GpuConfig* currentConfig = nullptr;

    while (std::getline(file, line)) {
        // Track which section we're in
        if (line.find("\"presets\"") != std::string::npos) {
            inPresets = true;
            inGpuConfigs = false;
            continue;
        }
        if (line.find("\"gpuConfigs\"") != std::string::npos) {
            inPresets = false;
            inGpuConfigs = true;
            continue;
        }

        // Parse presets section
        if (inPresets) {
            if (line.find("\"preset\"") != std::string::npos) {
                m_settings.presets.emplace_back();
                currentPreset = &m_settings.presets.back();
            } else if (currentPreset) {
                size_t pos;
                if ((pos = line.find("\"name\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 7) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentPreset->name = line.substr(start, end - start);
                    }
                } else if ((pos = line.find("\"command\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 10) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentPreset->command = line.substr(start, end - start);
                    }
                } else if ((pos = line.find("\"workingDir\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 13) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentPreset->workingDir = line.substr(start, end - start);
                    }
                } else if ((pos = line.find("\"selectedGpuUuids\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 19) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentPreset->selectedGpuUuids = line.substr(start, end - start);
                    }
                }
            }
        }

        // Parse GPU configs section
        if (inGpuConfigs) {
            if (line.find("\"gpuConfig\"") != std::string::npos) {
                m_settings.gpuConfigs.emplace_back();
                currentConfig = &m_settings.gpuConfigs.back();
            } else if (currentConfig) {
                size_t pos;
                if ((pos = line.find("\"uuid\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 7) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentConfig->uuid = line.substr(start, end - start);
                    }
                } else if ((pos = line.find("\"nickname\":")) != std::string::npos) {
                    size_t start = line.find("\"", pos + 11) + 1;
                    size_t end = line.find("\"", start);
                    if (start != std::string::npos && end != std::string::npos) {
                        currentConfig->nickname = line.substr(start, end - start);
                    }
                } else if ((pos = line.find("\"displayOrder\":")) != std::string::npos) {
                    size_t start = pos + 15;
                    while (start < line.size() && (line[start] == ' ' || line[start] == ':')) start++;
                    currentConfig->displayOrder = std::atoi(line.c_str() + start);
                }
            }
        }
    }
}

void GpuMonitorUI::saveSettings() {
    std::string path = getSettingsPath();

    // Create directory if needed
    fs::path dirPath = fs::path(path).parent_path();
    if (!fs::exists(dirPath)) {
        fs::create_directories(dirPath);
    }

    std::ofstream file(path);
    if (!file.is_open()) return;

    file << "{\n";

    // Write presets
    file << "  \"presets\": [\n";
    for (size_t i = 0; i < m_settings.presets.size(); i++) {
        const auto& preset = m_settings.presets[i];
        file << "    {\n";
        file << "      \"preset\": " << i << ",\n";
        file << "      \"name\": \"" << preset.name << "\",\n";
        file << "      \"command\": \"" << preset.command << "\",\n";
        file << "      \"workingDir\": \"" << preset.workingDir << "\",\n";
        file << "      \"selectedGpuUuids\": \"" << preset.selectedGpuUuids << "\"\n";
        file << "    }" << (i < m_settings.presets.size() - 1 ? "," : "") << "\n";
    }
    file << "  ],\n";

    // Write GPU configs
    file << "  \"gpuConfigs\": [\n";
    for (size_t i = 0; i < m_settings.gpuConfigs.size(); i++) {
        const auto& config = m_settings.gpuConfigs[i];
        file << "    {\n";
        file << "      \"gpuConfig\": " << i << ",\n";
        file << "      \"uuid\": \"" << config.uuid << "\",\n";
        file << "      \"nickname\": \"" << config.nickname << "\",\n";
        file << "      \"displayOrder\": " << config.displayOrder << "\n";
        file << "    }" << (i < m_settings.gpuConfigs.size() - 1 ? "," : "") << "\n";
    }
    file << "  ]\n";

    file << "}\n";
}

void GpuMonitorUI::copyToClipboard(const std::string& text) {
    Platform::copyToClipboard(text);
}

void GpuMonitorUI::showCopiedToast(const std::string& label) {
    m_toastMessage = "Copied: " + label;
    m_toastTimer = 2.0f;  // Show for 2 seconds
}

GpuConfig* GpuMonitorUI::getGpuConfig(const std::string& uuid) {
    for (auto& config : m_settings.gpuConfigs) {
        if (uuid == config.uuid) {
            return &config;
        }
    }
    return nullptr;
}

GpuConfig* GpuMonitorUI::getOrCreateGpuConfig(const std::string& uuid) {
    GpuConfig* existing = getGpuConfig(uuid);
    if (existing) return existing;

    // Create new config
    m_settings.gpuConfigs.emplace_back();
    auto& config = m_settings.gpuConfigs.back();
    config.uuid = uuid;
    config.displayOrder = -1;
    return &config;
}

std::string GpuMonitorUI::getGpuDisplayName(const GpuStats& stats) {
    GpuConfig* config = getGpuConfig(stats.uuid);
    if (config && !config->nickname.empty()) {
        return config->nickname;
    }
    // Default: use CUDA index
    return "GPU " + std::to_string(stats.cudaIndex);
}

std::vector<GpuStats> GpuMonitorUI::sortGpusByUserOrder(const std::vector<GpuStats>& gpuStats) {
    std::vector<GpuStats> sorted = gpuStats;

    std::ranges::sort(sorted, [this](const GpuStats& a, const GpuStats& b) {
        GpuConfig* configA = getGpuConfig(a.uuid);
        GpuConfig* configB = getGpuConfig(b.uuid);

        int orderA = (configA && configA->displayOrder >= 0) ? configA->displayOrder : 1000;
        int orderB = (configB && configB->displayOrder >= 0) ? configB->displayOrder : 1000;

        // If both have user-defined order, use that
        if (orderA != 1000 || orderB != 1000) {
            if (orderA != orderB) return orderA < orderB;
        }

        // Fall back to bus ID order (default)
        return a.pciBusId < b.pciBusId;
    });

    return sorted;
}

GpuCardState& GpuMonitorUI::getCardState(const std::string& uuid) {
    return m_cardStates[uuid];  // Creates default if not exists
}

void GpuMonitorUI::renderDragHandle(const GpuStats& stats, const std::string& displayName, int index) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // Draw 3 horizontal grip lines (hamburger icon)
    float lineWidth = 14.0f;
    float lineSpacing = 4.0f;
    float startY = pos.y + 3.0f;
    ImU32 gripColor = IM_COL32(150, 150, 150, 255);

    for (int i = 0; i < 3; i++) {
        float y = startY + i * lineSpacing;
        drawList->AddLine(ImVec2(pos.x + 2, y), ImVec2(pos.x + lineWidth, y), gripColor, 2.0f);
    }

    // Invisible button for drag interaction
    ImGui::InvisibleButton(("##drag_" + stats.uuid).c_str(), ImVec2(20, 18));

    // Hover tooltip
    if (ImGui::IsItemHovered()) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Drag to reorder");
        ImGui::EndTooltip();
    }

    // Drag source
    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
        m_dragState.isDragging = true;
        m_dragState.draggedUuid = stats.uuid;
        m_dragState.dragSourceIndex = index;

        ImGui::SetDragDropPayload("GPU_REORDER", stats.uuid.c_str(), stats.uuid.size() + 1);
        ImGui::Text("Moving: %s (cuda:%u)", displayName.c_str(), stats.cudaIndex);
        ImGui::EndDragDropSource();
    }
}

void GpuMonitorUI::renderDropIndicator(int targetIndex) {
    if (!m_dragState.isDragging) return;
    if (m_dragState.currentHoverIndex < 0) return;
    if (m_dragState.currentHoverIndex == m_dragState.dragSourceIndex) return;

    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 windowPos = ImGui::GetWindowPos();
    float windowWidth = ImGui::GetWindowWidth();

    // Calculate Y position for the drop indicator line using actual card positions
    // If moving DOWN (source < target): show line at BOTTOM of target card
    // If moving UP (source > target): show line at TOP of target card
    float indicatorY = 0.0f;
    int hoverIdx = m_dragState.currentHoverIndex;

    if (m_dragState.dragSourceIndex < hoverIdx) {
        // Moving down - show at bottom of target card
        if (hoverIdx < static_cast<int>(m_dragState.cardEndY.size())) {
            indicatorY = m_dragState.cardEndY[hoverIdx];
        }
    } else {
        // Moving up - show at top of target card
        if (hoverIdx < static_cast<int>(m_dragState.cardStartY.size())) {
            indicatorY = m_dragState.cardStartY[hoverIdx];
        }
    }

    if (indicatorY <= 0.0f) return;  // Invalid position

    // Draw horizontal blue line
    ImU32 lineColor = IM_COL32(80, 150, 255, 255);
    drawList->AddLine(
        ImVec2(windowPos.x + 10, indicatorY),
        ImVec2(windowPos.x + windowWidth - 10, indicatorY),
        lineColor, 3.0f
    );

    // Add small triangle indicator
    float triSize = 8.0f;
    drawList->AddTriangleFilled(
        ImVec2(windowPos.x + 5, indicatorY - triSize),
        ImVec2(windowPos.x + 5, indicatorY + triSize),
        ImVec2(windowPos.x + 5 + triSize, indicatorY),
        lineColor
    );
}

void GpuMonitorUI::commitReorder(int sourceIndex, int targetIndex, const std::vector<GpuStats>& sortedStats) {
    if (sourceIndex == targetIndex || sourceIndex < 0 || targetIndex < 0) return;
    if (sourceIndex >= static_cast<int>(sortedStats.size())) return;
    if (targetIndex >= static_cast<int>(sortedStats.size())) return;

    // Ensure all GPUs have explicit display orders
    for (size_t i = 0; i < sortedStats.size(); i++) {
        GpuConfig* config = getOrCreateGpuConfig(sortedStats[i].uuid);
        if (config) config->displayOrder = static_cast<int>(i);
    }

    // Get configs for source and target
    GpuConfig* sourceConfig = getGpuConfig(sortedStats[sourceIndex].uuid);
    GpuConfig* targetConfig = getGpuConfig(sortedStats[targetIndex].uuid);

    if (sourceConfig && targetConfig) {
        // Swap display orders
        std::swap(sourceConfig->displayOrder, targetConfig->displayOrder);
    }

    // Reset drag state
    m_dragState.isDragging = false;
    m_dragState.draggedUuid.clear();
    m_dragState.dragSourceIndex = -1;
    m_dragState.currentHoverIndex = -1;

    saveSettings();
}

std::string GpuMonitorUI::buildExcludeDevices(const std::vector<GpuStats>& allStats, unsigned int excludeIndex) {
    std::string result;
    for (const auto& gpu : allStats) {
        if (gpu.cudaIndex != excludeIndex) {
            if (!result.empty()) result += ",";
            result += std::format("{}", gpu.cudaIndex);
        }
    }
    return result;
}

std::string GpuMonitorUI::buildNvlinkPair(const std::vector<GpuStats>& allStats) {
    // Return all TCC (compute) GPUs - these are typically the NVLink-capable ones
    std::string result;
    for (const auto& gpu : allStats) {
        if (gpu.isTCC) {
            if (!result.empty()) result += ",";
            result += std::format("{}", gpu.cudaIndex);
        }
    }
    return result;
}

void GpuMonitorUI::openTerminalWithGpu(const std::string& cudaDevices, const std::string& label) {
    Platform::openTerminalWithEnv("CUDA_VISIBLE_DEVICES", cudaDevices, label);
    showCopiedToast("Terminal opened");
}


void GpuMonitorUI::killProcess(unsigned int pid) {
    if (Platform::killProcess(pid)) {
        showCopiedToast("Process killed");
    }
}

int GpuMonitorUI::getMetricHealth(float frac) {
    // For Power/Core/Mem - high usage is often expected
    if (frac > 0.90f) return 2;  // Red
    if (frac > 0.70f) return 1;  // Yellow
    return 0;                     // Green
}

int GpuMonitorUI::getVramHealth(float frac) {
    // For VRAM/GPU - more conservative thresholds since running low causes issues
    if (frac > 0.70f) return 2;  // Red
    if (frac > 0.40f) return 1;  // Yellow
    return 0;                     // Green
}

// 4-level health for temperature (0=green, 1=yellow, 2=orange, 3=red)
int GpuMonitorUI::getTempHealth(unsigned int tempC) {
    if (tempC > 80) return 3;  // Red - approaching throttle
    if (tempC > 65) return 2;  // Orange - heavy load
    if (tempC > 50) return 1;  // Yellow - normal load
    return 0;                   // Green - cool/idle
}

// 4-level health for fan speed (0=green, 1=yellow, 2=orange, 3=red)
int GpuMonitorUI::getFanHealth(unsigned int fanPercent) {
    if (fanPercent > 80) return 3;  // Red - max cooling
    if (fanPercent > 60) return 2;  // Orange - working hard
    if (fanPercent > 40) return 1;  // Yellow - moderate
    return 0;                        // Green - quiet/idle
}

// Get color for 4-level health indicator
ImVec4 GpuMonitorUI::getHealthColor4(int health) {
    switch (health) {
        case 0: return ImVec4(0.3f, 0.85f, 0.3f, 1.0f);   // Green
        case 1: return ImVec4(0.95f, 0.85f, 0.2f, 1.0f);  // Yellow
        case 2: return ImVec4(0.95f, 0.55f, 0.2f, 1.0f);  // Orange
        case 3: return ImVec4(0.95f, 0.3f, 0.3f, 1.0f);   // Red
        default: return ImVec4(0.5f, 0.5f, 0.5f, 1.0f);   // Gray
    }
}

void GpuMonitorUI::renderModalOverlay() {
    // Overlay is now handled by BeginDisabled/EndDisabled on individual sections
    // This keeps the active element (sparklines or cards being dragged) fully visible
    // while greying out non-interactive sections
}

void GpuMonitorUI::renderCompactMetrics(const GpuStats& stats) {
    // Calculate fractions
    float powerFrac = stats.powerLimit > 0 ? static_cast<float>(stats.powerDraw) / stats.powerLimit : 0.0f;
    float coreClockFrac = stats.gpuClockMax > 0 ? static_cast<float>(stats.gpuClock) / stats.gpuClockMax : 0.0f;
    float memClockFrac = stats.memClockMax > 0 ? static_cast<float>(stats.memClock) / stats.memClockMax : 0.0f;

    // Get history (already updated by renderGpuCard)
    GpuMetricHistory& history = m_metricHistory[stats.uuid];

    // Get display seconds (current or preview during drag)
    int displaySecs = history.displaySeconds;
    if (m_zoomState.isDragging && m_zoomState.dragGpuUuid == stats.uuid) {
        displaySecs = m_zoomState.previewDisplaySeconds;
    }

    // Temporarily override for data retrieval
    int savedDisplaySecs = history.displaySeconds;
    history.displaySeconds = displaySecs;

    // Get ordered history data for each metric
    float powerData[GpuMetricHistory::HISTORY_SIZE];
    float coreData[GpuMetricHistory::HISTORY_SIZE];
    float memData[GpuMetricHistory::HISTORY_SIZE];
    size_t dataCount = 0;
    history.getOrderedMetric(history.powerHistory, powerData, dataCount);
    history.getOrderedMetric(history.coreClockHistory, coreData, dataCount);
    history.getOrderedMetric(history.memClockHistory, memData, dataCount);

    history.displaySeconds = savedDisplaySecs;  // Restore

    // Health colors
    ImVec4 healthColors[] = {
        ImVec4(0.3f, 0.85f, 0.3f, 1.0f),   // Green
        ImVec4(0.95f, 0.75f, 0.2f, 1.0f),  // Yellow
        ImVec4(0.95f, 0.3f, 0.3f, 1.0f)    // Red
    };

    // Sparkline colors based on value
    auto getSparklineColor = [](float frac) -> ImU32 {
        if (frac > 0.90f) return IM_COL32(240, 80, 80, 255);   // Red
        if (frac > 0.70f) return IM_COL32(240, 190, 50, 255);  // Yellow
        return IM_COL32(80, 200, 80, 255);                      // Green
    };

    // Calculate responsive dimensions - match full-width sparkline margins
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float circleRadius = 5.0f;
    float rightMargin = 12.0f;  // Same as full-width sparklines
    float columnSpacing = 15.0f;
    // Total usable width = availableWidth - rightMargin, split into 3 columns with 2 gaps
    float columnWidth = (availableWidth - rightMargin - 2 * columnSpacing) / 3.0f;
    float sparklineHeight = 35.0f;
    float headerHeight = ImGui::GetTextLineHeight() + 4.0f;

    // Check for sparkline interaction (only if not in card drag mode)
    bool canInteract = !m_dragState.isDragging;
    bool isThisGpuZooming = m_zoomState.isDragging && m_zoomState.dragGpuUuid == stats.uuid;

    // Metric data for iteration
    struct MetricInfo {
        const char* label;
        const char* valueStr;  // Pre-formatted value string
        float frac;
        const float* data;
        const char* sparkId;
    };

    std::string powerSparkId = "##spark_power_" + stats.uuid;
    std::string coreSparkId = "##spark_core_" + stats.uuid;
    std::string memSparkId = "##spark_mem_" + stats.uuid;

    // Pre-format value strings
    std::string powerValueStr = std::format("{}/{}W", stats.powerDraw, stats.powerLimit);
    std::string coreValueStr = std::format("{}/{}MHz", stats.gpuClock, stats.gpuClockMax);
    std::string memValueStr = std::format("{}/{}MHz", stats.memClock, stats.memClockMax);

    MetricInfo metrics[] = {
        {"Power", powerValueStr.c_str(), powerFrac, powerData, powerSparkId.c_str()},
        {"Core", coreValueStr.c_str(), coreClockFrac, coreData, coreSparkId.c_str()},
        {"Mem", memValueStr.c_str(), memClockFrac, memData, memSparkId.c_str()}
    };

    bool anyHovered = false;
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    float leftOffset = circleRadius * 2 + 8;  // Circle + padding, same as full-width

    for (int col = 0; col < 3; col++) {
        const auto& m = metrics[col];
        float colX = startPos.x + col * (columnWidth + columnSpacing);
        float sparklineWidth = columnWidth - leftOffset;  // Sparkline fills rest of column

        // Health indicator circle (top-left, aligned with top of sparkline)
        int health = getMetricHealth(m.frac);
        ImVec2 circleCenter(colX + circleRadius + 2, startPos.y + headerHeight + circleRadius + 2);
        drawList->AddCircleFilled(circleCenter, circleRadius,
            ImGui::ColorConvertFloat4ToU32(healthColors[health]));

        // Sparkline position (after circle)
        float sparkX = colX + leftOffset;
        float sparkY = startPos.y + headerHeight;
        ImVec2 sparkPos(sparkX, sparkY);
        ImVec2 sparkSize(sparklineWidth, sparklineHeight);

        // Label (top-left, above sparkline)
        drawList->AddText(ImVec2(sparkX, startPos.y), IM_COL32(180, 180, 180, 255), m.label);

        // Value (top-right, above sparkline)
        ImVec2 valueSize = ImGui::CalcTextSize(m.valueStr);
        drawList->AddText(
            ImVec2(sparkX + sparklineWidth - valueSize.x, startPos.y),
            ImGui::ColorConvertFloat4ToU32(healthColors[health]),
            m.valueStr
        );

        // Sparkline background
        drawList->AddRectFilled(sparkPos,
            ImVec2(sparkPos.x + sparkSize.x, sparkPos.y + sparkSize.y),
            IM_COL32(20, 20, 25, 255));

        // Draw sparkline data
        if (dataCount > 1) {
            ImU32 lineColor = getSparklineColor(m.frac);
            float xStep = sparkSize.x / (dataCount - 1);

            for (size_t i = 1; i < dataCount; i++) {
                float x1 = sparkPos.x + (i - 1) * xStep;
                float x2 = sparkPos.x + i * xStep;
                float y1 = sparkPos.y + sparkSize.y - (m.data[i - 1] * sparkSize.y * 0.85f) - 3;
                float y2 = sparkPos.y + sparkSize.y - (m.data[i] * sparkSize.y * 0.85f) - 3;
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineColor, 1.5f);
            }
        }

        // Sparkline border
        ImU32 borderColor = isThisGpuZooming ? IM_COL32(100, 150, 255, 255) : IM_COL32(50, 50, 55, 255);
        drawList->AddRect(sparkPos,
            ImVec2(sparkPos.x + sparkSize.x, sparkPos.y + sparkSize.y), borderColor);

        // Invisible button for interaction
        ImGui::SetCursorScreenPos(sparkPos);
        ImGui::InvisibleButton(m.sparkId, sparkSize);

        bool thisHovered = ImGui::IsItemHovered();
        if (thisHovered) {
            anyHovered = true;

            // Time label only shown on hover (vertically centered, right-aligned)
            std::string timeLabel = std::format("{}s", displaySecs);
            ImVec2 timeLabelSize = ImGui::CalcTextSize(timeLabel.c_str());
            drawList->AddText(
                ImVec2(sparkPos.x + sparkSize.x - timeLabelSize.x - 4,
                       sparkPos.y + (sparkSize.y - timeLabelSize.y) / 2),
                IM_COL32(120, 120, 130, 255), timeLabel.c_str()
            );
        }
    }

    // Move cursor past our custom drawing
    ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + headerHeight + sparklineHeight + 8));

    // Handle zoom drag interaction (initiation only - ongoing drag is handled in renderGpuCard)
    if (canInteract) {
        // Start drag on any sparkline
        if (anyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_zoomState.isDragging = true;
            m_zoomState.dragGpuUuid = stats.uuid;
            m_zoomState.dragStartX = ImGui::GetMousePos().x;
            m_zoomState.originalDisplaySeconds = history.displaySeconds;
            m_zoomState.previewDisplaySeconds = history.displaySeconds;
        }

        // Right-click to reset zoom
        if (anyHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            history.resetZoom();
        }

        // Simple tooltip when hovering (not dragging)
        if (anyHovered && !m_zoomState.isDragging) {
            ImGui::SetTooltip("Drag to time-dilate | Right-click to reset");
        }
    }
}

void GpuMonitorUI::renderSystemHealth(const SystemInfo& sysInfo) {
    // Disable interaction during modal states
    if (isModalActive()) {
        ImGui::BeginDisabled();
    }

    // Show system info inline (no collapsible header)
    ImGui::TextDisabled("Driver %s", sysInfo.driverVersion.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| CUDA %s", sysInfo.cudaVersion.c_str());

    // NVLink status (only show if connected)
    if (sysInfo.nvlinkAvailable) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.3f, 1.0f), "| NVLink");
        for (const auto& pair : sysInfo.nvlinkPairs) {
            ImGui::SameLine();
            ImGui::TextDisabled("(%d<->%d)", pair.first, pair.second);
        }
    }

    ImGui::Spacing();

    if (isModalActive()) {
        ImGui::EndDisabled();
    }
}

std::string GpuMonitorUI::buildGpuSelectionString(const QuickLaunchPreset& preset, const std::vector<GpuStats>& gpuStats) {
    // Build CUDA_VISIBLE_DEVICES string from selected GPU UUIDs
    // If no GPUs selected (empty string), return empty (means all)
    if (preset.selectedGpuUuids.empty()) {
        return "";
    }

    std::string result;

    for (const auto& gpu : gpuStats) {
        // Check if this GPU's UUID is in the selected list
        if (preset.selectedGpuUuids.find(gpu.uuid) != std::string::npos) {
            if (!result.empty()) result += ",";
            result += std::to_string(gpu.cudaIndex);
        }
    }
    return result;
}

bool GpuMonitorUI::isGpuSelectedInPreset(const QuickLaunchPreset& preset, const std::string& uuid) {
    if (preset.selectedGpuUuids.empty()) return false;
    return preset.selectedGpuUuids.find(uuid) != std::string::npos;
}

void GpuMonitorUI::toggleGpuInPreset(QuickLaunchPreset& preset, const std::string& uuid) {
    std::string& uuids = preset.selectedGpuUuids;

    size_t pos = uuids.find(uuid);
    if (pos != std::string::npos) {
        // Remove this UUID
        size_t end = pos + uuid.length();
        // Also remove trailing comma if present
        if (end < uuids.length() && uuids[end] == ',') end++;
        // Or leading comma if this wasn't the first
        else if (pos > 0 && uuids[pos - 1] == ',') pos--;
        uuids.erase(pos, end - pos);
    } else {
        // Add this UUID
        if (!uuids.empty()) uuids += ",";
        uuids += uuid;
    }
}

void GpuMonitorUI::renderQuickLaunch(const std::vector<GpuStats>& gpuStats) {
    // Disable interaction during modal states
    if (isModalActive()) {
        ImGui::BeginDisabled();
    }

    if (ImGui::CollapsingHeader("Quick Launch", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Indent(10);

        // Show existing presets
        for (size_t i = 0; i < m_settings.presets.size(); i++) {
            ImGui::PushID(static_cast<int>(i));
            auto& preset = m_settings.presets[i];

            // Build GPU label from selected GPUs
            std::string gpuLabel;
            if (preset.selectedGpuUuids.empty()) {
                gpuLabel = "ALL";
            } else {
                int count = 0;
                for (const auto& gpu : gpuStats) {
                    if (isGpuSelectedInPreset(preset, gpu.uuid)) {
                        if (count > 0) gpuLabel += ",";
                        gpuLabel += getGpuDisplayName(gpu);
                        count++;
                    }
                }
                if (gpuLabel.empty()) gpuLabel = "ALL";
            }

            if (ImGui::Button(!preset.name.empty() ? preset.name.c_str() : "Unnamed")) {
                // Launch the preset
                std::string gpuSel = buildGpuSelectionString(preset, gpuStats);
                Platform::executeCommand(preset.command, preset.workingDir,
                                         gpuSel.empty() ? "" : "CUDA_VISIBLE_DEVICES", gpuSel);
                showCopiedToast("Launched");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("[%s]", gpuLabel.c_str());

            // Edit button
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
            if (ImGui::SmallButton("Edit")) {
                ImGui::OpenPopup("EditPreset");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("X")) {
                // Remove preset
                m_settings.presets.erase(m_settings.presets.begin() + i);
                saveSettings();
                ImGui::PopID();
                break;  // Iterator invalidated, exit loop
            }

            // Edit popup
            if (ImGui::BeginPopup("EditPreset")) {
                ImGui::Text("Edit Preset");
                ImGui::Separator();
                ImGui::Spacing();

                InputTextString("Name", preset.name);

                ImGui::Spacing();
                ImGui::Text("GPUs:");
                ImGui::SameLine();
                ImGui::TextDisabled("(none = all)");

                // Dynamic GPU checkboxes
                for (const auto& gpu : gpuStats) {
                    ImGui::PushID(gpu.uuid.c_str());
                    bool selected = isGpuSelectedInPreset(preset, gpu.uuid);
                    std::string label = getGpuDisplayName(gpu) + " (cuda:" + std::to_string(gpu.cudaIndex) + ")";
                    if (ImGui::Checkbox(label.c_str(), &selected)) {
                        toggleGpuInPreset(preset, gpu.uuid);
                    }
                    ImGui::PopID();
                }

                ImGui::Spacing();
                InputTextString("Working Dir", preset.workingDir);
                ImGui::SameLine();
                if (ImGui::SmallButton("...")) {
                    std::string folder = Platform::browseForFolder("Select Working Directory");
                    if (!folder.empty()) {
                        preset.workingDir = folder;
                    }
                }

                ImGui::Spacing();
                InputTextString("Command", preset.command);
                ImGui::TextDisabled("(optional - runs after setting GPU)");

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();

                if (ImGui::Button("Done", ImVec2(80, 0))) {
                    saveSettings();
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        // Add new preset button
        if (m_settings.presets.size() < 5) {
            if (ImGui::Button("+ Add Preset")) {
                QuickLaunchPreset preset;
                preset.name = "New Preset";
                m_settings.presets.push_back(std::move(preset));
                saveSettings();
            }
        }

        ImGui::Unindent(10);
        ImGui::Spacing();
    }

    if (isModalActive()) {
        ImGui::EndDisabled();
    }
}

void GpuMonitorUI::renderProcessesSection(const GpuStats& stats) {
    if (stats.processes.empty()) {
        ImGui::TextDisabled("No processes running");
        return;
    }

    for (const auto& proc : stats.processes) {
        ImGui::PushID(proc.pid);

        // Process name and PID
        ImGui::Text("%s", proc.name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("(PID: %u)", proc.pid);

        // Kill button on the right
        ImGui::SameLine(ImGui::GetContentRegionAvail().x - 30);
        if (ImGui::SmallButton("Kill")) {
            m_confirmDialog.isOpen = true;
            m_confirmDialog.isDangerous = true;
            m_confirmDialog.title = "Kill Process";
            m_confirmDialog.message = "Kill process " + proc.name + " (PID: " + std::to_string(proc.pid) + ")?\n\nThis may cause data loss!";
            m_confirmDialog.command = "taskkill /F /PID " + std::to_string(proc.pid);
        }

        // Memory bar
        float memFrac = stats.vramTotal > 0 ? static_cast<float>(proc.usedMemory) / stats.vramTotal : 0.0f;
        float memGB = static_cast<float>(proc.usedMemory) / (1024.0f * 1024.0f * 1024.0f);

        ImGui::ProgressBar(memFrac, ImVec2(-60, 0));
        ImGui::SameLine();
        ImGui::Text("%.1fGB", memGB);

        ImGui::Spacing();
        ImGui::PopID();
    }

    // ECC errors (hidden by default, show if supported)
    if (stats.eccSupported && stats.eccErrors > 0) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "ECC Errors: %llu", stats.eccErrors);
    }
}

void GpuMonitorUI::render(const std::vector<GpuStats>& gpuStats, const SystemInfo& sysInfo) {
    ImGuiIO& io = ImGui::GetIO();

    // Update toast timer
    if (m_toastTimer > 0) {
        m_toastTimer -= io.DeltaTime;
    }

    // Set window to fill the viewport
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(io.DisplaySize);

    ImGuiWindowFlags windowFlags =
        ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoResize |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("GPU Monitor", nullptr, windowFlags);

    ImGui::Text("GPU Monitor");
    ImGui::SameLine(ImGui::GetWindowWidth() - 100);
    ImGui::TextDisabled("%.0f FPS", io.Framerate);
    ImGui::Separator();
    ImGui::Spacing();

    // Global sections at top
    renderSystemHealth(sysInfo);
    renderQuickLaunch(gpuStats);

    ImGui::Separator();
    ImGui::Spacing();

    // Handle drag state - check if we need to perform reorder on mouse release
    bool wasDragging = m_dragState.isDragging;
    int hoverIndexBeforeReset = m_dragState.currentHoverIndex;

    if (!ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        if (wasDragging && hoverIndexBeforeReset >= 0 &&
            hoverIndexBeforeReset != m_dragState.dragSourceIndex) {
            // Mouse released over a valid drop target - perform reorder
            auto sortedStats = sortGpusByUserOrder(gpuStats);
            commitReorder(m_dragState.dragSourceIndex, hoverIndexBeforeReset, sortedStats);
        }
        // Reset drag state
        m_dragState.isDragging = false;
        m_dragState.draggedUuid.clear();
        m_dragState.dragSourceIndex = -1;
        m_dragState.currentHoverIndex = -1;
    } else if (m_dragState.isDragging) {
        // Reset hover index at start of each frame during drag
        // Each card will set it if the mouse is over it
        m_dragState.currentHoverIndex = -1;
    }

    if (gpuStats.empty()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.5f, 1.0f), "No GPUs detected");
    } else {
        // Sort GPUs by user-defined order
        auto sortedStats = sortGpusByUserOrder(gpuStats);

        // Initialize per-card position tracking
        m_dragState.cardStartY.resize(sortedStats.size());
        m_dragState.cardEndY.resize(sortedStats.size());

        for (size_t i = 0; i < sortedStats.size(); i++) {
            renderGpuCard(sortedStats[i], gpuStats, static_cast<int>(i));
        }

        // Render drop indicator during drag
        renderDropIndicator(m_dragState.currentHoverIndex);
    }

    // Toast notification
    if (m_toastTimer > 0 && !m_toastMessage.empty()) {
        ImVec2 toastPos(io.DisplaySize.x - 220, io.DisplaySize.y - 50);
        ImGui::SetNextWindowPos(toastPos);
        ImGui::SetNextWindowSize(ImVec2(200, 0));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.2f, 0.6f, 0.2f, 0.9f));
        ImGui::Begin("##toast", nullptr,
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
            ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
            ImGuiWindowFlags_AlwaysAutoResize);
        ImGui::Text("%s", m_toastMessage.c_str());
        ImGui::End();
        ImGui::PopStyleColor();
    }

    // Confirmation dialog
    renderConfirmDialog();

    // Modal overlay (drawn on foreground, so after all other content)
    renderModalOverlay();

    ImGui::End();
}

void GpuMonitorUI::renderConfirmDialog() {
    if (!m_confirmDialog.isOpen) return;

    ImGui::OpenPopup("Confirm Action");

    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(450, 0));

    if (ImGui::BeginPopupModal("Confirm Action", &m_confirmDialog.isOpen, ImGuiWindowFlags_AlwaysAutoResize)) {
        if (m_confirmDialog.isDangerous) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.7f, 0.3f, 1.0f));
            ImGui::Text("WARNING: This action may require admin privileges");
            ImGui::PopStyleColor();
            ImGui::Spacing();
        }

        ImGui::Text("%s", m_confirmDialog.title.c_str());
        ImGui::Spacing();
        ImGui::TextWrapped("%s", m_confirmDialog.message.c_str());
        ImGui::Spacing();

        // Command preview
        ImGui::Text("Command:");
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        ImGui::InputTextMultiline("##cmd", const_cast<char*>(m_confirmDialog.command.c_str()),
            m_confirmDialog.command.size() + 1, ImVec2(-1, 60),
            ImGuiInputTextFlags_ReadOnly);
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Copy Command", ImVec2(120, 0))) {
            copyToClipboard(m_confirmDialog.command);
            showCopiedToast("Command");
            m_confirmDialog.isOpen = false;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(80, 0))) {
            m_confirmDialog.isOpen = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
    }
}

void GpuMonitorUI::renderBadge(const char* text, bool isTCC) {
    ImVec4 color = isTCC
        ? ImVec4(0.2f, 0.7f, 0.3f, 1.0f)   // Green for TCC
        : ImVec4(0.3f, 0.5f, 0.8f, 1.0f);  // Blue for WDDM

    ImGui::SameLine();
    ImGui::TextColored(color, "[%s]", text);
}

void GpuMonitorUI::renderGpuCard(const GpuStats& stats, const std::vector<GpuStats>& allStats, int index) {
    ImGui::PushID(index);

    GpuCardState& cardState = getCardState(stats.uuid);
    std::string displayName = getGpuDisplayName(stats);
    bool ctrlHeld = ImGui::GetIO().KeyCtrl;
    bool isDragging = m_dragState.isDragging;
    bool isBeingDragged = (isDragging && m_dragState.draggedUuid == stats.uuid);

    // Store card start position
    ImVec2 cardStartPos = ImGui::GetCursorScreenPos();
    float cardWidth = ImGui::GetContentRegionAvail().x;

    // Store this card's start Y position
    if (index < static_cast<int>(m_dragState.cardStartY.size())) {
        m_dragState.cardStartY[index] = cardStartPos.y;
    }

    // Check if this card is being hovered during drag (using actual card bounds from previous frame)
    bool isHoveredDuringDrag = false;
    if (isDragging && !isBeingDragged) {
        ImVec2 mousePos = ImGui::GetMousePos();

        // Use actual card end position from previous frame if available
        float cardEndY = cardStartPos.y + 200.0f;  // Default fallback
        if (index < static_cast<int>(m_dragState.cardEndY.size()) && m_dragState.cardEndY[index] > 0) {
            cardEndY = m_dragState.cardEndY[index];
        }

        if (mousePos.x >= cardStartPos.x && mousePos.x <= cardStartPos.x + cardWidth &&
            mousePos.y >= cardStartPos.y && mousePos.y <= cardEndY) {
            isHoveredDuringDrag = true;
            m_dragState.currentHoverIndex = index;
        }
    }

    // Draw highlight background if hovered during drag (use actual bounds)
    if (isHoveredDuringDrag) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float cardEndY = cardStartPos.y + 200.0f;
        if (index < static_cast<int>(m_dragState.cardEndY.size()) && m_dragState.cardEndY[index] > 0) {
            cardEndY = m_dragState.cardEndY[index];
        }

        ImU32 highlightColor = IM_COL32(80, 150, 255, 50);
        drawList->AddRectFilled(
            cardStartPos,
            ImVec2(cardStartPos.x + cardWidth, cardEndY),
            highlightColor
        );
    }

    // Make dragged card slightly transparent
    if (isBeingDragged) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    }

    // Start a group for the entire card (for drop target)
    ImGui::BeginGroup();

    // Header row with drag handle
    renderDragHandle(stats, displayName, index);
    ImGui::SameLine();

    // GPU Name
    ImGui::Text("%s", stats.name.c_str());

    // Nickname - Ctrl+click to rename (disabled during drag)
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", displayName.c_str());

    // Popup ID for nickname editing
    std::string popupId = "RenamePopup_" + stats.uuid;

    // Check if nickname was clicked (only when not dragging)
    if (!isDragging && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted("Ctrl+click to rename");
        ImGui::EndTooltip();

        if (ImGui::IsItemClicked() && ctrlHeld) {
            ImGui::OpenPopup(popupId.c_str());
            cardState.focusNickname = true;
        }
    }

    // Nickname rename popup
    if (ImGui::BeginPopup(popupId.c_str())) {
        GpuConfig* config = getOrCreateGpuConfig(stats.uuid);
        if (config) {
            ImGui::Text("Rename GPU:");
            ImGui::SetNextItemWidth(200);

            if (cardState.focusNickname) {
                ImGui::SetKeyboardFocusHere();
                cardState.focusNickname = false;
            }

            bool enterPressed = InputTextString("##nickname", config->nickname, ImGuiInputTextFlags_EnterReturnsTrue);

            if (enterPressed) {
                saveSettings();
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("OK")) {
                saveSettings();
                ImGui::CloseCurrentPopup();
            }
        }
        ImGui::EndPopup();
    }

    renderBadge(stats.isTCC ? "TCC" : "WDDM", stats.isTCC);
    ImGui::SameLine();
    ImGui::TextDisabled("cuda:%u", stats.cudaIndex);

    // Collapse/Expand button (right-aligned)
    ImGui::SameLine(ImGui::GetContentRegionAvail().x - 20);
    std::string collapseId = "##collapse_" + stats.uuid;
    if (ImGui::SmallButton(cardState.collapsed ? "+" : "-")) {
        cardState.collapsed = !cardState.collapsed;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::TextUnformatted(cardState.collapsed ? "Expand" : "Collapse");
        ImGui::EndTooltip();
    }

    // Calculate basic fractions (needed even when collapsed for history tracking)
    float vramUsedGB = static_cast<float>(stats.vramUsed) / (1024.0f * 1024.0f * 1024.0f);
    float vramTotalGB = static_cast<float>(stats.vramTotal) / (1024.0f * 1024.0f * 1024.0f);
    float vramFrac = vramTotalGB > 0 ? vramUsedGB / vramTotalGB : 0.0f;
    float gpuUtilFrac = stats.gpuUtilization / 100.0f;
    float powerFrac = stats.powerLimit > 0 ? static_cast<float>(stats.powerDraw) / stats.powerLimit : 0.0f;
    float coreClockFrac = stats.gpuClockMax > 0 ? static_cast<float>(stats.gpuClock) / stats.gpuClockMax : 0.0f;
    float memClockFrac = stats.memClockMax > 0 ? static_cast<float>(stats.memClock) / stats.memClockMax : 0.0f;
    float tempFrac = stats.temperature / 100.0f;
    float fanFrac = stats.fanSpeed / 100.0f;

    // Always track history even when collapsed
    GpuMetricHistory& history = m_metricHistory[stats.uuid];
    float deltaTime = ImGui::GetIO().DeltaTime;
    history.addSample(deltaTime, vramFrac, gpuUtilFrac, powerFrac, coreClockFrac, memClockFrac, tempFrac, fanFrac);

    // Collapsed view: compact visual indicators in a single row
    if (cardState.collapsed) {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float rowHeight = 16.0f;
        float curX = startPos.x;
        float barWidth = 50.0f;
        float barHeight = 8.0f;
        float barY = startPos.y + 4.0f;

        // VRAM mini bar with 4-color health
        int vramHealth = getVramHealth(vramFrac);
        ImU32 vramColor = ImGui::ColorConvertFloat4ToU32(getHealthColor4(vramHealth));
        drawList->AddText(ImVec2(curX, startPos.y), IM_COL32(120, 120, 120, 255), "V");
        curX += 12.0f;
        drawList->AddRectFilled(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(30, 30, 35, 255), 2.0f);
        if (vramFrac > 0.01f) {
            drawList->AddRectFilled(ImVec2(curX, barY),
                ImVec2(curX + barWidth * vramFrac, barY + barHeight), vramColor, 2.0f);
        }
        drawList->AddRect(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(50, 50, 55, 255), 2.0f);
        curX += barWidth + 3.0f;
        std::string vramStr = std::format("{}%", static_cast<int>(vramFrac * 100.0f + 0.5f));
        drawList->AddText(ImVec2(curX, startPos.y), vramColor, vramStr.c_str());
        curX += ImGui::CalcTextSize(vramStr.c_str()).x + 10.0f;

        // GPU mini bar with 4-color health
        int gpuHealth = getVramHealth(gpuUtilFrac);  // Same thresholds as VRAM
        ImU32 gpuColor = ImGui::ColorConvertFloat4ToU32(getHealthColor4(gpuHealth));
        drawList->AddText(ImVec2(curX, startPos.y), IM_COL32(120, 120, 120, 255), "G");
        curX += 12.0f;
        drawList->AddRectFilled(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(30, 30, 35, 255), 2.0f);
        if (gpuUtilFrac > 0.01f) {
            drawList->AddRectFilled(ImVec2(curX, barY),
                ImVec2(curX + barWidth * gpuUtilFrac, barY + barHeight), gpuColor, 2.0f);
        }
        drawList->AddRect(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(50, 50, 55, 255), 2.0f);
        curX += barWidth + 3.0f;
        std::string gpuStr = std::format("{}%", stats.gpuUtilization);
        drawList->AddText(ImVec2(curX, startPos.y), gpuColor, gpuStr.c_str());
        curX += ImGui::CalcTextSize(gpuStr.c_str()).x + 10.0f;

        // Fan donut widget
        int fanHealth = getFanHealth(stats.fanSpeed);
        ImU32 fanColor = ImGui::ColorConvertFloat4ToU32(getHealthColor4(fanHealth));
        float donutCenterX = curX + 6.0f;
        float donutCenterY = startPos.y + rowHeight / 2.0f;
        float donutRadius = 6.0f;
        float donutThickness = 2.5f;
        float fanFillFrac = stats.fanSpeed / 100.0f;

        drawList->AddCircle(ImVec2(donutCenterX, donutCenterY), donutRadius,
            IM_COL32(40, 40, 45, 255), 16, donutThickness);
        if (fanFillFrac > 0.01f) {
            float startAngle = -3.14159f / 2.0f;
            float endAngle = startAngle + fanFillFrac * 2.0f * 3.14159f;
            drawList->PathArcTo(ImVec2(donutCenterX, donutCenterY), donutRadius,
                startAngle, endAngle, 16);
            drawList->PathStroke(fanColor, 0, donutThickness);
        }
        curX += 16.0f;
        std::string fanStr = std::format("{}%", stats.fanSpeed);
        drawList->AddText(ImVec2(curX, startPos.y), fanColor, fanStr.c_str());
        curX += ImGui::CalcTextSize(fanStr.c_str()).x + 10.0f;

        // Temperature with 4-color health
        int tempHealth = getTempHealth(stats.temperature);
        ImU32 tempColor = ImGui::ColorConvertFloat4ToU32(getHealthColor4(tempHealth));
        std::string tempStr = std::format("{}C", stats.temperature);
        drawList->AddText(ImVec2(curX, startPos.y), tempColor, tempStr.c_str());
        curX += ImGui::CalcTextSize(tempStr.c_str()).x + 10.0f;

        // PCIe info (right side)
        std::string pcieStr = std::format("Gen{} x{}", stats.pcieGen, stats.pcieWidth);
        drawList->AddText(ImVec2(curX, startPos.y), IM_COL32(100, 100, 100, 255), pcieStr.c_str());

        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + rowHeight + 4.0f));
        ImGui::Separator();
        ImGui::Spacing();

        // End group and return early
        ImGui::EndGroup();
        ImVec2 cardEndPos = ImGui::GetCursorScreenPos();

        // Store this card's actual end Y position
        if (index < static_cast<int>(m_dragState.cardEndY.size())) {
            m_dragState.cardEndY[index] = cardEndPos.y;
        }

        if (isBeingDragged) {
            ImGui::PopStyleVar();
        }
        ImGui::PopID();
        return;
    }

    ImGui::Separator();
    ImGui::Spacing();

    int displaySecs = history.displaySeconds;
    if (m_zoomState.isDragging && m_zoomState.dragGpuUuid == stats.uuid) {
        displaySecs = m_zoomState.previewDisplaySeconds;
    }

    // Health colors (3-level for Power/Core/Mem)
    ImVec4 healthColors[] = {
        ImVec4(0.3f, 0.85f, 0.3f, 1.0f),   // Green
        ImVec4(0.95f, 0.75f, 0.2f, 1.0f),  // Yellow
        ImVec4(0.95f, 0.3f, 0.3f, 1.0f)    // Red
    };

    // Sparkline colors for Power/Core/Mem (70%/90% thresholds)
    auto getSparklineColor = [](float frac) -> ImU32 {
        if (frac > 0.90f) return IM_COL32(240, 80, 80, 255);
        if (frac > 0.70f) return IM_COL32(240, 190, 50, 255);
        return IM_COL32(80, 200, 80, 255);
    };

    // Sparkline colors for VRAM/GPU (40%/70% thresholds - matches getVramHealth)
    auto getVramSparklineColor = [](float frac) -> ImU32 {
        if (frac > 0.70f) return IM_COL32(240, 80, 80, 255);   // Red
        if (frac > 0.40f) return IM_COL32(240, 190, 50, 255);  // Yellow
        return IM_COL32(80, 200, 80, 255);                      // Green
    };

    // Get history data
    float vramData[GpuMetricHistory::HISTORY_SIZE];
    float gpuUtilData[GpuMetricHistory::HISTORY_SIZE];
    size_t dataCount = 0;

    int savedDisplaySecs = history.displaySeconds;
    history.displaySeconds = displaySecs;
    history.getOrderedMetric(history.vramHistory, vramData, dataCount);
    history.getOrderedMetric(history.gpuUtilHistory, gpuUtilData, dataCount);
    history.displaySeconds = savedDisplaySecs;

    // Layout dimensions - must match compact metrics margins
    float availableWidth = ImGui::GetContentRegionAvail().x;
    float circleRadius = 5.0f;
    float rightMargin = 12.0f;  // Consistent right margin with compact metrics
    float sparklineHeight = 35.0f;
    float leftOffset = circleRadius * 2 + 8;  // Circle + padding
    float sparklineWidth = availableWidth - leftOffset - rightMargin;
    float headerHeight = ImGui::GetTextLineHeight() + 4.0f;

    bool canInteract = !m_dragState.isDragging;
    bool isThisGpuZooming = m_zoomState.isDragging && m_zoomState.dragGpuUuid == stats.uuid;
    bool anyVramGpuHovered = false;

    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Render full-width metric with sparkline
    // useVramThresholds: true for VRAM/GPU (40%/70%), false for Power/Core/Mem (70%/90%)
    auto renderFullWidthMetric = [&](const char* label, const char* valueStr, float frac,
                                      const float* data, size_t count, const char* sparkId,
                                      bool useVramThresholds = false) {
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        int health = useVramThresholds ? getVramHealth(frac) : getMetricHealth(frac);

        // Health indicator circle (left side, vertically centered with sparkline)
        ImVec2 circleCenter(startPos.x + circleRadius + 2,
                           startPos.y + headerHeight + sparklineHeight / 2);
        drawList->AddCircleFilled(circleCenter, circleRadius,
            ImGui::ColorConvertFloat4ToU32(healthColors[health]));

        // Sparkline area starts after circle
        float sparkX = startPos.x + leftOffset;
        float sparkY = startPos.y + headerHeight;
        ImVec2 sparkPos(sparkX, sparkY);
        ImVec2 sparkSize(sparklineWidth, sparklineHeight);

        // Label (top-left, above sparkline)
        drawList->AddText(ImVec2(sparkX, startPos.y), IM_COL32(180, 180, 180, 255), label);

        // Value (top-right, above sparkline)
        ImVec2 valueSize = ImGui::CalcTextSize(valueStr);
        drawList->AddText(
            ImVec2(sparkX + sparklineWidth - valueSize.x, startPos.y),
            ImGui::ColorConvertFloat4ToU32(healthColors[health]),
            valueStr
        );

        // Sparkline background
        drawList->AddRectFilled(sparkPos,
            ImVec2(sparkPos.x + sparkSize.x, sparkPos.y + sparkSize.y),
            IM_COL32(20, 20, 25, 255));

        // Draw sparkline data
        if (count > 1) {
            ImU32 lineColor = useVramThresholds ? getVramSparklineColor(frac) : getSparklineColor(frac);
            float xStep = sparkSize.x / (count - 1);

            for (size_t i = 1; i < count; i++) {
                float x1 = sparkPos.x + (i - 1) * xStep;
                float x2 = sparkPos.x + i * xStep;
                float y1 = sparkPos.y + sparkSize.y - (data[i - 1] * sparkSize.y * 0.85f) - 3;
                float y2 = sparkPos.y + sparkSize.y - (data[i] * sparkSize.y * 0.85f) - 3;
                drawList->AddLine(ImVec2(x1, y1), ImVec2(x2, y2), lineColor, 1.5f);
            }
        }

        // Sparkline border
        ImU32 borderColor = isThisGpuZooming ? IM_COL32(100, 150, 255, 255) : IM_COL32(50, 50, 55, 255);
        drawList->AddRect(sparkPos, ImVec2(sparkPos.x + sparkSize.x, sparkPos.y + sparkSize.y), borderColor);

        // Invisible button for interaction
        ImGui::SetCursorScreenPos(sparkPos);
        ImGui::InvisibleButton(sparkId, sparkSize);
        bool hovered = ImGui::IsItemHovered();

        // Time label only shown on hover (vertically centered, right-aligned)
        if (hovered) {
            std::string timeLabel = std::format("{}s", displaySecs);
            ImVec2 timeLabelSize = ImGui::CalcTextSize(timeLabel.c_str());
            drawList->AddText(
                ImVec2(sparkPos.x + sparkSize.x - timeLabelSize.x - 4,
                       sparkPos.y + (sparkSize.y - timeLabelSize.y) / 2),
                IM_COL32(120, 120, 130, 255), timeLabel.c_str()
            );
        }

        // Move cursor past this metric
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + headerHeight + sparklineHeight + 6));

        return hovered;
    };

    // VRAM - show used/total, percentage, and available
    float vramAvailGB = vramTotalGB - vramUsedGB;
    int vramPercent = static_cast<int>(vramFrac * 100.0f + 0.5f);
    std::string vramValueStr = std::format("{:.1f}/{:.0f}GB ({}%) | {:.1f}GB free",
                                           vramUsedGB, vramTotalGB, vramPercent, vramAvailGB);
    std::string vramSparkId = "##spark_vram_" + stats.uuid;
    if (renderFullWidthMetric("VRAM", vramValueStr.c_str(), vramFrac,
                               vramData, dataCount, vramSparkId.c_str(), true)) {
        anyVramGpuHovered = true;
    }

    // GPU Utilization
    std::string gpuValueStr = std::format("{}%", stats.gpuUtilization);
    std::string gpuSparkId = "##spark_gpuutil_" + stats.uuid;
    if (renderFullWidthMetric("GPU", gpuValueStr.c_str(), gpuUtilFrac,
                               gpuUtilData, dataCount, gpuSparkId.c_str(), true)) {
        anyVramGpuHovered = true;
    }

    // Handle zoom drag interaction for VRAM/GPU sparklines
    if (canInteract) {
        if (anyVramGpuHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            m_zoomState.isDragging = true;
            m_zoomState.dragGpuUuid = stats.uuid;
            m_zoomState.dragStartX = ImGui::GetMousePos().x;
            m_zoomState.originalDisplaySeconds = history.displaySeconds;
            m_zoomState.previewDisplaySeconds = history.displaySeconds;
        }

        if (anyVramGpuHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            history.resetZoom();
        }

        if (anyVramGpuHovered && !m_zoomState.isDragging) {
            ImGui::SetTooltip("Drag to time-dilate | Right-click to reset");
        }
    }

    // Handle ongoing zoom drag
    if (m_zoomState.isDragging && m_zoomState.dragGpuUuid == stats.uuid) {
        float deltaX = ImGui::GetMousePos().x - m_zoomState.dragStartX;
        int deltaSecs = static_cast<int>(deltaX / 2.0f);
        int newSecs = m_zoomState.originalDisplaySeconds + deltaSecs;
        newSecs = std::clamp(newSecs, GpuMetricHistory::MIN_DISPLAY_SECONDS,
            GpuMetricHistory::MAX_DISPLAY_SECONDS);

        bool shiftHeld = ImGui::GetIO().KeyShift;
        if (shiftHeld) {
            newSecs = ((newSecs + 2) / 5) * 5;
            newSecs = std::clamp(newSecs, GpuMetricHistory::MIN_DISPLAY_SECONDS,
                GpuMetricHistory::MAX_DISPLAY_SECONDS);
        }

        m_zoomState.previewDisplaySeconds = newSecs;

        ImGui::BeginTooltip();
        ImGui::Text("Time range: %ds", newSecs);
        if (shiftHeld) {
            ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "(snap to 5s)");
        }
        if (newSecs == GpuMetricHistory::MIN_DISPLAY_SECONDS) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(minimum)");
        } else if (newSecs == GpuMetricHistory::MAX_DISPLAY_SECONDS) {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "(maximum)");
        }
        ImGui::EndTooltip();

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            history.displaySeconds = m_zoomState.previewDisplaySeconds;
            m_zoomState.isDragging = false;
            m_zoomState.dragGpuUuid.clear();
        }
    }

    // Compact metrics with sparklines (Power, Core, Memory)
    renderCompactMetrics(stats);

    ImGui::Spacing();

    // Temperature, Fan (left) | PCIe, Bus (right-aligned)
    {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        ImVec2 startPos = ImGui::GetCursorScreenPos();
        float rowHeight = 20.0f;
        float availWidth = ImGui::GetContentRegionAvail().x;
        float curX = startPos.x;

        // === LEFT SIDE: Temperature + Fan ===

        // Temperature: Label + Bar + Value
        int tempHealth = getTempHealth(stats.temperature);
        ImVec4 tempColor = getHealthColor4(tempHealth);
        ImU32 tempColorU32 = ImGui::ColorConvertFloat4ToU32(tempColor);

        drawList->AddText(ImVec2(curX, startPos.y), IM_COL32(140, 140, 140, 255), "Temp");
        curX += 30.0f;

        float barY = startPos.y + 5.0f;
        float barWidth = 40.0f;
        float barHeight = 8.0f;
        float tempFillFrac = std::clamp(stats.temperature / 100.0f, 0.0f, 1.0f);

        drawList->AddRectFilled(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(30, 30, 35, 255), 2.0f);
        if (tempFillFrac > 0.01f) {
            drawList->AddRectFilled(ImVec2(curX, barY),
                ImVec2(curX + barWidth * tempFillFrac, barY + barHeight),
                tempColorU32, 2.0f);
        }
        drawList->AddRect(ImVec2(curX, barY), ImVec2(curX + barWidth, barY + barHeight),
            IM_COL32(60, 60, 65, 255), 2.0f);
        curX += barWidth + 3.0f;

        std::string tempStr = std::format("{}C", stats.temperature);
        drawList->AddText(ImVec2(curX, startPos.y), tempColorU32, tempStr.c_str());
        curX += ImGui::CalcTextSize(tempStr.c_str()).x + 14.0f;

        // Fan: Label + Donut + Value
        int fanHealth = getFanHealth(stats.fanSpeed);
        ImVec4 fanColor = getHealthColor4(fanHealth);
        ImU32 fanColorU32 = ImGui::ColorConvertFloat4ToU32(fanColor);

        drawList->AddText(ImVec2(curX, startPos.y), IM_COL32(140, 140, 140, 255), "Fan");
        curX += 24.0f;

        float donutCenterX = curX + 6.0f;
        float donutCenterY = startPos.y + rowHeight / 2.0f;
        float donutRadius = 6.0f;
        float donutThickness = 2.5f;
        float fanFillFrac = stats.fanSpeed / 100.0f;

        drawList->AddCircle(ImVec2(donutCenterX, donutCenterY), donutRadius,
            IM_COL32(40, 40, 45, 255), 20, donutThickness);
        if (fanFillFrac > 0.01f) {
            float startAngle = -3.14159f / 2.0f;
            float endAngle = startAngle + fanFillFrac * 2.0f * 3.14159f;
            drawList->PathArcTo(ImVec2(donutCenterX, donutCenterY), donutRadius,
                startAngle, endAngle, 20);
            drawList->PathStroke(fanColorU32, 0, donutThickness);
        }
        curX += 16.0f;

        std::string fanStr = std::format("{}%", stats.fanSpeed);
        drawList->AddText(ImVec2(curX, startPos.y), fanColorU32, fanStr.c_str());

        // === RIGHT SIDE: PCIe + Bus (right-aligned) ===
        std::string pcieStr = std::format("Gen{} x{}", stats.pcieGen, stats.pcieWidth);
        ImVec2 pcieSize = ImGui::CalcTextSize(pcieStr.c_str());
        ImVec2 busSize = ImGui::CalcTextSize(stats.pciBusId.c_str());

        float rightMargin = 8.0f;
        float gap = 10.0f;
        float rightX = startPos.x + availWidth - rightMargin;

        // Bus ID (rightmost, dimmed)
        float busX = rightX - busSize.x;
        drawList->AddText(ImVec2(busX, startPos.y), IM_COL32(90, 90, 90, 255), stats.pciBusId.c_str());

        // PCIe (to the left of Bus)
        float pcieX = busX - gap - pcieSize.x;
        drawList->AddText(ImVec2(pcieX, startPos.y), IM_COL32(140, 140, 140, 255), pcieStr.c_str());

        // Advance cursor
        ImGui::SetCursorScreenPos(ImVec2(startPos.x, startPos.y + rowHeight));
    }

    ImGui::Spacing();

    // Processes section (collapsible) - disabled during drag
    std::string procHeader = "Processes (" + std::to_string(stats.processes.size()) + ")";
    if (isDragging) {
        // Show as non-interactive text during drag
        ImGui::TextDisabled("> %s", procHeader.c_str());
    } else {
        if (ImGui::CollapsingHeader(procHeader.c_str())) {
            ImGui::Indent(10);
            renderProcessesSection(stats);
            ImGui::Unindent(10);
        }
    }

    // Commands section (collapsible) - disabled during drag
    if (isDragging) {
        ImGui::TextDisabled("> Commands");
    } else {
        if (ImGui::CollapsingHeader("Commands")) {
            renderCommandsSection(stats, allStats);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // End the card group
    ImGui::EndGroup();

    // Store this card's actual end Y position
    ImVec2 cardEndPos = ImGui::GetCursorScreenPos();
    if (index < static_cast<int>(m_dragState.cardEndY.size())) {
        m_dragState.cardEndY[index] = cardEndPos.y;
    }

    if (isBeingDragged) {
        ImGui::PopStyleVar(); // Alpha
    }

    ImGui::PopID();
}

void GpuMonitorUI::renderCommandsSection(const GpuStats& stats, const std::vector<GpuStats>& allStats) {
    ImGui::Indent(10);

    std::string displayName = getGpuDisplayName(stats);

    // === CUDA_VISIBLE_DEVICES Section ===
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "CUDA Device Selection");
    ImGui::Spacing();

    // Use only this GPU
    {
        std::string idx = std::format("{}", stats.cudaIndex);
        std::string cmd = std::format("$env:CUDA_VISIBLE_DEVICES=\"{}\"", idx);
        if (ImGui::Button("Use Only This GPU")) {
            copyToClipboard(cmd);
            showCopiedToast("CUDA_VISIBLE_DEVICES");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Open Terminal##only")) {
            openTerminalWithGpu(idx, displayName);
        }
        ImGui::SameLine();
        ImGui::TextDisabled("cuda:%u", stats.cudaIndex);
    }

    // Use TCC/Compute GPUs (show for TCC GPUs - typically NVLink capable)
    if (stats.isTCC) {
        std::string tccIndices = buildNvlinkPair(allStats);
        if (!tccIndices.empty() && tccIndices.find(',') != std::string::npos) {
            // Only show if there are multiple TCC GPUs
            std::string cmd = std::format("$env:CUDA_VISIBLE_DEVICES=\"{}\"", tccIndices);
            if (ImGui::Button("Use All TCC GPUs")) {
                copyToClipboard(cmd);
                showCopiedToast("TCC GPUs");
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("Open Terminal##tcc")) {
                openTerminalWithGpu(tccIndices, "TCC Compute GPUs");
            }
            ImGui::SameLine();
            ImGui::TextDisabled("cuda:%s", tccIndices.c_str());
        }
    }

    // Exclude this GPU
    {
        std::string otherIndices = buildExcludeDevices(allStats, stats.cudaIndex);
        std::string cmd = std::format("$env:CUDA_VISIBLE_DEVICES=\"{}\"", otherIndices);
        if (ImGui::Button("Exclude This GPU")) {
            copyToClipboard(cmd);
            showCopiedToast("Exclude GPU");
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("Open Terminal##exclude")) {
            openTerminalWithGpu(otherIndices, std::format("Excluding {}", displayName));
        }
        ImGui::SameLine();
        ImGui::TextDisabled("cuda:%s", otherIndices.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Quick Copy Section ===
    ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Quick Copy");
    ImGui::Spacing();

    if (ImGui::Button("Bus ID")) {
        copyToClipboard(stats.pciBusId);
        showCopiedToast("Bus ID");
    }
    ImGui::SameLine();

    if (ImGui::Button("CUDA Index")) {
        copyToClipboard(std::to_string(stats.cudaIndex));
        showCopiedToast("CUDA Index");
    }
    ImGui::SameLine();

    // nvidia-smi for this GPU
    {
        std::string cmd = std::format("nvidia-smi -i {}", stats.cudaIndex);
        if (ImGui::Button("nvidia-smi")) {
            copyToClipboard(cmd);
            showCopiedToast("nvidia-smi command");
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // === Management Section (with confirmations) ===
    ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.3f, 1.0f), "Management (Admin Required)");
    ImGui::Spacing();

    // Toggle TCC/WDDM
    {
        std::string targetMode = stats.isTCC ? "WDDM" : "TCC";
        int modeValue = stats.isTCC ? 0 : 1;
        std::string cmd = std::format("nvidia-smi -i {} -dm {}", stats.cudaIndex, modeValue);

        std::string btnLabel = std::format("Switch to {}", targetMode);
        if (ImGui::Button(btnLabel.c_str())) {
            m_confirmDialog.isOpen = true;
            m_confirmDialog.isDangerous = true;
            m_confirmDialog.title = "Toggle Driver Mode";
            m_confirmDialog.message = std::format(
                "This will switch GPU {} ({}) from {} to {} mode.\n\n"
                "A system restart is required for this change to take effect.",
                stats.cudaIndex, displayName, stats.isTCC ? "TCC" : "WDDM", targetMode);
            m_confirmDialog.command = cmd;
        }
    }

    ImGui::SameLine();

    // Reset GPU
    {
        std::string cmd = std::format("nvidia-smi -i {} --gpu-reset", stats.cudaIndex);
        if (ImGui::Button("Reset GPU")) {
            m_confirmDialog.isOpen = true;
            m_confirmDialog.isDangerous = true;
            m_confirmDialog.title = "Reset GPU";
            m_confirmDialog.message = std::format(
                "This will reset GPU {} ({}).\n\nAll running processes on this GPU will be terminated.",
                stats.cudaIndex, displayName);
            m_confirmDialog.command = cmd;
        }
    }

    // Power limit options
    ImGui::Text("Power Limit:");
    ImGui::SameLine();

    const unsigned int powerPresets[] = {200, 250, 300};
    for (int i = 0; i < 3; i++) {
        unsigned int watts = powerPresets[i];
        std::string label = std::format("{}W", watts);
        std::string cmd = std::format("nvidia-smi -i {} -pl {}", stats.cudaIndex, watts);

        if (i > 0) ImGui::SameLine();
        if (ImGui::SmallButton(label.c_str())) {
            m_confirmDialog.isOpen = true;
            m_confirmDialog.isDangerous = true;
            m_confirmDialog.title = "Set Power Limit";
            m_confirmDialog.message = std::format(
                "This will set the power limit for GPU {} ({}) to {}W.",
                stats.cudaIndex, displayName, watts);
            m_confirmDialog.command = cmd;
        }
    }

    // Kill processes on this GPU
    {
        // This uses a PowerShell one-liner to find and kill processes
        std::string cmd = std::format(
            "(nvidia-smi -i {} --query-compute-apps=pid --format=csv,noheader) | "
            "ForEach-Object {{ Stop-Process -Id $_ -Force }}", stats.cudaIndex);

        if (ImGui::Button("Kill All Processes")) {
            m_confirmDialog.isOpen = true;
            m_confirmDialog.isDangerous = true;
            m_confirmDialog.title = "Kill GPU Processes";
            m_confirmDialog.message = std::format(
                "This will forcefully terminate ALL processes running on GPU {} ({}).\n\n"
                "This may cause data loss in running applications!", stats.cudaIndex, displayName);
            m_confirmDialog.command = cmd;
        }
    }

    ImGui::Unindent(10);
}
