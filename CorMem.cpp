#include "CorMem.h"


BOOLEAN 
CorMem::KernelRead(
	PVOID	VirtualAddress,
	PVOID	ReadBuffer,
	SIZE_T	ReadSize)
{
	if (!VirtualAddress || !ReadBuffer || !ReadSize)
	{
		return FALSE;
	}

	PVOID pPhysicalAddress = VirtualToPhysical(VirtualAddress);
	if (!pPhysicalAddress)
	{
		return FALSE;
	}

	auto PageOffset = reinterpret_cast<ULONG_PTR>(pPhysicalAddress) & (0x1000 - 1);
	auto PageBase	= reinterpret_cast<ULONG_PTR>(pPhysicalAddress) & ~(0x1000 - 1);
	auto MapSize	= (PageOffset + ReadSize + 0x1000 - 1) & ~(0x1000 - 1);

	auto pMappedAddress = MapPhysicalMemory(reinterpret_cast<PVOID>(PageBase), MapSize);
	if (!pMappedAddress)
	{
		return FALSE;
	}

	RtlCopyMemory(ReadBuffer, reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(pMappedAddress) + PageOffset), ReadSize);
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
		return FALSE;
	}

	PVOID pPhysicalAddress = VirtualToPhysical(VirtualAddress);
	if (!pPhysicalAddress)
	{
		return FALSE;
	}

	auto PageOffset = reinterpret_cast<ULONG_PTR>(pPhysicalAddress) & (0x1000 - 1);
	auto PageBase	= reinterpret_cast<ULONG_PTR>(pPhysicalAddress) & ~(0x1000 - 1);
	auto MapSize	= (PageOffset + WriteSize + 0x1000 - 1) & ~(0x1000 - 1);

	auto pMappedAddress = MapPhysicalMemory(reinterpret_cast<PVOID>(PageBase), MapSize);
	if (!pMappedAddress)
	{
		return FALSE;
	}

	RtlCopyMemory(reinterpret_cast<PVOID>(reinterpret_cast<ULONG_PTR>(pMappedAddress) + PageOffset), WriteBuffer, WriteSize);
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
		return nullptr;
	}

	PVOID pPhysicalAddress = VirtualAddress;
	DWORD returned = 0;
	DeviceIoControl(m_hCorMem, 
					IOCTL_V2P, 
					&pPhysicalAddress,
					sizeof(pPhysicalAddress), 
					&pPhysicalAddress, 
					sizeof(pPhysicalAddress), 
					&returned, 
					nullptr);
	return pPhysicalAddress;
}
