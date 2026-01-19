# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

GPU Monitor is a cross-platform desktop application that displays real-time NVIDIA GPU statistics using NVML (NVIDIA Management Library). It features a Dear ImGui-based UI with platform-specific backends:
- **Windows**: DirectX 11 + Win32
- **Linux**: OpenGL3 + GLFW

## Build Commands

### Windows
```powershell
# Generate build files (from project root)
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build release
cmake --build build --config Release

# Build debug
cmake --build build --config Debug

# Output executable
build\Release\gpu_monitor.exe
```

### Linux
```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install libglfw3-dev libgl1-mesa-dev

# Generate build files
cmake -B build

# Build
cmake --build build

# Output executable
./build/gpu_monitor
```

## Dependencies

- **CUDA Toolkit** - Required for NVML headers and library (auto-detected)
- **Dear ImGui** - Bundled in `external/imgui/` (docking branch)
- **Windows**: DirectX 11 (Windows SDK), Visual Studio 2022
- **Linux**: GLFW3, OpenGL3, GCC/Clang

## Architecture

```
src/
├── main_win32.cpp        # Win32 window creation, DirectX 11 setup, main loop
├── main_linux.cpp        # GLFW window creation, OpenGL3 setup, main loop
├── gpu_monitor.h/cpp     # NVML wrapper - polls GPU stats on background thread
├── ui.h/cpp              # ImGui rendering - GPU cards, settings, commands
└── platform/
    ├── platform.h        # Cross-platform interface
    ├── platform_win32.cpp  # Windows implementations
    └── platform_linux.cpp  # Linux implementations
```

### Key Components

**GpuMonitor** (`gpu_monitor.cpp`)
- Initializes NVML and polls GPU data at configurable intervals
- Thread-safe stat access via mutex
- Collects: VRAM, utilization, temps, power, clocks, PCIe info, running processes, ECC errors
- Detects TCC vs WDDM driver mode
- Enumerates NVLink connections

**GpuMonitorUI** (`ui.cpp`)
- Renders GPU cards with progress bars, collapsible sections
- Per-GPU config stored by UUID (nicknames, display order)
- Quick Launch presets with GPU selection
- Drag-and-drop GPU reordering
- Command buttons: set CUDA_VISIBLE_DEVICES, open terminal, copy Bus ID, manage power/modes
- Settings persisted to platform-specific location:
  - Windows: `%USERPROFILE%\.gpu_monitor\presets.json`
  - Linux: `$HOME/.config/gpu_monitor/presets.json`

**Platform Abstraction** (`platform/`)
- `getSettingsDirectory()` - Platform-specific settings location
- `copyToClipboard()` - System clipboard integration
- `openTerminalWithEnv()` - Launch terminal with environment variable
- `killProcess()` - Process termination
- `getProcessName()` - Process name lookup
- `browseForFolder()` - Folder picker dialog

### NVML Integration

NVML functions are called directly (not via wrapper library). Key patterns:
- `nvmlInit()` / `nvmlShutdown()` for lifecycle
- `nvmlDeviceGetHandleByIndex()` then query functions
- `NVML_DRIVER_WDM` = TCC mode, `NVML_DRIVER_WDDM` = display mode
- Process enumeration via `nvmlDeviceGetComputeRunningProcesses()` and `nvmlDeviceGetGraphicsRunningProcesses()`

## Configuration

CUDA Toolkit is auto-detected. The build system:
1. Tries CMake's built-in `find_package(CUDAToolkit)`
2. Falls back to searching common installation paths
3. On Linux, also checks system paths for nvidia-ml

To specify a custom CUDA path:
```bash
cmake -B build -DCUDAToolkit_ROOT=/path/to/cuda
```

## Contributing

When making changes to this project:

### Code Style
- C++20 standard
- 4-space indentation (no tabs)
- K&R brace style (opening brace on same line)
- Naming: `PascalCase` for classes, `camelCase` for functions, `m_camelCase` for members

### Adding Features
- **New GPU metrics**: Add to `GpuStats` struct, update `updateStats()` in `gpu_monitor.cpp`, render in `ui.cpp`
- **New UI elements**: Add to `ui.cpp`, follow existing card/section patterns
- **Platform code**: Implement in both `platform_win32.cpp` and `platform_linux.cpp`

### Before Committing to Main
1. **Update CHANGELOG.md** - Add your changes under `[Unreleased]` section using Keep a Changelog format:
   - `Added` for new features
   - `Changed` for changes in existing functionality
   - `Fixed` for bug fixes
   - `Removed` for removed features
2. Verify the build works: `cmake -B build && cmake --build build`
3. Test on at least one platform

### Commit Messages
Use clear, imperative-mood messages:
```
Add NVLink bandwidth monitoring
Fix memory leak in GPU polling thread
Update ImGui to latest docking branch
```

See `CONTRIBUTING.md` for full guidelines.
