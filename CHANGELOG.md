# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.1.2] - 2025-01-23

### Added

- FontAwesome icons throughout the UI
  - Section headers: Quick Launch, Commands, Processes
  - Buttons: Play, Edit, Clone, Trash, Terminal, Copy, etc.
  - Labels: GPU Monitor title, CUDA Device Selection, Quick Copy, Management
  - Warning indicators with triangle exclamation icon
- Embedded font in executable (no external font file needed)
- Multiline command input for Quick Launch presets
  - Line numbers displayed in gutter
  - Platform-specific normalization (PowerShell `;` / Bash `&&`)
  - JSON escaping for persistence
- Persistent collapse state for all UI sections
  - GPU cards remember expanded/collapsed state
  - Quick Launch section state persists
  - Per-GPU Processes and Commands sections persist
- Card-style layout for Quick Launch presets
- Duplicate button for presets
- Custom colors for Quick Launch presets (button and card background)

### Changed

- CollapsingHeader styling to subtle dark theme for visual consistency
- Quick Launch preset buttons now use icons instead of text
- All collapsible sections start collapsed by default

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

[Unreleased]: https://github.com/yourusername/gpu_monitor/compare/v0.1.2...HEAD
[0.1.2]: https://github.com/yourusername/gpu_monitor/compare/v0.1.0...v0.1.2
[0.1.0]: https://github.com/yourusername/gpu_monitor/releases/tag/v0.1.0
