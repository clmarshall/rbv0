#define GLFW_EXPOSE_NATIVE_WIN32
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>
#include "../build/_deps/imgui-src/imgui.h"
#include "../build/_deps/imgui-src/backends/imgui_impl_glfw.h"
#include "../build/_deps/imgui-src/backends/imgui_impl_opengl3.h"
#include <iostream>
#include <windows.h>
#include <tlhelp32.h>
#include <shellapi.h>
#include <vector>
#include <string>
#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <nlohmann/json.hpp>
#include <Psapi.h>

// Logger class for writing to file
class Logger {
private:
    static std::ofstream logFile;
    static std::mutex logMutex;

    static std::string getTimestamp() {
        auto now = std::chrono::system_clock::now();
        auto time = std::chrono::system_clock::to_time_t(now);
        struct tm timeinfo;
        localtime_s(&timeinfo, &time);
        std::stringstream ss;
        ss << std::put_time(&timeinfo, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    static void init() {
        std::lock_guard<std::mutex> lock(logMutex);
        logFile.open("roblox_esp.log", std::ios::app);
    }

    static void close() {
        std::lock_guard<std::mutex> lock(logMutex);
        if (logFile.is_open()) {
            logFile.close();
        }
    }

    template<typename T>
    static void log(const T& message, bool toConsole = true) {
        std::lock_guard<std::mutex> lock(logMutex);
        std::string timestamp = getTimestamp();
        std::stringstream ss;
        ss << "[" << timestamp << "] " << message;
        
        if (logFile.is_open()) {
            logFile << ss.str() << std::endl;
            logFile.flush();
        }
        
        if (toConsole) {
            std::cout << ss.str() << std::endl;
        }
    }

    static void error(const std::string& message) {
        log("ERROR: " + message);
        std::cerr << "ERROR: " << message << std::endl;
    }
};

std::ofstream Logger::logFile;
std::mutex Logger::logMutex;


using json = nlohmann::json;

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "psapi.lib")

// Configuration structure
struct Config {
    bool showESP = true;
    bool showBoxes = true;
    bool showDistance = true;
    bool showHealth = true;
    ImVec4 espColor = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
    float boxThickness = 2.0f;
    int maxDistance = 1000;
    bool showSettings = false;
    int toggleKey = GLFW_KEY_F1;
    int settingsKey = GLFW_KEY_F2;
};

Config config;

// Roblox memory reading structures
struct Vector3 {
    float x, y, z;
};

struct RobloxPlayer {
    std::string name;
    Vector3 position;
    float health;
    float maxHealth;
    bool isTeammate;
};

class RateLimit {
private:
    std::chrono::steady_clock::time_point lastAttempt;
    int attempts;
    const int maxAttempts;
    const std::chrono::milliseconds cooldown;

public:
    RateLimit(int maxAttempts = 3, std::chrono::milliseconds cooldown = std::chrono::milliseconds(100))
        : attempts(0), maxAttempts(maxAttempts), cooldown(cooldown) {
        lastAttempt = std::chrono::steady_clock::now() - cooldown;
    }

    bool shouldAllow() {
        auto now = std::chrono::steady_clock::now();
        if (now - lastAttempt >= cooldown) {
            attempts = 0;
        }

        if (attempts >= maxAttempts) {
            return false;
        }

        attempts++;
        lastAttempt = now;
        return true;
    }

    void reset() {
        attempts = 0;
        lastAttempt = std::chrono::steady_clock::now() - cooldown;
    }
};

class RobloxMemoryReader {
private:
    HANDLE processHandle;
    uintptr_t baseAddress;
    std::vector<uintptr_t> foundPatterns;
    RateLimit scanRateLimit;
    RateLimit readRateLimit;
    
    template<typename T>
    T ReadMemory(uintptr_t address) {
        T value = T();
        
        if (!readRateLimit.shouldAllow()) {
            Logger::error("Read rate limit exceeded, cooling down...");
            return value;
        }
        
        // Validate address range
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQueryEx(processHandle, (LPCVOID)address, &mbi, sizeof(mbi))) {
            DWORD error = GetLastError();
            std::stringstream ss;
            ss << "VirtualQueryEx failed for address 0x" << std::hex << address << " with error " << std::dec << error;
            Logger::error(ss.str());
            return value;
        }

        // Check memory state and protection
        if (mbi.State != MEM_COMMIT ||
            (mbi.Protect & PAGE_GUARD) ||
            (mbi.Protect & PAGE_NOACCESS) ||
            address < (uintptr_t)mbi.BaseAddress ||
            address + sizeof(T) > (uintptr_t)mbi.BaseAddress + mbi.RegionSize) {
            std::stringstream ss;
            ss << "Invalid memory access at 0x" << std::hex << address;
            Logger::error(ss.str());
            return value;
        }

        SIZE_T bytesRead;
        if (!ReadProcessMemory(processHandle, (LPCVOID)address, &value, sizeof(T), &bytesRead) ||
            bytesRead != sizeof(T)) {
            DWORD error = GetLastError();
            std::stringstream ss;
            ss << "ReadMemory failed at 0x" << std::hex << address << " with error " << std::dec << error;
            Logger::error(ss.str());
            return value;
        }

        return value;
    }

    std::string ReadString(uintptr_t address, size_t maxLength = 32) {
        std::vector<char> buffer(maxLength + 1, 0);
        SIZE_T bytesRead;
        
        if (!ReadProcessMemory(processHandle, (LPCVOID)address, buffer.data(), maxLength, &bytesRead)) {
            DWORD error = GetLastError();
            std::stringstream ss;
            ss << "ReadString failed at 0x" << std::hex << address << " with error " << std::dec << error;
            Logger::error(ss.str());
            return "";
        }
        
        // Ensure null termination and valid string length
        buffer[maxLength] = '\0';
        size_t len = 0;
        while (len < maxLength && buffer[len] != '\0' && isprint(buffer[len])) {
            len++;
        }
        buffer[len] = '\0';
        
        return std::string(buffer.data());
    }

    // Pattern scanning function
    std::vector<uintptr_t> ScanMemory(const std::vector<uint8_t>& pattern, const std::string& mask) {
        static const size_t MAX_REGIONS = 1000;  // Maximum memory regions to scan
        static const size_t MAX_RESULTS = 20;    // Maximum pattern matches to find
        
        if (!scanRateLimit.shouldAllow()) {
            Logger::error("Pattern scan rate limit exceeded, cooling down...");
            return {};
        }

        if (pattern.empty() || mask.empty() || pattern.size() != mask.size()) {
            Logger::error("Invalid pattern or mask: size mismatch or empty");
            return {};
        }

        std::vector<uintptr_t> results;
        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t addr = 0;
        SIZE_T result;
        size_t regionsScanned = 0;

        try {
            while (regionsScanned < MAX_REGIONS &&
                   results.size() < MAX_RESULTS &&
                   (result = VirtualQueryEx(processHandle, (LPCVOID)addr, &mbi, sizeof(mbi)))) {
                regionsScanned++;
                
                if (mbi.State == MEM_COMMIT &&
                    (mbi.Protect & PAGE_GUARD) == 0 &&
                    (mbi.Protect & PAGE_NOACCESS) == 0) {
                    
                    try {
                        std::vector<uint8_t> buffer(mbi.RegionSize);
                        SIZE_T bytesRead;
                        
                        if (ReadProcessMemory(processHandle, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                            if (bytesRead < pattern.size()) {
                                addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
                                continue;
                            }

                            for (size_t i = 0; i < bytesRead - pattern.size(); i++) {
                                bool found = true;
                                for (size_t j = 0; j < pattern.size(); j++) {
                                    if (mask[j] == 'x' && buffer[i + j] != pattern[j]) {
                                        found = false;
                                        break;
                                    }
                                }
                                if (found) {
                                    results.push_back((uintptr_t)mbi.BaseAddress + i);
                                    std::stringstream ss;
                                    ss << "Found pattern at: 0x" << std::hex << ((uintptr_t)mbi.BaseAddress + i);
                                    Logger::log(ss.str());
                                }
                            }
                        } else {
                            DWORD error = GetLastError();
                            std::stringstream ss;
                            ss << "ReadProcessMemory failed at 0x" << std::hex << (uintptr_t)mbi.BaseAddress
                               << " with error " << std::dec << error;
                            Logger::error(ss.str());
                        }
                    } catch (const std::bad_alloc& e) {
                        std::stringstream ss;
                        ss << "Memory allocation failed for region at 0x" << std::hex << (uintptr_t)mbi.BaseAddress
                           << ": " << e.what();
                        Logger::error(ss.str());
                        addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
                        continue;
                    }
                }
                addr = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
            }

            if (result == 0 && GetLastError() != ERROR_NO_MORE_ITEMS) {
                DWORD error = GetLastError();
                std::stringstream ss;
                ss << "VirtualQueryEx failed with error " << error;
                Logger::error(ss.str());
            }
        } catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Error during memory scan: " << e.what();
            Logger::error(ss.str());
        }

        return results;
    }

    // Find player list through UI elements
    uintptr_t FindPlayerListThroughUI() {
    try {
        // Pattern for UI container that holds player list
        std::vector<uint8_t> pattern = {
            0x48, 0x8B, 0x05, 0x00, 0x00, 0x00, 0x00, // mov rax, [PlayerListClass]
            0x48, 0x85, 0xC0,                         // test rax, rax
            0x74, 0x00,                               // je short
            0x48, 0x8B, 0x48                          // mov rcx, [rax+?]
        };
        std::string mask = "xxx????xxx?xxx";
        
        if (pattern.size() != mask.size()) {
            std::cerr << "Pattern and mask size mismatch in FindPlayerListThroughUI" << std::endl;
            return 0;
        }

        auto results = ScanMemory(pattern, mask);
        if (results.empty()) {
            std::cout << "No UI pattern matches found" << std::endl;
            return 0;
        }

        // Validate and follow pointer chain
        for (auto potentialPtr : results) {
            try {
                if (!potentialPtr) continue;

                // Read and validate the offset
                uintptr_t offset = ReadMemory<uint32_t>(potentialPtr + 3);
                if (offset > 0x10000000) { // Sanity check for reasonable offset
                    std::cout << "Skipping invalid offset: 0x" << std::hex << offset << std::dec << std::endl;
                    continue;
                }

                uintptr_t targetAddr = potentialPtr + offset + 7;
                
                // Validate the target address
                MEMORY_BASIC_INFORMATION mbi;
                if (VirtualQueryEx(processHandle, (LPCVOID)targetAddr, &mbi, sizeof(mbi))) {
                    if (mbi.State == MEM_COMMIT &&
                        (mbi.Protect & PAGE_GUARD) == 0 &&
                        (mbi.Protect & PAGE_NOACCESS) == 0) {
                        
                        std::cout << "Found valid UI pattern at: 0x" << std::hex << targetAddr << std::dec << std::endl;
                        return targetAddr;
                    }
                }
            }
            catch (const std::exception& e) {
                std::cerr << "Error validating UI pattern: " << e.what() << std::endl;
                continue;
            }
        }

        std::cout << "No valid UI patterns found" << std::endl;
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Error in FindPlayerListThroughUI: " << e.what() << std::endl;
        return 0;
    }
}

    // Find player data through render objects
    uintptr_t FindPlayersThroughRender() {
        try {
            // Pattern for rendered player models
            std::vector<uint8_t> pattern = {
                0x48, 0x8B, 0x0D, 0x00, 0x00, 0x00, 0x00, // mov rcx, [RenderClass]
                0xE8, 0x00, 0x00, 0x00, 0x00,             // call RenderFunction
                0x48, 0x85, 0xC0                          // test rax, rax
            };
            std::string mask = "xxx????x????xxx";
            
            if (pattern.size() != mask.size()) {
                std::cerr << "Pattern and mask size mismatch in FindPlayersThroughRender" << std::endl;
                return 0;
            }

            auto results = ScanMemory(pattern, mask);
            if (results.empty()) {
                std::cout << "No render pattern matches found" << std::endl;
                return 0;
            }

            // Validate and follow pointer chain
            for (auto potentialPtr : results) {
                try {
                    if (!potentialPtr) continue;

                    // Read and validate the offset
                    uintptr_t offset = ReadMemory<uint32_t>(potentialPtr + 3);
                    if (offset > 0x10000000) { // Sanity check for reasonable offset
                        std::cout << "Skipping invalid offset: 0x" << std::hex << offset << std::dec << std::endl;
                        continue;
                    }

                    uintptr_t targetAddr = potentialPtr + offset + 7;
                    
                    // Validate the target address
                    MEMORY_BASIC_INFORMATION mbi;
                    if (VirtualQueryEx(processHandle, (LPCVOID)targetAddr, &mbi, sizeof(mbi))) {
                        if (mbi.State == MEM_COMMIT &&
                            (mbi.Protect & PAGE_GUARD) == 0 &&
                            (mbi.Protect & PAGE_NOACCESS) == 0) {
                            
                            std::cout << "Found valid render pattern at: 0x" << std::hex << targetAddr << std::dec << std::endl;
                            return targetAddr;
                        }
                    }
                }
                catch (const std::exception& e) {
                    std::cerr << "Error validating render pattern: " << e.what() << std::endl;
                    continue;
                }
            }

            std::cout << "No valid render patterns found" << std::endl;
            return 0;
        }
        catch (const std::exception& e) {
            std::cerr << "Error in FindPlayersThroughRender: " << e.what() << std::endl;
            return 0;
        }
    }

public:
    bool Initialize() {
        try {
            Logger::log("Initializing memory reader...");
            DWORD processId = 0;
            HWND window = FindWindowA("WINDOWSCLIENT", "Roblox");
            if (!window) {
                Logger::error("Failed to find Roblox window");
                return false;
            }

            GetWindowThreadProcessId(window, &processId);
            if (!processId) {
                Logger::error("Failed to get process ID");
                return false;
            }

            processHandle = OpenProcess(PROCESS_VM_READ | PROCESS_QUERY_INFORMATION, FALSE, processId);
            if (!processHandle) {
                DWORD error = GetLastError();
                std::stringstream ss;
                ss << "Failed to open process with error " << error;
                Logger::error(ss.str());
                return false;
            }

            HMODULE modules[1024];
            DWORD needed;
            if (!EnumProcessModules(processHandle, modules, sizeof(modules), &needed)) {
                DWORD error = GetLastError();
                std::stringstream ss;
                ss << "Failed to enumerate process modules with error " << error;
                Logger::error(ss.str());
                CloseHandle(processHandle);
                return false;
            }

            baseAddress = (uintptr_t)modules[0];
            std::stringstream ss;
            ss << "Base address: 0x" << std::hex << baseAddress;
            Logger::log(ss.str());
            
            // Try to find player data through multiple methods
            foundPatterns.clear();
            Logger::log("Searching for player list patterns...");
            
            if (auto uiPtr = FindPlayerListThroughUI()) {
                foundPatterns.push_back(uiPtr);
                std::stringstream ss;
                ss << "Found player list through UI at: 0x" << std::hex << uiPtr;
                Logger::log(ss.str());
            }
            
            if (auto renderPtr = FindPlayersThroughRender()) {
                foundPatterns.push_back(renderPtr);
                std::stringstream ss;
                ss << "Found player list through render at: 0x" << std::hex << renderPtr;
                Logger::log(ss.str());
            }
            
            if (foundPatterns.empty()) {
                Logger::error("No valid patterns found");
                CloseHandle(processHandle);
                return false;
            }
            
            Logger::log("Memory reader initialized successfully");
            return true;
        }
        catch (const std::exception& e) {
            std::stringstream ss;
            ss << "Error during initialization: " << e.what();
            Logger::error(ss.str());
            if (processHandle) {
                CloseHandle(processHandle);
                processHandle = nullptr;
            }
            return false;
        }
    }

    std::vector<RobloxPlayer> GetPlayers() {
        static const std::chrono::milliseconds PATTERN_COOLDOWN(50);
        std::vector<RobloxPlayer> players;
        
        if (!processHandle || foundPatterns.empty()) {
            Logger::error("Invalid process handle or no patterns found");
            return players;
        }

        if (!readRateLimit.shouldAllow()) {
            Logger::error("Player data read rate limit exceeded, cooling down...");
            return players;
        }

        for (auto basePtr : foundPatterns) {
            try {
                std::stringstream ss;
                ss << "Trying pattern at 0x" << std::hex << basePtr;
                Logger::log(ss.str());

                std::this_thread::sleep_for(PATTERN_COOLDOWN); // Add cooldown between patterns

                // Try to read player data through each found pattern
                uintptr_t listPtr = ReadMemory<uintptr_t>(basePtr);
                if (!listPtr) {
                    Logger::error("Failed to read list pointer");
                    continue;
                }

                // Follow pointer chain
                uintptr_t containerPtr = ReadMemory<uintptr_t>(listPtr + 0x10);
                if (!containerPtr) {
                    Logger::error("Failed to read container pointer");
                    continue;
                }

                int count = ReadMemory<int>(containerPtr + 0x8);
                if (count <= 0 || count > 100) {
                    ss.str("");
                    ss << "Invalid player count: " << count;
                    Logger::error(ss.str());
                    continue;
                }

                ss.str("");
                ss << "Found " << count << " potential players";
                Logger::log(ss.str());

                for (int i = 0; i < count; i++) {
                    std::stringstream ss;
                    ss << "Processing player " << i + 1 << " of " << count;
                    Logger::log(ss.str());

                    uintptr_t playerPtr = ReadMemory<uintptr_t>(containerPtr + 0x10 + i * 8);
                    if (!playerPtr) {
                        Logger::error("Failed to read player pointer");
                        continue;
                    }

                    // Try to validate player object
                    uint32_t magicNumber = ReadMemory<uint32_t>(playerPtr);
                    if (magicNumber != 0x1234567) {
                        ss.str("");
                        ss << "Invalid magic number: 0x" << std::hex << magicNumber;
                        Logger::error(ss.str());
                        continue;
                    }

                    RobloxPlayer player;
                    bool validPlayer = true;
                    
                    // Read through multiple pointer chains for redundancy
                    auto readPosition = [&](uintptr_t ptr) -> Vector3 {
                        Vector3 pos;
                        for (int offset : {0x80, 0x90, 0xA0}) {
                            pos = ReadMemory<Vector3>(ptr + offset);
                            if (pos.x != 0 || pos.y != 0 || pos.z != 0) {
                                ss.str("");
                                ss << "Found valid position at offset 0x" << std::hex << offset;
                                Logger::log(ss.str());
                                return pos;
                            }
                        }
                        Logger::error("Failed to find valid position");
                        validPlayer = false;
                        return pos;
                    };

                    player.position = readPosition(playerPtr);
                    if (!validPlayer) continue;
                    
                    // Read other data through multiple paths
                    bool foundHealth = false;
                    for (int healthOffset : {0x100, 0x110, 0x120}) {
                        player.health = ReadMemory<float>(playerPtr + healthOffset);
                        if (player.health > 0 && player.health <= 100) {
                            foundHealth = true;
                            ss.str("");
                            ss << "Found valid health " << player.health << " at offset 0x" << std::hex << healthOffset;
                            Logger::log(ss.str());
                            break;
                        }
                    }
                    if (!foundHealth) {
                        Logger::error("Failed to find valid health value");
                        continue;
                    }

                    player.maxHealth = 100; // Default to 100 if can't read

                    // Try to read name through different methods
                    bool foundName = false;
                    uintptr_t namePtr = 0;
                    for (int nameOffset : {0x40, 0x48, 0x50}) {
                        namePtr = ReadMemory<uintptr_t>(playerPtr + nameOffset);
                        if (namePtr) {
                            std::string name = ReadString(namePtr);
                            if (!name.empty()) {
                                player.name = name;
                                foundName = true;
                                ss.str("");
                                ss << "Found valid name '" << name << "' at offset 0x" << std::hex << nameOffset;
                                Logger::log(ss.str());
                                break;
                            }
                        }
                    }
                    if (!foundName) {
                        Logger::error("Failed to find valid name");
                        continue;
                    }

                    // Determine team through color or other properties
                    player.isTeammate = false; // Default to enemy
                    bool foundTeam = false;
                    for (int teamOffset : {0x120, 0x130, 0x140}) {
                        int teamId = ReadMemory<int>(playerPtr + teamOffset);
                        if (teamId > 0) {
                            player.isTeammate = (teamId == ReadMemory<int>(basePtr + 0x10));
                            foundTeam = true;
                            ss.str("");
                            ss << "Found team info (ID: " << teamId << ") at offset 0x" << std::hex << teamOffset;
                            Logger::log(ss.str());
                            break;
                        }
                    }
                    if (!foundTeam) {
                        Logger::log("No team info found, defaulting to enemy");
                    }

                    if (player.health > 0 && !player.name.empty()) {
                        ss.str("");
                        ss << "Adding valid player: " << player.name << " (Health: " << player.health << ")";
                        Logger::log(ss.str());
                        players.push_back(player);
                    }
                }

                if (!players.empty()) break; // Successfully found players through this pattern
            }
            catch (...) {
                continue; // Try next pattern if this one fails
            }
        }

        return players;
    }

    void Cleanup() {
        if (processHandle) {
            CloseHandle(processHandle);
            processHandle = nullptr;
        }
    }

    ~RobloxMemoryReader() {
        Cleanup();
    }
};

RobloxMemoryReader memoryReader;

// Function to save configuration
void saveConfig() {
    try {
        Logger::log("Saving configuration...");
        json j;
        j["showESP"] = config.showESP;
        j["showBoxes"] = config.showBoxes;
        j["showDistance"] = config.showDistance;
        j["showHealth"] = config.showHealth;
        j["espColor"] = {
            {"r", config.espColor.x},
            {"g", config.espColor.y},
            {"b", config.espColor.z},
            {"a", config.espColor.w}
        };
        j["boxThickness"] = config.boxThickness;
        j["maxDistance"] = config.maxDistance;
        j["toggleKey"] = config.toggleKey;
        j["settingsKey"] = config.settingsKey;

        std::ofstream file("config.json");
        if (!file.is_open()) {
            Logger::error("Failed to open config.json for writing");
            return;
        }
        file << j.dump(4);
        Logger::log("Configuration saved successfully");
    }
    catch (const std::exception& e) {
        std::string error = std::string("Failed to save config: ") + e.what();
        Logger::error(error);
    }
}

// Function to load configuration
void loadConfig() {
    try {
        Logger::log("Loading configuration...");
        std::ifstream file("config.json");
        if (file.is_open()) {
            json j = json::parse(file);
            config.showESP = j["showESP"];
            config.showBoxes = j["showBoxes"];
            config.showDistance = j["showDistance"];
            config.showHealth = j["showHealth"];
            config.espColor = ImVec4(
                j["espColor"]["r"],
                j["espColor"]["g"],
                j["espColor"]["b"],
                j["espColor"]["a"]
            );
            config.boxThickness = j["boxThickness"];
            config.maxDistance = j["maxDistance"];
            config.toggleKey = j["toggleKey"];
            config.settingsKey = j["settingsKey"];
            Logger::log("Configuration loaded successfully");
        } else {
            Logger::log("No existing config found, using defaults");
        }
    }
    catch (const std::exception& e) {
        std::string error = std::string("Failed to load config: ") + e.what();
        Logger::error(error);
        Logger::log("Using default configuration");
    }
}

// Function to check if a process is running
bool isProcessRunning(const wchar_t* processName, bool silent = true) {
    bool exists = false;
    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        if (!silent) {
            DWORD error = GetLastError();
            std::stringstream ss;
            ss << "Failed to create process snapshot with error " << error;
            Logger::error(ss.str());
        }
        return false;
    }

    if (Process32FirstW(snapshot, &entry)) {
        while (Process32NextW(snapshot, &entry)) {
            if (_wcsicmp(entry.szExeFile, processName) == 0) {
                exists = true;
                if (!silent) {
                    size_t size = wcslen(processName) * 2 + 1;
                    std::vector<char> processNameStr(size);
                    size_t converted;
                    wcstombs_s(&converted, processNameStr.data(), size, processName, _TRUNCATE);
                    Logger::log(std::string("Found process: ") + processNameStr.data());
                }
                break;
            }
        }
    } else if (!silent) {
        DWORD error = GetLastError();
        std::stringstream ss;
        ss << "Failed to enumerate processes with error " << error;
        Logger::error(ss.str());
    }

    CloseHandle(snapshot);
    if (!silent && !exists) {
        size_t size = wcslen(processName) * 2 + 1;
        std::vector<char> processNameStr(size);
        size_t converted;
        wcstombs_s(&converted, processNameStr.data(), size, processName, _TRUNCATE);
        Logger::log(std::string("Process not found: ") + processNameStr.data());
    }
    return exists;
}

// Function to show error message box
void showError(const std::string& message) {
    Logger::error(message);
    MessageBoxA(NULL, message.c_str(), "Error", MB_OK | MB_ICONERROR);
}

// GLFW error callback
static void glfw_error_callback(int error, const char* description) {
    std::stringstream ss;
    ss << "GLFW Error " << error << ": " << description;
    Logger::error(ss.str());
    showError(std::string("GLFW Error: ") + description);
}

// Function to convert world position to screen position
bool WorldToScreen(const Vector3& pos, ImVec2& screen, const ImVec2& displaySize) {
    // This is a simplified projection - real implementation would need proper view matrix
    // You would need to get the actual camera matrix from Roblox memory
    float fov = 90.0f;
    float aspectRatio = displaySize.x / displaySize.y;
    float nearPlane = 0.1f;
    
    // Simple perspective projection
    float scale = tanf((fov * 0.5f) * 0.0174532925f);
    float x = pos.x / (pos.z * scale * aspectRatio);
    float y = pos.y / (pos.z * scale);
    
    // Convert to screen coordinates
    screen.x = (x + 1.0f) * displaySize.x * 0.5f;
    screen.y = (1.0f - y) * displaySize.y * 0.5f;
    
    // Check if point is behind camera or too far
    return pos.z > nearPlane && pos.z < config.maxDistance;
}

// Function to render ESP
void renderESP() {
    if (!config.showESP) return;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 displaySize = ImGui::GetIO().DisplaySize;
    
    std::vector<RobloxPlayer> players = memoryReader.GetPlayers();
    
    for (const auto& player : players) {
        ImVec2 screenPos;
        if (!WorldToScreen(player.position, screenPos, displaySize)) continue;

        // Draw player box
        if (config.showBoxes) {
            float boxWidth = 50.0f * (1000.0f / player.position.z);
            float boxHeight = 100.0f * (1000.0f / player.position.z);
            ImVec2 boxMin(screenPos.x - boxWidth/2, screenPos.y - boxHeight);
            ImVec2 boxMax(screenPos.x + boxWidth/2, screenPos.y);
            
            ImU32 boxColor = player.isTeammate ? 
                IM_COL32(0, 255, 0, 255) : 
                ImGui::ColorConvertFloat4ToU32(config.espColor);
            
            draw_list->AddRect(
                boxMin, boxMax,
                boxColor,
                0.0f, 0, config.boxThickness
            );
        }

        // Draw player info
        ImVec2 textPos(screenPos.x, screenPos.y - 120.0f);
        float distance = sqrtf(
            player.position.x * player.position.x +
            player.position.y * player.position.y +
            player.position.z * player.position.z
        );

        if (config.showDistance) {
            draw_list->AddText(
                textPos,
                ImGui::ColorConvertFloat4ToU32(config.espColor),
                (std::to_string((int)distance) + "m").c_str()
            );
            textPos.y += 15.0f;
        }

        if (config.showHealth) {
            // Health bar background
            draw_list->AddRectFilled(
                ImVec2(screenPos.x - 25.0f, textPos.y),
                ImVec2(screenPos.x + 25.0f, textPos.y + 5.0f),
                IM_COL32(255, 0, 0, 255)
            );

            // Health bar
            float healthPercent = player.health / player.maxHealth;
            draw_list->AddRectFilled(
                ImVec2(screenPos.x - 25.0f, textPos.y),
                ImVec2(screenPos.x - 25.0f + (50.0f * healthPercent), textPos.y + 5.0f),
                IM_COL32(0, 255, 0, 255)
            );
            textPos.y += 15.0f;
        }

        // Draw player name
        draw_list->AddText(textPos, ImGui::ColorConvertFloat4ToU32(config.espColor), player.name.c_str());
    }
}

// Function to render settings window
void renderSettings() {
    if (!config.showSettings) return;

    ImGui::SetNextWindowSize(ImVec2(400, 400), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("ESP Settings", &config.showSettings)) {
        if (ImGui::BeginTabBar("SettingsTabs")) {
            if (ImGui::BeginTabItem("General")) {
                ImGui::Checkbox("Enable ESP", &config.showESP);
                ImGui::Checkbox("Show Boxes", &config.showBoxes);
                ImGui::Checkbox("Show Distance", &config.showDistance);
                ImGui::Checkbox("Show Health", &config.showHealth);
                ImGui::SliderFloat("Box Thickness", &config.boxThickness, 1.0f, 5.0f);
                ImGui::SliderInt("Max Distance", &config.maxDistance, 100, 2000);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Colors")) {
                ImGui::ColorEdit4("ESP Color", (float*)&config.espColor);
                ImGui::EndTabItem();
            }
            
            if (ImGui::BeginTabItem("Hotkeys")) {
                ImGui::Text("Toggle ESP: F1");
                ImGui::Text("Settings: F2");
                ImGui::EndTabItem();
            }
            
            ImGui::EndTabBar();
        }

        if (ImGui::Button("Save Settings")) {
            saveConfig();
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset to Defaults")) {
            config = Config();
            saveConfig();
        }
    }
    ImGui::End();
}

int main() {
    try {
        // Initialize logger
        Logger::init();
        Logger::log("Starting Roblox ESP Overlay...");

        // Load configuration
        loadConfig();

        // Check if Roblox is running
        if (!isProcessRunning(L"RobloxPlayerBeta.exe", false)) {
            Logger::log("Showing Roblox not running dialog...");
            int response = MessageBoxA(
                NULL,
                "Roblox is not running. Would you like to launch it?",
                "Roblox Not Found",
                MB_YESNO | MB_ICONQUESTION
            );

            if (response == IDYES) {
                Logger::log("Launching Roblox website...");
                HINSTANCE result = ShellExecuteA(NULL, "open", "https://www.roblox.com/home", NULL, NULL, SW_SHOWNORMAL);
                if ((intptr_t)result <= 32) { // ShellExecute returns HINSTANCE >32 on success
                    Logger::error("Failed to launch browser");
                }
                Logger::log("Exiting application - please restart after Roblox is running");
                Logger::close();
                return 0;
            } else {
                Logger::log("User chose not to launch Roblox");
                Logger::log("Exiting application");
                Logger::close();
                return 1;
            }
        }

        // Initialize memory reader
        if (!memoryReader.Initialize()) {
            Logger::error("Failed to initialize memory reader");
            Logger::close();
            throw std::runtime_error("Failed to initialize memory reader");
        }

        Logger::log("Initializing GLFW...");
        // Setup GLFW window
        Logger::log("Setting up GLFW error callback...");
        glfwSetErrorCallback(glfw_error_callback);
        
        Logger::log("Initializing GLFW...");
        if (!glfwInit()) {
            Logger::error("Failed to initialize GLFW - check graphics drivers");
            Logger::log("Exiting application due to GLFW initialization failure");
            Logger::close();
            throw std::runtime_error("Failed to initialize GLFW");
        }
        Logger::log("GLFW initialized successfully");

        // GL 3.3 + GLSL 130
        const char* glsl_version = "#version 130";
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, GLFW_TRUE);
        glfwWindowHint(GLFW_FLOATING, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

        Logger::log("Creating window...");
        // Create window with graphics context
        GLFWwindow* window = glfwCreateWindow(1280, 720, "Roblox ESP Overlay", nullptr, nullptr);
        if (window == nullptr) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        // Set window to click-through
        HWND hwnd = glfwGetWin32Window(window);
        LONG ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
        SetWindowLong(hwnd, GWL_EXSTYLE, ex_style | WS_EX_TRANSPARENT | WS_EX_LAYERED);

        glfwMakeContextCurrent(window);
        glfwSwapInterval(1); // Enable vsync

        Logger::log("Initializing GLAD...");
        // Initialize GLAD
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            glfwDestroyWindow(window);
            glfwTerminate();
            Logger::error("Failed to initialize GLAD");
            Logger::close();
            throw std::runtime_error("Failed to initialize GLAD");
        }

        Logger::log("Setting up ImGui...");
        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForOpenGL(window, true);
        ImGui_ImplOpenGL3_Init(glsl_version);

        bool robloxClosed = false;
        auto lastCheckTime = std::chrono::steady_clock::now();
        const auto checkInterval = std::chrono::seconds(1); // Check every second

        Logger::log("Entering main loop...");

        // Main loop
        while (!glfwWindowShouldClose(window)) {
            auto currentTime = std::chrono::steady_clock::now();
            
            // Check Roblox process status every second
            if (currentTime - lastCheckTime >= checkInterval) {
                if (!isProcessRunning(L"RobloxPlayerBeta.exe") && !robloxClosed) {
                    showError("Roblox process has been closed. ESP overlay will now exit.");
                    robloxClosed = true;
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                    continue;
                }
                lastCheckTime = currentTime;
            }

            glfwPollEvents();

            // Handle hotkeys
            if (glfwGetKey(window, config.toggleKey) == GLFW_PRESS) {
                config.showESP = !config.showESP;
                glfwWaitEventsTimeout(0.2);
            }
            if (glfwGetKey(window, config.settingsKey) == GLFW_PRESS) {
                config.showSettings = !config.showSettings;
                // Remove click-through when settings are open
                HWND hwnd = glfwGetWin32Window(window);
                LONG ex_style = GetWindowLong(hwnd, GWL_EXSTYLE);
                if (config.showSettings) {
                    SetWindowLong(hwnd, GWL_EXSTYLE, ex_style & ~WS_EX_TRANSPARENT);
                } else {
                    SetWindowLong(hwnd, GWL_EXSTYLE, ex_style | WS_EX_TRANSPARENT);
                }
                glfwWaitEventsTimeout(0.2);
            }

            // Start the Dear ImGui frame
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            // Create a fullscreen window for ESP
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
            ImGui::Begin("Overlay", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_NoMove | 
                ImGuiWindowFlags_NoScrollbar | 
                ImGuiWindowFlags_NoScrollWithMouse | 
                ImGuiWindowFlags_NoCollapse | 
                ImGuiWindowFlags_NoBackground | 
                ImGuiWindowFlags_NoSavedSettings | 
                ImGuiWindowFlags_NoBringToFrontOnFocus |
                ImGuiWindowFlags_NoInputs);

            // Render ESP
            renderESP();
            
            ImGui::End();

            // Render settings window
            renderSettings();

            // Status text
            ImGui::SetNextWindowPos(ImVec2(10, 10));
            ImGui::SetNextWindowBgAlpha(0.3f);
            ImGui::Begin("Status", nullptr, 
                ImGuiWindowFlags_NoTitleBar | 
                ImGuiWindowFlags_NoResize | 
                ImGuiWindowFlags_AlwaysAutoResize | 
                ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoInputs);
            ImGui::Text("ESP: %s (F1)", config.showESP ? "ON" : "OFF");
            ImGui::Text("Settings: F2");
            ImGui::End();

            // Rendering
            ImGui::Render();
            int display_w, display_h;
            glfwGetFramebufferSize(window, &display_w, &display_h);
            glViewport(0, 0, display_w, display_h);
            glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
            glClear(GL_COLOR_BUFFER_BIT);
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glfwSwapBuffers(window);
        }

        Logger::log("Cleaning up...");
        // Cleanup
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        glfwDestroyWindow(window);
        glfwTerminate();

        memoryReader.Cleanup();
        Logger::log("Cleanup complete");
        Logger::close();

        return 0;
    }
    catch (const std::exception& e) {
        std::string error = std::string("Fatal Error: ") + e.what();
        Logger::error(error);
        showError(error);
        Logger::close();
        return 1;
    }
    catch (...) {
        Logger::error("An unknown error occurred");
        showError("An unknown error occurred");
        Logger::close();
        return 1;
    }
}