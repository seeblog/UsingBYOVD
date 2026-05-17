#pragma once
#include <windows.h>
#include <string>
#include "DriverProvider.hpp"
#include "DriverService.hpp"
#include "Singleton.hpp"

class BiosToolCommonDriver final : 
	public DriverProvider<BiosToolCommonDriver>,
	public Singleton<BiosToolCommonDriver>
{
	friend class Singleton<BiosToolCommonDriver>;
private:
	static constexpr ULONG IOCTL_READ_PHYSICAL		= 0x22202Cu;
	static constexpr ULONG IOCTL_WRITE_PHYSICAL		= 0x222030u;
	static constexpr ULONG IOCTL_VIRTUAL2PHYSICAL	= 0x222034u;
public:
	explicit BiosToolCommonDriver(Token) noexcept : BiosToolCommonDriver()
	{
	}	
	~BiosToolCommonDriver() = default;
	
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
	BiosToolCommonDriver() = default;

private:
	BOOLEAN
		ReadPhysicalMemory(
			PVOID	PhysicalAddress,
			ULONG	Size,
			PVOID	ReadBuffer);

	BOOLEAN
		WritePhysicalMemory(
			PVOID	PhysicalAddress,
			ULONG	Size,
			PVOID	WriteBuffe);

	PVOID
		VirtualToPhysical(PVOID VirtualAddress);

	HANDLE
		CreateDevice(const char* DeviceName);

private:
	HANDLE			m_hDevice{ INVALID_HANDLE_VALUE };
	DriverService*  m_pDriverService{ nullptr };
	BOOLEAN			m_bInitialized{ FALSE };
};


struct BiosToolCommonDriverProxy final
{
	constexpr BiosToolCommonDriverProxy() noexcept = default;

	BiosToolCommonDriverProxy(const BiosToolCommonDriverProxy&) = delete;
	BiosToolCommonDriverProxy& operator=(const BiosToolCommonDriverProxy&) = delete;
	BiosToolCommonDriverProxy(BiosToolCommonDriverProxy&&) = delete;
	BiosToolCommonDriverProxy& operator=(BiosToolCommonDriverProxy&&) = delete;

	BiosToolCommonDriver* operator->() const noexcept
	{
		return std::addressof(BiosToolCommonDriver::Instance());
	}
};

inline constexpr BiosToolCommonDriverProxy g_BiosToolCommonDriver{};