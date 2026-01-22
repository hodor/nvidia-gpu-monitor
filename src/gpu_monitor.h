#pragma once

#include <map>
#include <mutex>
#include <stop_token>
#include <string>
#include <thread>
#include <vector>

// Process running on a GPU
struct GpuProcess {
    unsigned int pid;
    std::string name;
    unsigned long long usedMemory;  // bytes
    // Note: per-process GPU utilization not available via NVML
};

// System-wide GPU info
struct SystemInfo {
    std::string driverVersion;
    std::string cudaVersion;
    bool nvlinkAvailable;
    std::vector<std::pair<int, int>> nvlinkPairs;  // pairs of connected GPU indices
};

struct GpuStats {
    std::string name;
    std::string uuid;          // Unique GPU identifier (for settings key)
    std::string pciBusId;      // Physical slot (for default sort order)
    bool isTCC;
    unsigned int cudaIndex;    // CUDA device index

    // Memory
    unsigned long long vramUsed;   // bytes
    unsigned long long vramTotal;  // bytes

    // Utilization
    unsigned int gpuUtilization;   // 0-100%
    unsigned int memUtilization;   // 0-100%

    // Thermals & Power
    unsigned int temperature;      // Celsius
    unsigned int fanSpeed;         // 0-100%
    unsigned int powerDraw;        // Watts
    unsigned int powerLimit;       // Watts

    // Clocks
    unsigned int gpuClock;         // MHz (current)
    unsigned int gpuClockMax;      // MHz (max)
    unsigned int memClock;         // MHz (current)
    unsigned int memClockMax;      // MHz (max)

    // PCIe
    unsigned int pcieGen;          // 1-4
    unsigned int pcieWidth;        // lanes (e.g., 16)

    // Processes
    std::vector<GpuProcess> processes;

    // ECC Errors
    unsigned long long eccErrors;  // total correctable errors
    bool eccSupported;
};

class GpuMonitor {
public:
    GpuMonitor();
    ~GpuMonitor();

    bool initialize();
    void shutdown();

    // Get a copy of current GPU stats (thread-safe)
    std::vector<GpuStats> getStats();

    // Get system-wide info (driver, CUDA version, NVLink)
    SystemInfo getSystemInfo();

    // Start/stop background polling
    void startPolling(int intervalMs = 1000);
    void stopPolling();

private:
    void pollThread(std::stop_token stopToken);
    void updateStats();
    void updateSystemInfo();
    std::string getProcessName(unsigned int pid, bool forceRefresh = false);

    std::vector<GpuStats> m_stats;
    SystemInfo m_systemInfo;
    std::mutex m_mutex;
    std::jthread m_pollThread;
    int m_pollIntervalMs{1000};
    bool m_initialized{false};

    // Process name caching - only refresh names every N polls to reduce syscalls
    int m_processNameUpdateInterval{5};  // Refresh process names every N polls
    int m_pollsSinceProcessNameUpdate{0};
    std::map<unsigned int, std::string> m_processNameCache;  // PID -> name
};
