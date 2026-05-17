#pragma once
#include <atomic>
#include <windows.h>
#include "Log.hpp"

template<typename T>
class DriverProvider 
{
public:
	[[nodiscard]] auto Initialize() noexcept
	{
		return static_cast<T*>(this)->Initialize();
	}

	[[nodiscard]] VOID Uninitialize() noexcept
	{
		static_cast<T*>(this)->Uninitialize();
	}

	[[nodiscard]] BOOLEAN Read(PVOID VA, PVOID Buf, ULONG Size)
	{
		return static_cast<T*>(this)->KernelRead(VA, Buf, Size);
	}

	[[nodiscard]] BOOLEAN Write(PVOID VA, PVOID Buf, ULONG Size)
	{
		return static_cast<T*>(this)->KernelWrite(VA, Buf, Size);
	}
};

