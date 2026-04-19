#pragma once
#include <filesystem>
#include <string>


namespace FileUtils
{
	bool 
	CreateDriverFile(
		std::string&		driverFullPath, 
		const char*			filedata, 
		const size_t		size, 
		const unsigned char key);

	std::string
	GetServiceName(const unsigned char* data, const size_t size, const unsigned char key);
};

