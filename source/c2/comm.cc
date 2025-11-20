#include "comm.h"
#include <iostream>

#pragma comment(lib, "winhttp.lib")

Comm::Comm(const std::wstring& userAgent) : userAgent(userAgent), hSession(NULL), hConnect(NULL), hRequest(NULL) {}

Comm::~Comm() {
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
}

bool Comm::Initialize() {
    hSession = WinHttpOpen(userAgent.c_str(),  
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, 
                           WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) return false;

    if (!proxy.empty()) {
        WINHTTP_PROXY_INFO proxyInfo;
        proxyInfo.dwAccessType = WINHTTP_ACCESS_TYPE_NAMED_PROXY;
        proxyInfo.lpszProxy = (LPWSTR)proxy.c_str();
        proxyInfo.lpszProxyBypass = NULL;

        if (!WinHttpSetOption(hSession, WINHTTP_OPTION_PROXY, &proxyInfo, sizeof(proxyInfo))) {
            return false;
        }
    }

    return true;
}

void Comm::SetProxy(const std::wstring& proxy) {
    this->proxy = proxy;
}

std::string Comm::SendRequest(const std::wstring& server, int port, const std::wstring& path, const std::wstring& method, const std::string& data) {
    std::string response;

    if (!hSession) {
        if (!Initialize()) return "";
    }

    hConnect = WinHttpConnect(hSession, server.c_str(), port, 0);
    if (!hConnect) return "";

    hRequest = WinHttpOpenRequest(hConnect, method.c_str(), path.c_str(),
                                  NULL, WINHTTP_NO_REFERER, 
                                  WINHTTP_DEFAULT_ACCEPT_TYPES, 
                                  WINHTTP_FLAG_SECURE); // Assuming HTTPS by default

    if (!hRequest) return "";

    BOOL bResults = WinHttpSendRequest(hRequest,
                                       WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                                       (LPVOID)data.c_str(), data.length(),
                                       data.length(), 0);

    if (bResults) {
        bResults = WinHttpReceiveResponse(hRequest, NULL);
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

    return response;
}
