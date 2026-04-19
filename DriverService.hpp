#pragma once
#include <string>
#include <windows.h>
#include "Utils.hpp"

class DriverService final
{
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

	NTSTATUS RegisterAndStart();

	NTSTATUS StopAndUnregister();


private:
	std::string m_driverPath;
	std::string m_serviceName;
	HANDLE m_hService{ INVALID_HANDLE_VALUE };
};

