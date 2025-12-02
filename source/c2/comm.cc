#include "comm.h"
#include <iostream>

#pragma comment(lib, "winhttp.lib")

Comm::Comm(const std::wstring& userAgent) : userAgent(userAgent), hSession(NULL), hConnect(NULL), hRequest(NULL) {
#ifdef DEBUG
    // Simple conversion for debug output to avoid warnings
    std::string debugUA;
    for(wchar_t c : userAgent) debugUA += (char)c;
    
    std::cout << "[Main] Comm initialized with UA: " << debugUA << std::endl;
#endif
}

Comm::~Comm() {
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}

#ifdef DEBUG
void Comm::SetDebugBypass(bool bypass) {
    debug_bypass = bypass;
    std::cout << "[DEBUG] Proxy bypass set to: " << (bypass ? "TRUE" : "FALSE") << std::endl;
}
#endif

bool Comm::Initialize() {
    hSession = WinHttpOpen(userAgent.c_str(),  
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, 
                           WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return false;

#ifdef DEBUG
    if (debug_bypass) {
        std::cout << "[DEBUG] Bypassing proxy configuration." << std::endl;
        return true;
    }
#endif

    if (!proxy.empty()) {
        WINHTTP_PROXY_INFO proxyInfo;
        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        proxyInfo.lpszProxy = (LPWSTR)proxy.c_str();
        proxyInfo.lpszProxyBypass = NULL;

        if (!WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo))) {
#ifdef DEBUG
            std::cerr << "[DEBUG] Failed to set proxy: " << GetLastError() << std::endl;
#endif
            return false;
        }
#ifdef DEBUG
        std::cout << "[DEBUG] Proxy set to: " << std::string(proxy.begin(), proxy.end()) << std::endl;
#endif
    }

    return true;
}

void Comm::SetProxy(const std::wstring& proxy) {
    this->proxy = proxy;
}

std::string Comm::SendRequest(const std::wstring& server, int port, const std::wstring& path, const std::wstring& method, const std::string& data, const std::wstring& headers) {
    std::string response;

#ifdef DEBUG
    std::cout << "[DEBUG] Sending " << std::string(method.begin(), method.end()) 
              << " request to " << std::string(server.begin(), server.end()) 
              << " port " << port << " path " << std::string(path.begin(), path.end()) << std::endl;
#endif

    if (!hSession) {
        if (!Initialize()) return "";
    }

    hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
    if (!hConnect) {
#ifdef DEBUG
        std::cerr << "[DEBUG] WinHttpConnect failed: " << GetLastError() << std::endl;
#endif
        return "";
    }

    DWORD dwFlags = WINHTTP_FLAG_SECURE;
#ifdef DEBUG
    if (port == 8080 || port == 80) {
        dwFlags = 0; // Disable SSL for local debug
        std::cout << "[DEBUG] Disabling SSL for port " << port << std::endl;
    }
#endif

    hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                                  NULL, WINHTTP_NO_REFERER, 
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                  dwFlags);

    if (!hRequest) {
#ifdef DEBUG
        std::cerr << "[DEBUG] WinHttpOpenRequest failed: " << GetLastError() << std::endl;
#endif
        return "";
    }

    LPCWSTR pwszHeaders = headers.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : headers.c_str();
    DWORD dwHeadersLen = headers.empty() ? 0 : headers.length();

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       pwszHeaders, dwHeadersLen,
                                       (LPVOID)data.c_str(), data.length(),
                                       data.length(), 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
    } else {
#ifdef DEBUG
        std::cerr << "[DEBUG] WinHttpSendRequest failed: " << GetLastError() << std::endl;
#endif
    }

    if (bResults) {
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        LPSTR pszOutBuffer;

        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                break;
            }

            if (dwSize == 0) break;

            pszOutBuffer = new char[dwSize + 1];
            if (!pszOutBuffer) {
                break;
            }

            ZeroMemory(pszOutBuffer, dwSize + 1);

            if (WinHttpReadData(hRequest, (LPVOID)pszOutBuffer, dwSize, &dwDownloaded)) {
                response.append(pszOutBuffer, dwDownloaded);
            }

            delete[] pszOutBuffer;
        } while (dwSize > 0);
    }
#ifdef DEBUG
    std::cout << "[DEBUG] Response received (" << response.length() << " bytes)" << std::endl;
#endif

    return response;
}
