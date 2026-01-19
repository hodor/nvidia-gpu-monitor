# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.0] - 2025-01-18

### Added

- Initial release
- Real-time GPU monitoring via NVML
  - VRAM usage and utilization
  - GPU/memory clock speeds
  - Temperature and fan speed
  - Power draw and limits
  - PCIe generation and lane width
  - ECC error counts
- Multi-GPU support
  - Drag-and-drop card reordering
  - Custom GPU nicknames
  - TCC/WDDM mode detection
  - NVLink status display
- Process management
  - View processes per GPU
  - Process memory usage
  - Kill processes from UI
- Quick Launch presets
  - Custom commands with GPU selection
  - Automatic CUDA_VISIBLE_DEVICES configuration
- Cross-platform support
  - Windows: DirectX 11 + Win32
  - Linux: OpenGL 3 + GLFW
- Settings persistence (JSON format)
- Dear ImGui-based UI (docking branch)

[Unreleased]: https://github.com/yourusername/gpu_monitor/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/yourusername/gpu_monitor/releases/tag/v0.1.0
