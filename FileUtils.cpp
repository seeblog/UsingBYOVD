#include "FileUtils.hpp"
#include <vector>
#include <fstream>
#include <array>
#include <random>

#include "Log.hpp"

#if _DEBUG
#include <iostream>
#endif 

static
std::string 
CreateRandomFileName(const size_t length = 16)
{
	static const char letters[] =
		"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	static std::random_device rd;
	static std::mt19937 gen(rd());                  
	std::uniform_int_distribution<> dis(0, sizeof(letters) - 2);

	std::string name;
	name.reserve(length);

	for (size_t i = 0; i < length; ++i)
	{
		name += letters[dis(gen)];
	}

	return name;
}

static
std::string 
CreateDriverFilePath()
{
	namespace fs = std::filesystem;

	try
	{
		// Get tmp path
		auto tempPath = fs::temp_directory_path();
		if (tempPath.empty())
		{
			LOG("[-] Failed to get temporary directory path");
			return {};
		}

		// get random file name
		std::string strRandomFileName = CreateRandomFileName(12);
		if (strRandomFileName.empty())
		{
			strRandomFileName = "tempfile_" + std::to_string(std::time(nullptr));
		}

		// get full name
		auto fullPath = tempPath / strRandomFileName;
		fullPath.replace_extension(".sys");


		return fullPath.string();
	}
	catch (const std::exception& e [[maybe_unused]])
	{
#if _DEBUG
		LOG("[-] Exception in CreateDriverFilePath: " << e.what());
#endif
		return {};
	}
}

static
bool 
CreateFileFromMemory(
	const std::string_view& filePath,
	const char*				data, 
	const size_t			size, 
	const unsigned char		key)
{
	if (filePath.empty() || data == nullptr || size == 0)
	{
		LOG("[-] Invalid parameters for CreateFileFromMemory");
		return false;
	}

	std::vector<char> buffer(data, data + size);

	for (auto& vecData : buffer)
	{
		vecData ^= key;
	}

	std::ofstream outFile(filePath.data(), std::ios::binary | std::ios::out);
	if (!outFile.is_open())
	{
		LOG("[-] Failed to create file: " << filePath);
		return false;
	}

	outFile.write(buffer.data(), buffer.size());
	if (!outFile.good())
	{
		LOG("[-] Failed to write data to file: " << filePath);
		outFile.close();
		return false;
	}

	outFile.close();
	
	return true;
}

bool FileUtils::CreateDriverFile(
	std::string&		driverFullPath,
	const char*			filedata, 
	const size_t		size, 
	const unsigned char key)
{
	auto tmpPath = CreateDriverFilePath();
	if (tmpPath.empty())
	{
		LOG("[-] Failed to create driver file path");
		return false;
	}
	

	driverFullPath = tmpPath;
	return CreateFileFromMemory(tmpPath, filedata, size, key);
}

std::string 
FileUtils::GetServiceName(
	const unsigned char*	data, 
	const size_t			size, 
	const unsigned char		key)
{
	if (data == nullptr || size == 0)
	{
		return {};
	}

	std::vector<unsigned char> buffer(data, data + size);
	for (auto& vecData : buffer)
	{
		vecData ^= key;
	}
	return std::string(buffer.begin(), buffer.end());
}
