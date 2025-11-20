#include "crypto.h"
#include <sstream>

// Simple DJB2 hash for string
unsigned long djb2_hash(const std::string& str) {
    unsigned long hash = 5381;
    for (char c : str) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

time_based_encoder::time_based_encoder(const std::string& base_key, uint64_t time_window_seconds) 
    : base_key(base_key), time_window(time_window_seconds), current_timestamp(0), current_hint("") {}

std::string time_based_encoder::encrypt(const std::string& data) {
    std::string key = derive_key();
    return xor_process(data, key);
}

std::string time_based_encoder::decrypt(const std::string& data) {
    std::string key = derive_key();
    return xor_process(data, key);
}

void time_based_encoder::set_key_context(uint64_t timestamp, const std::string& hint) {
    current_timestamp = timestamp;
    current_hint = hint;
}

std::string time_based_encoder::derive_key() {
    // Calculate time step
    uint64_t time_step = current_timestamp / time_window;
    
    // Combine base key, time step, and hint
    std::stringstream ss;
    ss << base_key << ":" << time_step << ":" << current_hint;
    std::string combined = ss.str();

    // Hash to generate a derived key
    unsigned long hash = djb2_hash(combined);
    
    // Convert hash to string key
    std::stringstream key_ss;
    key_ss << std::hex << hash;
    return key_ss.str();
}

std::string time_based_encoder::xor_process(const std::string& data, const std::string& key) {
    std::string result = data;
    size_t key_len = key.length();
    
    if (key_len == 0) return result;

    for (size_t i = 0; i < result.length(); ++i) {
        result[i] ^= key[i % key_len];
    }
    return result;
}
