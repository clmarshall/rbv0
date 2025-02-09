# Systems Architecture

## Core Components

### Logger System
- Singleton pattern implementation for global logging
- Thread-safe operations with mutex protection
- File and console output capabilities
- Timestamp-based log entries
- Error level differentiation
- Automatic file management

### Rate Limiting System
- RateLimit class for managing operation frequency
- Configurable attempt limits and cooldown periods
- Automatic reset after cooldown
- Separate limits for different operations (scanning, reading)
- Thread-safe implementation

### Memory Management
- Safe memory access with validation
- Address range checking
- Memory state verification
- Protection flags validation
- Buffer overflow prevention
- String length safety

### Pattern Scanning
- Limited region scanning
- Maximum result limiting
- Cooldown periods between scans
- Error handling and logging
- Memory access validation

### Error Handling
- Comprehensive error logging
- Detailed error messages
- Error code translation
- Operation context logging
- Recovery mechanisms

## Data Flow
1. Memory operation request
2. Rate limit check
3. Memory validation
4. Operation execution
5. Error handling
6. Result logging
7. Rate limit update

## Safety Features
- Thread safety through mutex locks
- Memory access validation
- Rate limiting to prevent abuse
- String length checks
- Buffer overflow protection
- Error recovery mechanisms
# Systems Architecture

## Overview
The Roblox ESP Overlay is a transparent overlay application designed to provide visual enhancements for Roblox gameplay. It uses OpenGL for rendering, Dear ImGui for the user interface, and direct memory reading for real-time game data access.

## Core Components

### 1. Memory Reading System
- Process handle management
- Memory address scanning
- Data structure mapping
- Safety checks and validation
- Error handling for memory operations
- Team detection system
- Player data extraction
- Health monitoring
- Position tracking

### 2. Process Management
- Process detection system for Roblox
- Auto-launch capability via web browser
- Continuous process monitoring
- Automatic shutdown on Roblox exit
- Administrator privilege handling
- Process handle cleanup

### 3. Window Management
- GLFW window system
- OpenGL 3.3 context
- Transparent framebuffer
- Borderless window configuration
- Click-through capability
- Input passthrough
- Always-on-top functionality

### 4. Rendering System
- GLAD OpenGL loader
- Dear ImGui integration
- Vsync support
- Transparent overlay rendering
- ESP drawing system
  - Player boxes
  - Health bars
  - Distance indicators
  - Name tags
  - Team colors

### 5. Configuration System
- JSON-based configuration storage
- Real-time settings updates
- Configuration file handling
- Default settings management
- Settings persistence
- Error recovery

### 6. User Interface
- Settings menu with tabs
  - General settings
  - Color customization
  - Hotkey configuration
- Status indicator
- ESP toggle controls
- Visual customization options
- Input blocking prevention

### 7. Error Handling
- Exception management system
- User-friendly error dialogs
- Resource cleanup handlers
- GLFW error callback system
- Memory reading error handling
- Configuration error recovery

### 8. Hotkey System
- Global hotkey management
- Key binding system
- Toggle functionality
- Settings access
- Input interference prevention

## Technical Specifications

### Memory Reading
- Direct process memory access
- Structure mapping
- Address scanning
- Data validation
- Error handling
- Safety checks

### Graphics
- OpenGL 3.3
- GLSL version 130
- GLFW for window management
- Dear ImGui for UI rendering
- Transparent overlay
- Click-through support

### Configuration
- JSON format
- File-based storage
- Default configuration fallback
- Real-time updates
- Error recovery

### ESP Features
- Box rendering
- Health visualization
- Distance calculation
- Player information display
- Team detection
- Color customization
- Thickness adjustment
- Distance limiting

### System Requirements
- Windows operating system
- OpenGL 3.3 capable graphics card
- Roblox installation
- Administrator privileges
- DirectX 11 support

### Performance Considerations
- Optimized process checking (1-second intervals)
- Efficient memory reading
- Vsync enabled for smooth rendering
- Resource cleanup
- Minimal CPU usage
- Distance-based culling
- Input handling optimization

## File Structure
```
/
├── src/
│   └── main.cpp           # Main application code
├── include/
│   ├── glad/             # OpenGL loading
│   └── imgui/            # Dear ImGui library
├── build/                # Build outputs
├── config.json          # User configuration
└── memlog/              # Documentation
```

## Data Flow

### Memory Reading Flow
```
Initialize Process Handle
        ↓
Scan Memory Addresses
        ↓
Read Player Data
        ↓
Validate Data
        ↓
Update ESP Data
        ↓
Cleanup Resources
```

### Configuration Flow
```
Load Configuration
      ↓
Parse JSON Settings
      ↓
Apply Default Values
      ↓
Update UI State
      ↓
Handle User Changes
      ↓
Save Configuration
```

### ESP Rendering Flow
```
Read Memory Data
      ↓
Process Player Info
      ↓
Filter by Distance
      ↓
Calculate Screen Positions
      ↓
Apply Team Colors
      ↓
Render ESP Elements
      ↓
Update Display
```

## Dependencies
- GLFW (Window management)
- GLAD (OpenGL loading)
- Dear ImGui (UI framework)
- nlohmann/json (Configuration)
- Windows API (Process/Memory management)

## Security Considerations
- Administrator privileges required
- Memory reading validation
- Error handling for access violations
- Resource cleanup
- Handle management
- Process verification

## Future Expansion
- Advanced ESP features
- Performance optimizations
- Security enhancements
- Additional customization options
- Memory pattern scanning
- Anti-detection measures