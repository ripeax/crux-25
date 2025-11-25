#pragma once
#include <Windows.h>
#include <winternl.h>
#include <string>

#pragma comment(lib, "ntdll.lib")

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

#define STATUS_SUCCESS 0x00000000

// Function Pointer Definitions
typedef NTSTATUS(NTAPI* NTQUERYINFORMATIONFILE)(
    HANDLE FileHandle,
    PIO_STATUS_BLOCK IoStatusBlock,
    PVOID FileInformation,
    ULONG Length,
    FILE_INFORMATION_CLASS FileInformationClass
);

typedef NTSTATUS(NTAPI* NTOPENFILE)(
    PHANDLE FileHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    PIO_STATUS_BLOCK IoStatusBlock,
    ULONG ShareAccess,
    ULONG OpenOptions
);

typedef NTSTATUS(NTAPI* NTCLOSE)(
    HANDLE Handle
);

typedef BOOLEAN(NTAPI* RTLDOSPATHNAMETONTPATHNAME_U)(
    PCWSTR DosFileName,
    PUNICODE_STRING NtFileName,
    PWSTR* FilePart,
    PVOID RelativeName
);

typedef NTSTATUS(NTAPI* NTQUERYSYSTEMINFORMATION)(
    SYSTEM_INFORMATION_CLASS SystemInformationClass,
    PVOID SystemInformation,
    ULONG SystemInformationLength,
    PULONG ReturnLength
);

// Struct Definitions
typedef struct _FILE_PROCESS_IDS_USING_FILE_INFORMATION {
    ULONG NumberOfProcessIdsInList;
    ULONG_PTR ProcessIdList[1];
} FILE_PROCESS_IDS_USING_FILE_INFORMATION, *PFILE_PROCESS_IDS_USING_FILE_INFORMATION;

typedef struct _MY_SYSTEM_PROCESS_INFORMATION {
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    BYTE Reserved1[48];
    UNICODE_STRING ImageName;
    KPRIORITY BasePriority;
    HANDLE UniqueProcessId;
    PVOID Reserved2;
    ULONG HandleCount;
    ULONG SessionId;
    PVOID Reserved3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG Reserved4;
    SIZE_T PeakWorkingSetSize;
    SIZE_T WorkingSetSize;
    PVOID Reserved5;
    SIZE_T QuotaPagedPoolUsage;
    PVOID Reserved6;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
    LARGE_INTEGER Reserved7[6];
} MY_SYSTEM_PROCESS_INFORMATION, *PMY_SYSTEM_PROCESS_INFORMATION;

// Helpers
inline HMODULE GetModuleHandleEx2W(LPCWSTR lpModuleName) {
    return GetModuleHandleW(lpModuleName);
}

inline HANDLE GetProcessHeapFromTeb() {
    return GetProcessHeap();
}

inline BOOL IsPathValidW(LPCWSTR path) {
    return (path != NULL && path[0] != L'\0');
}

inline int StringLengthA(LPCSTR str) {
    return (int)strlen(str);
}

inline int StringCompareW(LPCWSTR s1, LPCWSTR s2) {
    return wcscmp(s1, s2); // Returns 0 (ERROR_SUCCESS) if equal
}

inline DWORD HandleToLong(HANDLE h) {
    return (DWORD)(ULONG_PTR)h;
}

inline int CharStringToWCharString(WCHAR* dest, char* src, int len) {
    return MultiByteToWideChar(CP_ACP, 0, src, -1, dest, len + 1);
}
