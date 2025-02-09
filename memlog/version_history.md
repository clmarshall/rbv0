# Version History

## Version 1.1.0 (2025-02-08)
- Major update focusing on safety and stability
- Added thread-safe logging system
- Implemented rate limiting for memory operations
- Enhanced error handling and recovery
- Improved memory access safety

### Key Features Added
- Thread-safe Logger class
- RateLimit system
- Memory validation
- Operation cooldowns
- Comprehensive error logging

### Technical Improvements
- Memory access validation
- String safety improvements
- Pattern scanning limits
- Rate limiting mechanisms
- Error recovery systems

### Safety Enhancements
- Buffer overflow protection
- String length validation
- Memory state verification
- Access violation prevention
- Thread safety mechanisms

## Version 1.0.0 (Initial Release)
- Basic memory reading functionality
- Pattern scanning capability
- Player data extraction
- ESP overlay rendering
- Basic configuration system
# Version History

## Version 0.3.0 (2025-02-08)
- Added memory reading and improved overlay interaction

### Features
- Real-time memory reading
- Player position detection
- Health value reading
- Team detection
- Click-through functionality
- Input passthrough
- Improved overlay interaction

### Technical Details
- OpenGL 3.3
- GLSL 130
- GLFW 3.3.8
- Dear ImGui 1.89.9
- nlohmann/json 3.11.2
- Windows API integration
- Process memory reading
- CMake build system

### Known Issues
- Memory offsets need regular updates
- Limited to basic player data

### Security Notes
- Basic memory reading implementation
- No anti-detection measures yet
- No encryption for configuration

### Compatibility
- Windows 10
- Requires OpenGL 3.3 capable graphics card
- Requires Roblox installation
- Requires administrator privileges for memory reading

## Version 0.2.0 (2025-02-08)
- Added ESP functionality and configuration system

### Features
- ESP drawing with player boxes
- Health bar visualization
- Distance indicators
- Player name display
- Settings menu with configuration
- JSON-based configuration system
- Hotkey system (F1/F2)
- Color customization
- Box thickness adjustment
- Maximum distance setting
- Status indicator window

### Technical Details
- OpenGL 3.3
- GLSL 130
- GLFW 3.3.8
- Dear ImGui 1.89.9
- nlohmann/json 3.11.2
- Windows API integration
- CMake build system

### Known Issues
- Using mock player data (memory reading not implemented)
- Limited customization for hotkeys

### Compatibility
- Windows 10
- Requires OpenGL 3.3 capable graphics card
- Requires Roblox installation

### Build Information
- CMake 3.10 minimum required
- C++17 standard
- MSVC compiler supported

## Version 0.1.0 (2025-02-08)
- Initial release
- Basic overlay functionality
- Process monitoring system
- Error handling implementation

### Features
- Transparent overlay window
- Roblox process detection
- Auto-launch capability
- Error notifications
- Debug logging
- ESC key exit
- Always-on-top behavior
- Click-through capability
- Full screen support

### Technical Details
- OpenGL 3.3
- GLSL 130
- GLFW 3.3.8
- Dear ImGui 1.89.9
- Windows API integration
- CMake build system

### Known Issues
- None reported

### Compatibility
- Windows 10
- Requires OpenGL 3.3 capable graphics card
- Requires Roblox installation

### Build Information
- CMake 3.10 minimum required
- C++17 standard
- MSVC compiler supported