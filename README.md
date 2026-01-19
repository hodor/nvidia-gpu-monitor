# GPU Monitor

A cross-platform desktop application for real-time NVIDIA GPU monitoring using NVML (NVIDIA Management Library).

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg)

## Features

- **Real-time GPU Statistics**
  - VRAM usage and utilization
  - GPU and memory clock speeds
  - Temperature and fan speed
  - Power draw and limits
  - PCIe generation and lane width
  - ECC error counts (if supported)

- **Multi-GPU Support**
  - Monitor all NVIDIA GPUs simultaneously
  - Drag-and-drop GPU card reordering
  - Custom nicknames for each GPU
  - TCC/WDDM driver mode detection
  - NVLink connection status

- **Process Management**
  - View processes running on each GPU
  - Process memory usage
  - Kill processes directly from UI

- **Quick Launch**
  - Create presets with custom commands
  - Select specific GPUs per preset
  - Set `CUDA_VISIBLE_DEVICES` automatically
  - Open terminals with GPU environment configured

- **Cross-Platform**
  - Windows: DirectX 11 + Win32
  - Linux: OpenGL 3 + GLFW

## Screenshots

*Coming soon*

## Requirements

- **NVIDIA GPU** with driver 450.0 or later
- **CUDA Toolkit** (for NVML headers)
- **Windows**: Visual Studio 2022, Windows SDK
- **Linux**: GCC/Clang, GLFW3, OpenGL

## Build Instructions

### Windows

```powershell
# Clone the repository
git clone https://github.com/yourusername/gpu_monitor.git
cd gpu_monitor

# Generate build files
cmake -B build -G "Visual Studio 17 2022" -A x64

# Build release
cmake --build build --config Release

# Run
.\build\Release\gpu_monitor.exe
```

### Linux

```bash
# Install dependencies (Debian/Ubuntu)
sudo apt install libglfw3-dev libgl1-mesa-dev

# Clone the repository
git clone https://github.com/yourusername/gpu_monitor.git
cd gpu_monitor

# Generate and build
cmake -B build
cmake --build build

# Run
./build/gpu_monitor
```

### Custom CUDA Path

If CUDA is not auto-detected, specify the path manually:

```bash
cmake -B build -DCUDAToolkit_ROOT=/path/to/cuda
```

## Usage

Launch the application and it will automatically detect all NVIDIA GPUs. The UI displays:

- **System Health**: Driver version, CUDA version, NVLink status
- **GPU Cards**: One card per GPU with all statistics
- **Quick Launch**: Configure and run presets with specific GPU selections

### GPU Card Features

- Click the drag handle to reorder GPUs
- Expand settings to set a nickname
- View running processes and their memory usage
- Use command buttons to:
  - Copy Bus ID to clipboard
  - Open terminal with `CUDA_VISIBLE_DEVICES` set
  - Switch between TCC/WDDM modes (Windows, requires admin)

### Configuration

Settings are stored in:
- **Windows**: `%USERPROFILE%\.gpu_monitor\presets.json`
- **Linux**: `$HOME/.config/gpu_monitor/presets.json`

## Architecture

```
src/
├── main_win32.cpp        # Windows entry point, DirectX 11 setup
├── main_linux.cpp        # Linux entry point, GLFW/OpenGL setup
├── gpu_monitor.h/cpp     # NVML wrapper, background polling thread
├── ui.h/cpp              # Dear ImGui UI rendering
└── platform/
    ├── platform.h        # Cross-platform interface
    ├── platform_win32.cpp
    └── platform_linux.cpp
```

## Dependencies

- [Dear ImGui](https://github.com/ocornut/imgui) (bundled, docking branch)
- [NVML](https://developer.nvidia.com/nvidia-management-library-nvml) (part of CUDA Toolkit)
- DirectX 11 (Windows) / GLFW + OpenGL (Linux)

## License

MIT License - see [LICENSE](LICENSE) for details.

## Contributing

Contributions are welcome! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.
