# Environment Settings

## Logging Configuration
- Log File: roblox_esp.log
- Log Format: [YYYY-MM-DD HH:MM:SS] Message
- Log Levels: INFO, ERROR
- Console Output: Enabled
- File Output: Enabled
- Timestamp Format: ISO 8601
- Log Rotation: None (append mode)

## Rate Limiting Settings
- Memory Read Rate Limit: 3 attempts per 100ms
- Pattern Scan Rate Limit: 3 attempts per 100ms
- Memory Region Scan Limit: 1000 regions
- Pattern Match Limit: 20 matches
- Operation Cooldown: 50ms between attempts
- Auto Reset: After cooldown period

## Memory Access Settings
- Read Buffer Size: 32 bytes (default)
- String Length Limit: 32 characters
- Memory Region Size Limit: 1MB per scan
- Pattern Size Limit: 16 bytes
- Access Validation: Enabled
- Protection Check: Enabled

## Error Handling
- Error Logging: Enabled
- Error Details: Full stack trace
- Error Recovery: Automatic retry with cooldown
- Error Limits: 3 attempts before cooldown
- Error Context: Full operation details
- Debug Information: Memory addresses and error codes

## Performance Settings
- Max Scan Regions: 1000
- Max Pattern Matches: 20
- Thread Safety: Enabled
- Buffer Size: Dynamic
- Operation Timeout: None
- Memory Cache: Disabled
# Environment Settings

## Development Environment

### Build System
- CMake 3.10 or higher
- C++17 compliant compiler
- Visual Studio 2019 or higher (Windows)

### Required Tools
- Git for dependency management
- PowerShell for build scripts
- Visual Studio Build Tools
- Windows SDK 10.0 or higher
- Debugging Tools for Windows

### Dependencies
- GLFW 3.3.8
- Dear ImGui 1.89.9
- GLAD (OpenGL 3.3)
- nlohmann/json 3.11.2
- Windows SDK components
  - Process API
  - Debug API
  - Memory API

## Runtime Environment

### System Requirements
- Operating System: Windows 10
- Graphics: OpenGL 3.3 capable GPU
- RAM: 4GB minimum (2GB for application)
- Storage: 100MB free space
- Display: 1280x720 minimum resolution
- CPU: Dual-core processor or better
- Administrator privileges
- DirectX 11 support

### Required Software
- Roblox Player
- Visual C++ Redistributable 2019 or later
- OpenGL 3.3 compatible graphics drivers
- Administrator account

### Security Requirements
- Administrator privileges for memory reading
- Windows UAC configuration
- Debug privileges
- Process access rights

### Configuration Files
- config.json: User settings and preferences
  - Location: Application directory
  - Format: JSON
  - Auto-created if missing
  - Requires write permissions

### Environment Variables
None required

## Build Configuration

### CMake Options
```cmake
cmake_minimum_required(VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
```

### Compiler Flags
- /W4 (Warning Level 4)
- /EHsc (Exception Handling)
- /MT (Static Runtime Library)
- /O2 (Release Optimization)
- /SUBSYSTEM:WINDOWS
- /MANIFESTUAC:level='requireAdministrator'

### Build Modes
- Debug: Full debug information, no optimization
  - Debug logging enabled
  - Memory validation
  - Address sanitizer
  - Performance monitoring
  - ImGui demo window

- Release: Maximum optimization, no debug info
  - Minimal logging
  - Optimized performance
  - Reduced binary size
  - Security features enabled

- RelWithDebInfo: Optimization with debug info
  - Balance of performance and debugging
  - Moderate logging
  - Memory tracking
  - Performance monitoring

## Development Setup Instructions

1. Install Required Software:
   ```powershell
   # Install Visual Studio Build Tools with Windows SDK
   # Install Git
   # Install CMake
   # Install Debugging Tools for Windows
   ```

2. Configure System:
   ```powershell
   # Enable Developer Mode
   # Configure Debug Privileges
   # Set up Administrator Access
   ```

3. Clone Repository:
   ```powershell
   git clone <repository-url>
   cd rbv0
   ```

4. Build Project:
   ```powershell
   mkdir build
   cd build
   cmake ..
   cmake --build . --config Release
   ```

5. Run Application (as Administrator):
   ```powershell
   Start-Process .\build\Release\roblox_esp.exe -Verb RunAs
   ```

## Configuration File Structure

```json
{
  "showESP": true,
  "showBoxes": true,
  "showDistance": true,
  "showHealth": true,
  "espColor": {
    "r": 1.0,
    "g": 0.0,
    "b": 0.0,
    "a": 1.0
  },
  "boxThickness": 2.0,
  "maxDistance": 1000,
  "toggleKey": 290,
  "settingsKey": 291
}
```

## Troubleshooting

### Common Issues
1. Access Denied
   - Solution: Run as Administrator
   - Check: UAC settings and privileges

2. Memory Reading Errors
   - Solution: Verify Roblox version
   - Solution: Update memory offsets
   - Check: Process access rights

3. OpenGL version mismatch
   - Solution: Update graphics drivers
   - Check: OpenGL version support

4. Missing Visual C++ Redistributable
   - Solution: Install latest Visual C++ Redistributable
   - Check: System requirements

5. Build failures
   - Solution: Ensure all required tools are installed
   - Solution: Clear build directory and rebuild
   - Check: CMake version and compiler compatibility

6. Configuration file errors
   - Solution: Delete corrupted config.json
   - Solution: Application will create new default config
   - Check: File permissions and JSON syntax

### Debug Options
- Console output enabled in debug builds
- Process monitoring logs
- OpenGL error checking
- Memory access validation
- ImGui demo window
- Configuration validation
- ESP rendering diagnostics
- Memory reading logs

### Performance Monitoring
- Frame time tracking
- Process check timing
- Memory read latency
- ESP render statistics
- Memory usage monitoring
- Configuration load/save timing
- Resource cleanup tracking