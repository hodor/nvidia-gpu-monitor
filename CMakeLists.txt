cmake_minimum_required(VERSION 3.20)
project(gpu_monitor VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================================
# CUDA Toolkit / NVML Detection
# ============================================================================

# Try CMake's built-in CUDA detection first
find_package(CUDAToolkit QUIET)

if(CUDAToolkit_FOUND)
    message(STATUS "Found CUDA Toolkit: ${CUDAToolkit_VERSION}")
    set(NVML_INCLUDE_DIR "${CUDAToolkit_INCLUDE_DIRS}")
    if(WIN32)
        set(NVML_LIBRARY "${CUDAToolkit_LIBRARY_DIR}/nvml.lib")
    else()
        # Linux: prefer stub library for linking (works on CI without GPU)
        find_library(NVML_LIBRARY
            NAMES nvidia-ml
            PATHS "${CUDAToolkit_LIBRARY_DIR}/stubs" "${CUDAToolkit_LIBRARY_DIR}"
            NO_DEFAULT_PATH
        )
        if(NOT NVML_LIBRARY)
            set(NVML_LIBRARY "nvidia-ml")  # Fall back to system
        endif()
    endif()
else()
    if(WIN32)
        # Fallback: Search common Windows CUDA installation paths
        file(GLOB CUDA_SEARCH_PATHS "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v*")
        list(SORT CUDA_SEARCH_PATHS ORDER DESCENDING)
        if(CUDA_SEARCH_PATHS)
            list(GET CUDA_SEARCH_PATHS 0 CUDA_PATH)
            message(STATUS "Found CUDA Toolkit (fallback): ${CUDA_PATH}")
            set(NVML_INCLUDE_DIR "${CUDA_PATH}/include")
            set(NVML_LIBRARY "${CUDA_PATH}/lib/x64/nvml.lib")
        else()
            message(FATAL_ERROR "CUDA Toolkit not found. Install CUDA or set CUDAToolkit_ROOT.")
        endif()
    else()
        # Linux fallback - search standard paths
        find_path(NVML_INCLUDE_DIR nvml.h PATHS /usr/include /usr/local/cuda/include)
        find_library(NVML_LIBRARY
            NAMES nvidia-ml
            PATHS /usr/local/cuda/lib64/stubs /usr/local/cuda/lib64 /usr/lib64 /usr/lib
        )
        if(NOT NVML_LIBRARY)
            set(NVML_LIBRARY "nvidia-ml")  # Fall back to system
        endif()
        if(NOT NVML_INCLUDE_DIR)
            message(FATAL_ERROR "CUDA/NVML headers not found. Install CUDA Toolkit or nvidia-driver packages.")
        endif()
    endif()
endif()

# ============================================================================
# Dear ImGui sources (common)
# ============================================================================

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
)

# ============================================================================
# Platform-specific configuration
# ============================================================================

if(WIN32)
    # Windows: DirectX 11 + Win32
    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
        ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
    )

    set(APP_SOURCES
        src/main_win32.cpp
        src/gpu_monitor.cpp
        src/ui.cpp
        src/platform/platform_win32.cpp
    )

    add_executable(${PROJECT_NAME} WIN32 ${APP_SOURCES} ${IMGUI_SOURCES})

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${NVML_INCLUDE_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        d3d11
        dxgi
        d3dcompiler
        shell32
        ole32
        ${NVML_LIBRARY}
    )

    # MSVC-specific settings
    if(MSVC)
        set_target_properties(${PROJECT_NAME} PROPERTIES
            LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
            LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
        )
        target_compile_options(${PROJECT_NAME} PRIVATE /W3 /Zc:__cplusplus)
    endif()

else()
    # Linux: GLFW + OpenGL3
    find_package(glfw3 REQUIRED)
    find_package(OpenGL REQUIRED)

    list(APPEND IMGUI_SOURCES
        ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
        ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    )

    set(APP_SOURCES
        src/main_linux.cpp
        src/gpu_monitor.cpp
        src/ui.cpp
        src/platform/platform_linux.cpp
    )

    add_executable(${PROJECT_NAME} ${APP_SOURCES} ${IMGUI_SOURCES})

    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_SOURCE_DIR}/src
        ${IMGUI_DIR}
        ${IMGUI_DIR}/backends
        ${NVML_INCLUDE_DIR}
    )

    target_link_libraries(${PROJECT_NAME} PRIVATE
        glfw
        OpenGL::GL
        ${NVML_LIBRARY}
    )

    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
endif()
