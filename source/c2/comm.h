#ifndef COMM_H
#define COMM_H

#include <windows.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include "json.hpp"

using json = nlohmann::json;

class Comm {
public:
    Comm(const std::wstring& userAgent);
    ~Comm();

    bool Initialize();
    void SetProxy(const std::wstring& proxy);
    std::string SendRequest(const std::wstring& server, int port, const std::wstring& path, const std::wstring& method, const std::string& data = "");

#ifdef DEBUG
    void SetDebugBypass(bool bypass);
#endif

private:
    HINTERNET hSession;
    HINTERNET hConnect;
    HINTERNET hRequest;
    std::wstring userAgent;
    std::wstring proxy;

#ifdef DEBUG
    bool debug_bypass = false;
#endif
};

#endif // COMM_H
