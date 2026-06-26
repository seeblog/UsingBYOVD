#include "IMFForceDelete123.h"
#include <string>

BOOLEAN 
IMFForceDelete123::KernelDeleteFile(PWCHAR FilePath)
{
	std::wstring filePath(FilePath);
	const std::wstring prefix = L"\\??\\";

	if (filePath.rfind(prefix, 0) != 0)
	{
		filePath = prefix + filePath;
	}


	LOGW("[+] Deleting file: " << std::wstring(filePath.begin(), filePath.end()) << std::endl);

	return DeviceIoControl(m_hDevice,
						   IOCTL_FORCE_DELETEFILE, 
						   (LPVOID)filePath.c_str(), 
						   filePath.length() * sizeof(wchar_t), 
						   nullptr, 
						   0);

}