# Dependencies

## Core Libraries
- Windows API (windows.h)
- C++ Standard Library (STL)
  - mutex (thread safety)
  - chrono (rate limiting, timestamps)
  - fstream (file logging)
  - sstream (string formatting)
  - vector (data structures)
  - string (string handling)
  - iomanip (log formatting)

## Graphics Libraries
- GLFW 3.3+
- GLAD
- Dear ImGui

## External Dependencies
- nlohmann/json (configuration handling)

## System Requirements
- Windows 10 or later
- Visual C++ Runtime
- OpenGL 3.3+ capable GPU
- Administrator privileges (for memory reading)

## Development Requirements
- CMake 3.10+
- Visual Studio 2019+ or compatible compiler
- C++17 or later

## Memory Access Requirements
- PROCESS_VM_READ permission
- PROCESS_QUERY_INFORMATION permission
# Dependencies

## Core Libraries

### GLFW (3.3.8)
- Purpose: Window management and OpenGL context creation
- Source: https://github.com/glfw/glfw
- License: zlib/libpng
- Integration: FetchContent in CMake
- Usage: Window creation, input handling, OpenGL context

### Dear ImGui (1.89.9)
- Purpose: User interface rendering
- Source: https://github.com/ocornut/imgui
- License: MIT
- Integration: FetchContent in CMake
- Usage: Overlay UI, debug information, settings interface

### GLAD
- Purpose: OpenGL function loading
- Source: https://github.com/Dav1dde/glad
- License: MIT
- Integration: Direct source inclusion
- Usage: OpenGL function pointer management

### nlohmann/json (3.11.2)
- Purpose: JSON parsing and serialization
- Source: https://github.com/nlohmann/json
- License: MIT
- Integration: FetchContent in CMake
- Usage: Configuration file handling

## System Dependencies

### OpenGL
- Version: 3.3 Core Profile
- Purpose: Graphics rendering
- Integration: System library
- Usage: Rendering backend

### Windows API
- Components:
  - Shell32: Process launching
  - User32: Window management
  - Kernel32: Process management
  - Psapi: Process memory information
  - DbgHelp: Debug helpers (optional)
- Integration: System library
- Usage:
  - Process detection
  - Window management
  - Memory reading
  - Process information
  - Debug support

### Windows SDK
- Components:
  - Windows.h
  - TlHelp32.h
  - Psapi.h
  - DbgHelp.h
- Purpose: System API access
- Usage: Process and memory management

## Build System

### CMake (3.10+)
- Purpose: Build configuration and management
- Integration: Primary build system
- Usage: Project configuration, dependency management

### C++ Standard Library
- Version: C++17
- Purpose: Core functionality
- Integration: Language standard
- Usage: Throughout codebase

## Development Dependencies

### Visual Studio Build Tools
- Purpose: Compilation and linking
- Components:
  - MSVC Compiler
  - Windows SDK
  - C++ Core Features
  - Debugging Tools

### Git
- Purpose: Version control
- Usage: Dependency fetching, source control

## Dependency Graph
```
RobloxESP
├── GLFW 3.3.8
│   └── OpenGL 3.3
├── Dear ImGui 1.89.9
│   ├── GLFW (backend)
│   └── OpenGL (backend)
├── GLAD
│   └── OpenGL 3.3
├── nlohmann/json 3.11.2
└── Windows API
    ├── Shell32
    ├── User32
    ├── Kernel32
    ├── Psapi
    └── DbgHelp
```

## Version Management
- All dependencies are version-locked in CMakeLists.txt
- GLFW, ImGui, and JSON libraries are fetched from specific Git tags
- GLAD is included as direct source
- System dependencies are runtime requirements

## License Compliance
All dependencies are used under permissive licenses:
- GLFW: zlib/libpng
- Dear ImGui: MIT
- GLAD: MIT
- nlohmann/json: MIT
- Windows API: Proprietary (Microsoft)

## Configuration Files
The project uses JSON for configuration storage:
- config.json: User settings and preferences
  - ESP settings
  - Visual customization
  - Hotkey configuration
  - Performance settings

## Runtime Requirements
- Administrator privileges (for memory reading)
- Windows 10 or later
- Visual C++ Redistributable 2019+
- OpenGL 3.3 capable GPU
- DirectX 11 compatible system