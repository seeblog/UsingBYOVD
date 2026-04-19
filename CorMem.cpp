#include "CorMem.h"
#include <iostream>

BOOLEAN 
CorMem::KernelRead(
	PVOID	VirtualAddress,
	PVOID	ReadBuffer,
	SIZE_T	ReadSize)
{
	if (INVALID_HANDLE_VALUE == m_hCorMem)
	{
		std::cout << "[-] CorMem device is not available." << std::endl;
		return FALSE;
	}

	if (!VirtualAddress || !ReadBuffer || !ReadSize)
	{
		std::cout << "[-] Invalid parameters for KernelRead." << std::endl;
		return FALSE;
	}

	PVOID pPhysicalAddress = VirtualToPhysical(VirtualAddress);
	if (!pPhysicalAddress)
	{
		// 测试在Windows11 26200 存在错误 
		std::cout << "[-] Failed to translate virtual address to physical address." << std::endl;
		return FALSE;
	}

	auto pMappedAddress = MapPhysicalMemory(pPhysicalAddress, ReadSize);
	if (!pMappedAddress)
	{
		std::cout << "[-] Failed to map physical memory With Read. Error code: " << GetLastError() << std::endl;
		return FALSE;
	}

	RtlCopyMemory(ReadBuffer, pMappedAddress, ReadSize);
	UnmapPhysicalMemory(pMappedAddress);

	return TRUE;
}

BOOLEAN
CorMem::KernelWrite(
	PVOID VirtualAddress, 
	PVOID WriteBuffer, 
	SIZE_T WriteSize)
{
	if (!VirtualAddress || !WriteBuffer || !WriteSize)
	{
		std::cout << "[-] Invalid parameters for KernelWrite." << std::endl;
		return FALSE;
	}

	PVOID pPhysicalAddress = VirtualToPhysical(VirtualAddress);
	if (!pPhysicalAddress)
	{
		std::cout << "[-] Failed to translate virtual address to physical address." << std::endl;
		return FALSE;
	}

	auto pMappedAddress = MapPhysicalMemory(pPhysicalAddress, WriteSize);
	if (!pMappedAddress)
	{
		std::cout << "[-] Failed to map physical memory. Error code: " << GetLastError() << std::endl;
		return FALSE;
	}

	RtlCopyMemory(pMappedAddress, WriteBuffer, WriteSize);
	UnmapPhysicalMemory(pMappedAddress);

	return TRUE;
}

PVOID
CorMem::MapPhysicalMemory(
	PVOID	PhysicalAddress,
	SIZE_T	Size)
{
	struct
	{
		PVOID	PhysicalAddress;
		SIZE_T	Size;
		PVOID	Reserved;
	} Request;
	Request.PhysicalAddress = PhysicalAddress;
	Request.Size = Size;
	Request.Reserved = nullptr;

	PVOID pMappedAddress = nullptr;
	DWORD dwBytesReturned = 0;

	if (DeviceIoControl(m_hCorMem, 
						IOCTL_MAP, 
						&Request, 
						sizeof(Request),
						&pMappedAddress, 
						sizeof(pMappedAddress),
						&dwBytesReturned, 
						nullptr))
	{
		return pMappedAddress;
	}
	return nullptr;
}

VOID 
CorMem::UnmapPhysicalMemory(
	PVOID MappedAddress)
{
	if (!MappedAddress)
	{
		return;
	}

	DeviceIoControl(m_hCorMem, IOCTL_UNMAP, &MappedAddress, sizeof(MappedAddress), nullptr, 0, nullptr, nullptr);
}

PVOID 
CorMem::VirtualToPhysical(PVOID VirtualAddress)
{
	if (!VirtualAddress)
	{
		std::cout << "[-] Invalid virtual address for translation." << std::endl;
		return nullptr;
	}

	PVOID pPhysicalAddress = VirtualAddress;
	DWORD returned = 0;
	auto bRet = DeviceIoControl(m_hCorMem,
								IOCTL_V2P,
								&pPhysicalAddress,
								sizeof(pPhysicalAddress),
								&pPhysicalAddress,
								sizeof(pPhysicalAddress),
								&returned,
								nullptr);
	if (!bRet)
	{
		std::cout << "[-] Failed to translate virtual address to physical address. Error code: " << GetLastError() << std::endl;
		return nullptr;
	}
	else
	{
		if (!pPhysicalAddress)
		{
			std::cout << "[-] Translation returned a null physical address." << std::endl;
			return nullptr;
		}
	}
	return pPhysicalAddress;
}
