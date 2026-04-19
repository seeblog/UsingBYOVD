#pragma once
#include <Windows.h>
#include <winternl.h>
#pragma comment(lib, "ntdll.lib")

#pragma warning(push)
#pragma warning(disable: 4005) //macro redefinition
#include <ntstatus.h>
#pragma warning(pop)


namespace Utils
{
	constexpr auto SystemModuleInformation = 11;

	extern "C" NTSTATUS NtLoadDriver(PUNICODE_STRING DriverServiceName);
	extern "C" NTSTATUS NtUnloadDriver(PUNICODE_STRING DriverServiceName);
	extern "C" NTSTATUS RtlAdjustPrivilege(ULONG Privilege, BOOLEAN Enable, BOOLEAN Client, BOOLEAN* WasEnabled);
}


