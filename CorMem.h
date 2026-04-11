#pragma once

#include <windows.h>

constexpr ULONG IOCTL_MAP	= 0x22200Cu;
constexpr ULONG IOCTL_UNMAP = 0x222010u;
constexpr ULONG IOCTL_V2P	= 0x22201Cu;

class CorMem
{
public:
	CorMem()
	{
		m_hCorMem = CreateFileA("\\\\.\\CorMem", 
								GENERIC_READ | GENERIC_WRITE,
								0, 
								nullptr, 
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL, 
								nullptr);

		if (!m_hCorMem)
		{
			//std::cerr << "[-] Failed to open CorMem device. Error code: " << GetLastError() << std::endl;
			m_hCorMem = INVALID_HANDLE_VALUE;
		}
	}

	~CorMem()
	{
		if (m_hCorMem != INVALID_HANDLE_VALUE)
		{
			CloseHandle(m_hCorMem);
			m_hCorMem = INVALID_HANDLE_VALUE;
		}
	}


	BOOLEAN 
	KernelRead(PVOID	VirtualAddress,
			   PVOID	ReadBuffer,
			   SIZE_T	ReadSize);

	BOOLEAN 
	KernelWrite(PVOID	VirtualAddress,
				PVOID	WriteBuffer,
				SIZE_T	WriteSize);
	
private:
	PVOID	MapPhysicalMemory(PVOID PhysicalAddress, SIZE_T Size);
	VOID	UnmapPhysicalMemory(PVOID MappedAddress);
	PVOID	VirtualToPhysical(PVOID VirtualAddress);


private:
	HANDLE m_hCorMem{INVALID_HANDLE_VALUE};
};

