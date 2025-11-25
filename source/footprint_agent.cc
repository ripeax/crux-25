#include "footprinting/footprinting.h"
#include <iostream>
#include <vector>
#include <windows.h>

// Helper to print banner
void PrintBanner() {
    std::cout << "============================================" << std::endl;
    std::cout << "    CRUX RECONNAISSANCE AGENT v1.0" << std::endl;
    std::cout << "============================================" << std::endl;
}

int main() {
    PrintBanner();

    Footprinting recon;

    // 1. System Information
    SystemInfo sysInfo = recon.GetSystemInfo();
    std::cout << "\n[+] System Information" << std::endl;
    std::cout << "    OS Version:    " << sysInfo.os_version << std::endl;
    std::cout << "    Architecture:  " << sysInfo.architecture << std::endl;
    std::cout << "    Domain Joined: " << (sysInfo.is_domain_joined ? "Yes" : "No") << std::endl;
    std::cout << "    Computer Name: " << sysInfo.computer_name << std::endl;
    std::cout << "    Username:      " << sysInfo.username << std::endl;

    // 2. Security Products
    std::vector<std::string> secProducts = recon.GetSecurityProducts();
    std::cout << "\n[+] Security Products" << std::endl;
    if (secProducts.empty()) {
        std::cout << "    [-] No common security products detected." << std::endl;
    } else {
        for (const auto& prod : secProducts) {
            std::cout << "    [!] Detected: " << prod << std::endl;
        }
    }

    // 3. Process Enumeration
    std::cout << "\n[+] Process Enumeration" << std::endl;
    std::vector<ProcessInfo> procs = recon.GetProcessList();
    std::cout << "    Total Processes: " << procs.size() << std::endl;
    
    int systemProcs = 0;
    for(const auto& p : procs) {
        if (p.is_system) systemProcs++;
    }
    std::cout << "    System Processes (Session 0): " << systemProcs << std::endl;
    std::cout << "    User Processes: " << (procs.size() - systemProcs) << std::endl;

    std::cout << "    User Processes: " << (procs.size() - systemProcs) << std::endl;

    // 4. Advanced Reconnaissance
    std::cout << "\n[+] Advanced Reconnaissance" << std::endl;
    IntegrityLevel integrity = recon.GetIntegrityLevel();
    std::cout << "    Integrity Level: " << recon.GetIntegrityLevelName(integrity) << std::endl;
    
    bool isSandbox = recon.IsSandbox();
    std::cout << "    Sandbox Detected: " << (isSandbox ? "YES" : "No") << std::endl;

    std::vector<std::string> software = recon.GetInstalledSoftware();
    std::cout << "    Installed Software: " << software.size() << " found." << std::endl;
    if (!software.empty()) {
        std::cout << "    First 5 apps:" << std::endl;
        for (size_t i = 0; i < software.size() && i < 5; ++i) {
            std::cout << "      - " << software[i] << std::endl;
        }
    }

    // 5. Payload Selection
    std::cout << "\n[+] Payload Selection" << std::endl;
    PayloadType suggested = recon.SuggestPayload();
    std::cout << "    Suggested Payload: " << recon.GetPayloadName(suggested) << std::endl;

    std::cout << "\n[+] Reconnaissance Complete." << std::endl;
    system("pause");
    return 0;
}
