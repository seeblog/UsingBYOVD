#pragma once
#include <concepts>
#include <type_traits>
#include <memory>
#include <mutex>

template<typename T>
class Singleton
{
public:
	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

	Singleton(Singleton&&) = delete;
	Singleton& operator=(Singleton&&) = delete;

	struct Token
	{
		explicit Token() = default;
	};

	static T& Instance()
	{ 
		std::call_once(_flag, []() {
			_instance = std::make_unique<T>(Token{});
					   });

		return *_instance;
	}

protected:
	Singleton() = default;
	~Singleton() = default;

private:
	static inline std::once_flag _flag;
	static inline std::unique_ptr<T> _instance;
};

