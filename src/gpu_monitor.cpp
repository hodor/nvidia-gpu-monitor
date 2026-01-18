#include "gpu_monitor.h"
#include <nvml.h>
#include <algorithm>
#include <chrono>
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>

GpuMonitor::GpuMonitor() = default;

GpuMonitor::~GpuMonitor() {
    shutdown();
}

bool GpuMonitor::initialize() {
    if (m_initialized) return true;

    nvmlReturn_t result = nvmlInit();
    if (result != NVML_SUCCESS) {
        return false;
    }

    m_initialized = true;
    updateStats();      // Initial poll
    updateSystemInfo(); // Initial system info
    return true;
}

void GpuMonitor::shutdown() {
    stopPolling();
    if (m_initialized) {
        nvmlShutdown();
        m_initialized = false;
    }
}

std::string GpuMonitor::getProcessName(unsigned int pid) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (hProcess == nullptr) {
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

void GpuMonitor::updateStats() {
    if (!m_initialized) return;

    unsigned int deviceCount = 0;
    nvmlReturn_t result = nvmlDeviceGetCount(&deviceCount);
    if (result != NVML_SUCCESS) return;

    std::vector<GpuStats> newStats;
    newStats.reserve(deviceCount);

    for (unsigned int i = 0; i < deviceCount; i++) {
        nvmlDevice_t device;
        result = nvmlDeviceGetHandleByIndex(i, &device);
        if (result != NVML_SUCCESS) continue;

        GpuStats stats{};
        stats.cudaIndex = i;  // NVML index matches CUDA index

        // Name
        char name[NVML_DEVICE_NAME_BUFFER_SIZE];
        if (nvmlDeviceGetName(device, name, sizeof(name)) == NVML_SUCCESS) {
            stats.name = name;
        }

        // UUID (unique identifier for this specific GPU)
        char uuid[NVML_DEVICE_UUID_BUFFER_SIZE];
        if (nvmlDeviceGetUUID(device, uuid, sizeof(uuid)) == NVML_SUCCESS) {
            stats.uuid = uuid;
        }

        // PCI Bus ID (physical slot location)
        nvmlPciInfo_t pci;
        if (nvmlDeviceGetPciInfo(device, &pci) == NVML_SUCCESS) {
            stats.pciBusId = pci.busId;
        }

        // Driver model (TCC vs WDDM)
        // NVML_DRIVER_WDDM = 0 (display), NVML_DRIVER_WDM = 1 (TCC/compute)
        nvmlDriverModel_t current, pending;
        if (nvmlDeviceGetDriverModel(device, &current, &pending) == NVML_SUCCESS) {
            stats.isTCC = (current == NVML_DRIVER_WDM);
        }

        // Memory
        nvmlMemory_t memory;
        if (nvmlDeviceGetMemoryInfo(device, &memory) == NVML_SUCCESS) {
            stats.vramUsed = memory.used;
            stats.vramTotal = memory.total;
        }

        // Utilization
        nvmlUtilization_t utilization;
        if (nvmlDeviceGetUtilizationRates(device, &utilization) == NVML_SUCCESS) {
            stats.gpuUtilization = utilization.gpu;
            stats.memUtilization = utilization.memory;
        }

        // Temperature
        unsigned int temp;
        if (nvmlDeviceGetTemperature(device, NVML_TEMPERATURE_GPU, &temp) == NVML_SUCCESS) {
            stats.temperature = temp;
        }

        // Fan speed (may not be available on all GPUs)
        unsigned int fan;
        if (nvmlDeviceGetFanSpeed(device, &fan) == NVML_SUCCESS) {
            stats.fanSpeed = fan;
        }

        // Power
        unsigned int power;
        if (nvmlDeviceGetPowerUsage(device, &power) == NVML_SUCCESS) {
            stats.powerDraw = power / 1000;  // Convert mW to W
        }
        unsigned int limit;
        if (nvmlDeviceGetPowerManagementLimit(device, &limit) == NVML_SUCCESS) {
            stats.powerLimit = limit / 1000;  // Convert mW to W
        }

        // Clocks (current and max)
        unsigned int clock;
        if (nvmlDeviceGetClockInfo(device, NVML_CLOCK_GRAPHICS, &clock) == NVML_SUCCESS) {
            stats.gpuClock = clock;
        }
        if (nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_GRAPHICS, &clock) == NVML_SUCCESS) {
            stats.gpuClockMax = clock;
        }
        if (nvmlDeviceGetClockInfo(device, NVML_CLOCK_MEM, &clock) == NVML_SUCCESS) {
            stats.memClock = clock;
        }
        if (nvmlDeviceGetMaxClockInfo(device, NVML_CLOCK_MEM, &clock) == NVML_SUCCESS) {
            stats.memClockMax = clock;
        }

        // PCIe info
        unsigned int gen, width;
        if (nvmlDeviceGetCurrPcieLinkGeneration(device, &gen) == NVML_SUCCESS) {
            stats.pcieGen = gen;
        }
        if (nvmlDeviceGetCurrPcieLinkWidth(device, &width) == NVML_SUCCESS) {
            stats.pcieWidth = width;
        }

        // Running processes
        nvmlProcessInfo_t processInfos[32];
        unsigned int processCount = 32;
        if (nvmlDeviceGetComputeRunningProcesses(device, &processCount, processInfos) == NVML_SUCCESS) {
            for (unsigned int p = 0; p < processCount; p++) {
                GpuProcess proc;
                proc.pid = processInfos[p].pid;
                proc.usedMemory = processInfos[p].usedGpuMemory;
                proc.name = getProcessName(proc.pid);
                stats.processes.push_back(proc);
            }
        }
        // Also get graphics processes (for WDDM mode)
        processCount = 32;
        if (nvmlDeviceGetGraphicsRunningProcesses(device, &processCount, processInfos) == NVML_SUCCESS) {
            for (unsigned int p = 0; p < processCount; p++) {
                // Avoid duplicates
                bool found = false;
                for (const auto& existing : stats.processes) {
                    if (existing.pid == processInfos[p].pid) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    GpuProcess proc;
                    proc.pid = processInfos[p].pid;
                    proc.usedMemory = processInfos[p].usedGpuMemory;
                    proc.name = getProcessName(proc.pid);
                    stats.processes.push_back(proc);
                }
            }
        }

        // ECC errors
        stats.eccSupported = false;
        stats.eccErrors = 0;
        nvmlEnableState_t eccMode;
        if (nvmlDeviceGetEccMode(device, &eccMode, nullptr) == NVML_SUCCESS) {
            stats.eccSupported = true;
            if (eccMode == NVML_FEATURE_ENABLED) {
                unsigned long long eccCount;
                if (nvmlDeviceGetTotalEccErrors(device, NVML_MEMORY_ERROR_TYPE_CORRECTED,
                    NVML_VOLATILE_ECC, &eccCount) == NVML_SUCCESS) {
                    stats.eccErrors = eccCount;
                }
            }
        }

        newStats.push_back(stats);
    }

    // Sort by PCI bus ID (matches physical slot order when looking at hardware)
    std::sort(newStats.begin(), newStats.end(),
              [](const GpuStats& a, const GpuStats& b) {
                  return a.pciBusId < b.pciBusId;
              });

    // Update shared stats
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stats = std::move(newStats);
    }
}

std::vector<GpuStats> GpuMonitor::getStats() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stats;
}

SystemInfo GpuMonitor::getSystemInfo() {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_systemInfo;
}

void GpuMonitor::updateSystemInfo() {
    if (!m_initialized) return;

    SystemInfo info;

    // Driver version
    char driverVersion[NVML_SYSTEM_DRIVER_VERSION_BUFFER_SIZE];
    if (nvmlSystemGetDriverVersion(driverVersion, sizeof(driverVersion)) == NVML_SUCCESS) {
        info.driverVersion = driverVersion;
    }

    // CUDA version (NVML reports as int like 12090 for 12.9)
    int cudaVersion;
    if (nvmlSystemGetCudaDriverVersion(&cudaVersion) == NVML_SUCCESS) {
        int major = cudaVersion / 1000;
        int minor = (cudaVersion % 1000) / 10;
        info.cudaVersion = std::to_string(major) + "." + std::to_string(minor);
    }

    // NVLink status - check connections between GPUs
    info.nvlinkAvailable = false;
    unsigned int deviceCount = 0;
    if (nvmlDeviceGetCount(&deviceCount) == NVML_SUCCESS) {
        for (unsigned int i = 0; i < deviceCount; i++) {
            nvmlDevice_t device;
            if (nvmlDeviceGetHandleByIndex(i, &device) != NVML_SUCCESS) continue;

            // Check each NVLink link (up to 6 links per GPU)
            for (unsigned int link = 0; link < 6; link++) {
                nvmlEnableState_t isActive;
                if (nvmlDeviceGetNvLinkState(device, link, &isActive) == NVML_SUCCESS) {
                    if (isActive == NVML_FEATURE_ENABLED) {
                        info.nvlinkAvailable = true;

                        // Get remote GPU info
                        nvmlPciInfo_t remotePci;
                        if (nvmlDeviceGetNvLinkRemotePciInfo(device, link, &remotePci) == NVML_SUCCESS) {
                            // Find which GPU index this connects to
                            for (unsigned int j = 0; j < deviceCount; j++) {
                                if (j == i) continue;
                                nvmlDevice_t otherDevice;
                                if (nvmlDeviceGetHandleByIndex(j, &otherDevice) != NVML_SUCCESS) continue;
                                nvmlPciInfo_t otherPci;
                                if (nvmlDeviceGetPciInfo(otherDevice, &otherPci) == NVML_SUCCESS) {
                                    if (strcmp(remotePci.busId, otherPci.busId) == 0) {
                                        // Found a connection
                                        auto pair = std::make_pair(std::min(i, j), std::max(i, j));
                                        // Avoid duplicates
                                        bool found = false;
                                        for (const auto& existing : info.nvlinkPairs) {
                                            if (existing == pair) {
                                                found = true;
                                                break;
                                            }
                                        }
                                        if (!found) {
                                            info.nvlinkPairs.push_back(pair);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    m_systemInfo = info;
}

void GpuMonitor::startPolling(int intervalMs) {
    if (m_running) return;

    m_pollIntervalMs = intervalMs;
    m_running = true;
    m_pollThread = std::thread(&GpuMonitor::pollThread, this);
}

void GpuMonitor::stopPolling() {
    if (!m_running) return;

    m_running = false;
    if (m_pollThread.joinable()) {
        m_pollThread.join();
    }
}

void GpuMonitor::pollThread() {
    while (m_running) {
        updateStats();

        // Sleep in small increments to allow quick shutdown
        int slept = 0;
        while (slept < m_pollIntervalMs && m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            slept += 100;
        }
    }
}
