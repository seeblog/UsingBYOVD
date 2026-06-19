#include "Ardrv.h"


BOOLEAN Ardrv::KillProcess(ULONG Pid)
{
	return DeviceIoControl(m_hDevice, 
						   IOCTL_KILL_PROCESS, 
						   &Pid, 
						   sizeof(Pid), 
						   nullptr, 
						   0);
}

