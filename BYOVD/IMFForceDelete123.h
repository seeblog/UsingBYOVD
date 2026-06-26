#pragma once
#include <windows.h>
#include <string>
#include "DriverService.hpp"
#include "Singleton.hpp"
#include "ObjectProxy.hpp"
#include "ForceDeleteFile.hpp"
#include "IMFForceDelete123Bin.hpp"

class IMFForceDelete123 final :
	public ForceDeleteFile<IMFForceDelete123>,
	public Singleton<IMFForceDelete123>
{
	friend class Singleton<IMFForceDelete123>;

private:
	static constexpr ULONG IOCTL_FORCE_DELETEFILE = 0x8016E000u;

public:
	explicit IMFForceDelete123(Token) noexcept : IMFForceDelete123()
	{
	}
	~IMFForceDelete123() = default;

	BOOLEAN InitDeleteFile() noexcept
	{
		return Initialize(IMFForceDelete123Bin::hexData,
						  IMFForceDelete123Bin::hexSize,
						  IMFForceDelete123Bin::service,
						  IMFForceDelete123Bin::serviceLength,
						  IMFForceDelete123Bin::Key);
	}


	BOOLEAN
		KernelDeleteFile(PWCHAR FilePath);

private:
	IMFForceDelete123() = default;
};



inline constexpr ObjectProxy<IMFForceDelete123> g_IMFForceDelete123{};