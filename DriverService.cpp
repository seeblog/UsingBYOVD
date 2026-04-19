#include "DriverService.hpp"
#include "Log.hpp"
#include <string>
#include <format>
#include <memory>
#include <filesystem>

namespace fs = std::filesystem;

NTSTATUS DriverService::RegisterAndStart()
{
	if (m_serviceName.empty() || m_driverPath.empty())
	{
		LOG("[-] Service name or driver path is empty" << std::endl);
		return STATUS_INVALID_PARAMETER;
	}

	const std::wstring serviceName = std::wstring(m_serviceName.begin(), m_serviceName.end());

	// Build registry paths using filesystem for clean and safe concatenation
	const fs::path servicesPath = fs::path(L"SYSTEM") / L"CurrentControlSet" / L"Services" / serviceName;
	const fs::path regPath = fs::path(L"\\Registry\\Machine\\System") / L"CurrentControlSet" / L"Services" / serviceName;

	// ====================== RAII Registry Key Guard ======================
	struct RegKeyDeleter
	{
		using pointer = HKEY;
		void operator()(HKEY hKey) const noexcept
		{
			if (hKey)
			{
				RegCloseKey(hKey);
			}
		}
	};

	std::unique_ptr<HKEY, RegKeyDeleter> hDriverService;

	HKEY hRaw = nullptr;
	LSTATUS status = RegCreateKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &hRaw);
	if (status != ERROR_SUCCESS)
	{
		LOG(std::format("[-] Failed to create service key (0x{:X})", status) << std::endl);
		return STATUS_REGISTRY_IO_FAILED;
	}
	hDriverService.reset(hRaw);

	const DWORD ServiceTypeKernel	= 1;  // SERVICE_KERNEL_DRIVER
	const DWORD ServiceErrorControl = 1;  // SERVICE_ERROR_NORMAL
	const DWORD ServiceStartDemand	= 3;  // SERVICE_DEMAND_START

	// Build ImagePath with \??\ prefix
	const fs::path driverPath = m_driverPath;
	const std::wstring imagePath = LR"(\??\)" + driverPath.wstring();

	// ImagePath
	status = RegSetKeyValueW(hDriverService.get(), nullptr, L"ImagePath", REG_EXPAND_SZ,
							 imagePath.c_str(),
							 static_cast<DWORD>((imagePath.size() + 1) * sizeof(wchar_t)));

	if (status != ERROR_SUCCESS)
	{
		LOG(std::format("[-] Failed to set ImagePath (0x{:X})", status) << std::endl);
		RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		return STATUS_REGISTRY_IO_FAILED;
	}

	// Required registry values
	RegSetKeyValueW(hDriverService.get(), nullptr, L"Type", REG_DWORD, &ServiceTypeKernel, sizeof(DWORD));
	RegSetKeyValueW(hDriverService.get(), nullptr, L"ErrorControl", REG_DWORD, &ServiceErrorControl, sizeof(DWORD));
	RegSetKeyValueW(hDriverService.get(), nullptr, L"Start", REG_DWORD, &ServiceStartDemand, sizeof(DWORD));

	// ====================== Load Driver ======================
	if (GetModuleHandleW(L"ntdll.dll") == nullptr)
	{
		RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		return STATUS_UNSUCCESSFUL;
	}

	constexpr ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
	BOOLEAN wasEnabled = FALSE;

	NTSTATUS ntStatus = Utils::RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &wasEnabled);
	if (!NT_SUCCESS(ntStatus))
	{
		RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		LOG(std::format("[-] Failed to acquire SeLoadDriverPrivilege (0x{:X})", ntStatus) << std::endl);
		return ntStatus;
	}

	UNICODE_STRING serviceStr{};
	RtlInitUnicodeString(&serviceStr, regPath.c_str());

	ntStatus = Utils::NtLoadDriver(&serviceStr);

	if (ntStatus == STATUS_OBJECT_NAME_COLLISION)
	{
		std::wcout << L"[-] Driver already loaded, attempting to unload first...\n";
		Utils::NtUnloadDriver(&serviceStr);
		ntStatus = Utils::NtLoadDriver(&serviceStr);
		std::wcout << std::format(L"[+] NtLoadDriver after unload: 0x{:X}\n", ntStatus);
	}

	if (!NT_SUCCESS(ntStatus))
	{
		std::wcout << L"[-] Load failed, cleaning registry...\n";
		RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
	}

	return ntStatus;
}

NTSTATUS DriverService::StopAndUnregister()
{
	if (GetModuleHandleW(L"ntdll.dll") == nullptr)
	{
		return STATUS_UNSUCCESSFUL;
	}

	const std::wstring serviceName = std::wstring(m_serviceName.begin(), m_serviceName.end());

	// Build paths using filesystem
	const fs::path servicesPath = fs::path(L"SYSTEM") / L"CurrentControlSet" / L"Services" / serviceName;
	const fs::path regPath = fs::path(L"\\Registry\\Machine\\System") / L"CurrentControlSet" / L"Services" / serviceName;

	// Check if service exists
	HKEY hKey = nullptr;
	LSTATUS status = RegOpenKeyW(HKEY_LOCAL_MACHINE, servicesPath.c_str(), &hKey);
	if (status != ERROR_SUCCESS)
	{
		if (status == ERROR_FILE_NOT_FOUND)
		{
			return STATUS_SUCCESS;
		}
		else
		{
			return STATUS_REGISTRY_IO_FAILED;
		}
	}
	RegCloseKey(hKey);

	// ====================== Unload Driver ======================
	constexpr ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
	BOOLEAN wasEnabled = FALSE;

	NTSTATUS ntStatus = Utils::RtlAdjustPrivilege(SE_LOAD_DRIVER_PRIVILEGE, TRUE, FALSE, &wasEnabled);
	if (!NT_SUCCESS(ntStatus))
	{
		LOG(std::format("[-] Failed to acquire SeLoadDriverPrivilege (0x{:X}). Run as Administrator!", ntStatus)
			<< std::endl);
		return ntStatus;
	}

	UNICODE_STRING serviceStr{};
	RtlInitUnicodeString(&serviceStr, regPath.c_str());

	ntStatus = Utils::NtUnloadDriver(&serviceStr);

	if (!NT_SUCCESS(ntStatus))
	{
		LOG("[-] Driver Unload Failed!!" << std::endl);
		RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
		return ntStatus;
	}

	status = RegDeleteTreeW(HKEY_LOCAL_MACHINE, servicesPath.c_str());
	if (status != ERROR_SUCCESS)
	{
		return STATUS_REGISTRY_IO_FAILED;
	}

	return ntStatus;
}