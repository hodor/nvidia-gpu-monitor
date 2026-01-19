# Contributing to GPU Monitor

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Development Setup

### Prerequisites

- **Windows**: Visual Studio 2022 with C++ workload, CUDA Toolkit
- **Linux**: GCC 11+ or Clang 14+, GLFW3, OpenGL, CUDA Toolkit
- Git

### Building from Source

```bash
# Clone with submodules (if any)
git clone https://github.com/yourusername/gpu_monitor.git
cd gpu_monitor

# Windows
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Debug

# Linux
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Running Tests

*Tests are planned for a future release.*

## Code Style

### C++ Guidelines

- **Standard**: C++20
- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style (opening brace on same line)
- **Naming**:
  - Classes: `PascalCase`
  - Functions: `camelCase`
  - Member variables: `m_camelCase`
  - Constants: `UPPER_SNAKE_CASE`
- **Line length**: 100 characters max (soft limit)

### Example

```cpp
class GpuMonitor {
public:
    GpuMonitor();
    void startPolling(int intervalMs = 1000);

private:
    void pollThread(std::stop_token stopToken);

    std::vector<GpuStats> m_stats;
    std::mutex m_mutex;
    bool m_initialized{false};
};
```

### Formatting

An `.editorconfig` file is provided. Most editors will pick this up automatically.

## Pull Request Process

1. **Fork** the repository and create a feature branch from `main`
2. **Make changes** following the code style guidelines
3. **Test** your changes on at least one platform
4. **Commit** with clear, descriptive messages
5. **Push** to your fork and open a Pull Request

### Commit Messages

Use clear, imperative-mood messages:

```
Add NVLink bandwidth monitoring
Fix memory leak in GPU polling thread
Update ImGui to latest docking branch
```

### PR Guidelines

- Keep PRs focused on a single feature or fix
- Update documentation if adding new features
- Add yourself to CONTRIBUTORS if this is your first contribution
- Respond to review feedback promptly

## Reporting Issues

### Bug Reports

Please include:

- OS and version (Windows 11, Ubuntu 22.04, etc.)
- GPU model and driver version
- Steps to reproduce the issue
- Expected vs actual behavior
- Relevant log output or screenshots

### Feature Requests

- Check existing issues first to avoid duplicates
- Describe the use case and why it would be valuable
- Consider if it fits the project's scope

## Architecture Overview

Understanding the codebase structure helps when contributing:

| Component | Description |
|-----------|-------------|
| `gpu_monitor.cpp` | NVML wrapper, polls GPU stats on background thread |
| `ui.cpp` | Dear ImGui rendering, all UI logic |
| `platform/` | Platform-specific code (clipboard, terminals, etc.) |
| `main_*.cpp` | Window creation and main loop per platform |

### Adding Features

- **New GPU metrics**: Add to `GpuStats` struct, update `updateStats()` in `gpu_monitor.cpp`, render in `ui.cpp`
- **New UI elements**: Add to `ui.cpp`, follow existing patterns for cards and sections
- **Platform code**: Add to both `platform_win32.cpp` and `platform_linux.cpp`

## License

By contributing, you agree that your contributions will be licensed under the MIT License.
