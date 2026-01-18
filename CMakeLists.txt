cmake_minimum_required(VERSION 3.20)
project(gpu_monitor LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CUDA Toolkit path for NVML
set(CUDA_TOOLKIT_ROOT "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.9")

# Dear ImGui sources
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)

# Application sources
set(APP_SOURCES
    src/main.cpp
    src/gpu_monitor.cpp
    src/ui.cpp
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${IMGUI_SOURCES})

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
    "${CUDA_TOOLKIT_ROOT}/include"
)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    d3d11
    dxgi
    d3dcompiler
    shell32
    ole32
    "${CUDA_TOOLKIT_ROOT}/lib/x64/nvml.lib"
)

# Windows-specific settings
if(MSVC)
    # Disable console window for Release builds
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
    )
    # Enable warnings and correct __cplusplus macro
    target_compile_options(${PROJECT_NAME} PRIVATE /W3 /Zc:__cplusplus)
endif()
