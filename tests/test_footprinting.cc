    std::cout << "OS Version: " << sysInfo.os_version << std::endl;
    std::cout << "Architecture: " << sysInfo.architecture << std::endl;
    std::cout << "Domain Joined: " << (sysInfo.is_domain_joined ? "Yes" : "No") << std::endl;
    std::cout << "Computer Name: " << sysInfo.computer_name << std::endl;
    std::cout << "Username: " << sysInfo.username << std::endl;

    // Test Security Products
    std::vector<std::string> secProducts = recon.GetSecurityProducts();
    std::cout << "\n[Security Products]" << std::endl;
    if (secProducts.empty()) {
        std::cout << "No common security products detected." << std::endl;
    } else {
        for (const auto& prod : secProducts) {
            std::cout << "- " << prod << std::endl;
        }
    }

    // Test Process Enumeration
    std::cout << "\n[Process Enumeration]" << std::endl;
    std::vector<ProcessInfo> procs = recon.GetProcessList();
    std::cout << "Total Processes Found: " << procs.size() << std::endl;
    
    int systemProcs = 0;
    for(const auto& p : procs) {
        if (p.is_system) systemProcs++;
    }
    std::cout << "System Processes (Session 0): " << systemProcs << std::endl;

    if (!procs.empty()) {
        std::cout << "First 5 processes:" << std::endl;
        for (size_t i = 0; i < procs.size() && i < 5; ++i) {
            std::cout << "PID: " << procs[i].pid << " Name: " << procs[i].name 
                      << " Session: " << procs[i].session_id 
                      << " [" << (procs[i].is_system ? "SYSTEM" : "USER") << "]" << std::endl;
        }
    }

    // Test Payload Selection
    std::cout << "\n[Payload Selection]" << std::endl;
    PayloadType suggested = recon.SuggestPayload();
    std::cout << "Suggested Payload: " << recon.GetPayloadName(suggested) << std::endl;

    std::cout << "\nTest Complete." << std::endl;
    return 0;
}
