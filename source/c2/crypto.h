#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>
#include <cstdint>

// Abstract base class for crypto encoders
class crypto_encoder {
public:
    virtual ~crypto_encoder() = default;
    virtual std::string encrypt(const std::string& data) = 0;
    virtual std::string decrypt(const std::string& data) = 0;
    // Set context for key derivation (e.g., timestamp and hint)
    virtual void set_key_context(uint64_t timestamp, const std::string& hint) = 0;
};

// Time-Based Key XOR Encoder implementation
class time_based_encoder : public crypto_encoder {
public:
    time_based_encoder(const std::string& base_key, uint64_t time_window_seconds = 30);
    
    std::string encrypt(const std::string& data) override;
    std::string decrypt(const std::string& data) override;
    void set_key_context(uint64_t timestamp, const std::string& hint) override;

private:
    std::string base_key;
    uint64_t time_window;
    uint64_t current_timestamp;
    std::string current_hint;
    
    std::string derive_key();
    std::string xor_process(const std::string& data, const std::string& key);
};

#endif // CRYPTO_H
