#pragma once
#include <iostream>

#ifdef _DEBUG
#define LOG(msg) std::cout << "[*] " << msg << std::endl
#define LOGW(msg) std::wcout << L"[*] " << msg << std::endl
#else
#define LOG(msg)	
#define LOGW(msg)
#endif