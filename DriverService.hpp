#pragma once
#include <string>
#include "Utils.hpp"

class DriverService final
{
	static constexpr ULONG SE_LOAD_DRIVER_PRIVILEGE = 10UL;
public:	
	DriverService(const std::string& driverPath, const std::string& serviceName) : 
		m_driverPath(driverPath), 
		m_serviceName(serviceName) 
	{

	}

	~DriverService()
	{

	}

	DriverService(const DriverService&) = delete;
	DriverService& operator=(const DriverService&) = delete;

	DriverService(DriverService&&) = delete;
	DriverService& operator=(DriverService&&) = delete;

	NTSTATUS RegisterService();
	NTSTATUS LoadDriver();
	NTSTATUS StopAndUnregister();


private:
	NTSTATUS EnablePrivilege();
	
	NTSTATUS LoadDriver(PUNICODE_STRING DriverServiceName);
	
	NTSTATUS UnloadDriver(PUNICODE_STRING DriverServiceName);
	

private:
	std::string m_driverPath;
	std::string m_serviceName;

	std::wstring m_wRegDriverPath;
	std::wstring m_wRegServicePath;

	BOOLEAN m_bIsRegisted{ FALSE };
	HANDLE m_hService{ INVALID_HANDLE_VALUE };
};

