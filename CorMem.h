#pragma once
#include <windows.h>
#include <string>
#include "DriverProvider.hpp"
#include "DriverService.hpp"
#include "Singleton.hpp"

class CorMem final : 
	public DriverProvider<CorMem>,
	public Singleton<CorMem>
{
	friend class Singleton<CorMem>;

private:
	static constexpr ULONG IOCTL_MAP	= 0x22200Cu;
	static constexpr ULONG IOCTL_UNMAP	= 0x222010u;
	static constexpr ULONG IOCTL_V2P	= 0x22201Cu;

public:
	explicit CorMem(Token) noexcept : CorMem()
	{
	}
	~CorMem() = default;
	
	BOOLEAN Initialize() noexcept;
	VOID Uninitialize();

	BOOLEAN 
	KernelRead(PVOID	VirtualAddress,
			   PVOID	ReadBuffer,
			   SIZE_T	ReadSize);

	BOOLEAN 
	KernelWrite(PVOID	VirtualAddress,
				PVOID	WriteBuffer,
				SIZE_T	WriteSize);
	
private:
	CorMem() = default;

private:
	PVOID		
	MapPhysicalMemory(
		PVOID	PhysicalAddress,
		SIZE_T	Size);

	VOID		
	UnmapPhysicalMemory(PVOID MappedAddress);

	PVOID		
	VirtualToPhysical(PVOID VirtualAddress);


	HANDLE
	CreateDevice(const char* DeviceName);

private:
	HANDLE			m_hDevice{INVALID_HANDLE_VALUE};
	BOOLEAN			m_bInitialized{ FALSE };
	DriverService*	m_pDriverService{ nullptr };
};

struct CorMemProxy final
{
	constexpr CorMemProxy() noexcept = default;

	CorMemProxy(const CorMemProxy&) = delete;
	CorMemProxy& operator=(const CorMemProxy&) = delete;
	CorMemProxy(CorMemProxy&&) = delete;
	CorMemProxy& operator=(CorMemProxy&&) = delete;

	CorMem* operator->() const noexcept
	{
		return std::addressof(CorMem::Instance());
	}
};

inline constexpr CorMemProxy g_CorMem{};