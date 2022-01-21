#pragma once
#if defined(_MSC_VER)
#include "windows.h"
#endif
#include <string>
#include <sstream>
#include <iostream>
#include <cassert>
#include <functional>
enum LogType {
	EVerbos = 0,
	EWarning = 1,
	EError = 2,
};
extern std::string g_verbo_log;
extern std::string g_warning_log;
extern std::string g_error_log;
extern std::vector<std::function<void(const std::string& str)>> g_verbo_log_funcs_;
extern std::vector<std::function<void(const std::string& str)>> g_warning_log_funcs_;
extern std::vector<std::function<void(const std::string& str)>> g_error_log_funcs_;

template <typename ...Args>
void MyTraceImplTmp(LogType log_type, int line, const char* fileName, Args&& ...args) {
	std::ostringstream stream;
	std::string log_name;

	switch (log_type)
	{
	case LogType::EVerbos:
		log_name = "[log]";
		break;
	case LogType::EWarning:
		log_name = "[warning]";
		break;
	case LogType::EError:
		log_name = "[error]";
		break;
	}
	stream << log_name;
	switch (log_type)
	{
	case LogType::EVerbos:
		(stream << ... << std::forward<Args>(args)) << '\n';
		g_verbo_log += stream.str();
		for (auto& func : g_verbo_log_funcs_)
		{
			func(stream.str());
		}

		break;
	case LogType::EWarning:
		stream << fileName << "(" << line << ") : ";
		(stream << ... << std::forward<Args>(args)) << '\n';
		g_warning_log += stream.str();
		for (auto& func : g_warning_log_funcs_)
		{
			func(stream.str());
		}
		break;
	case LogType::EError:
		stream << fileName << "(" << line << ") : ";
		(stream << ... << std::forward<Args>(args)) << '\n';
		g_error_log += stream.str();
		for (auto& func : g_error_log_funcs_)
		{
			func(stream.str());
		}
		break;
	}
#	if defined(_MSC_VER)
	OutputDebugStringA(stream.str().c_str());
	std::cout << stream.str();
#endif
# if defined(DEBUG) | defined(_DEBUG)
	if (log_type == LogType::EError)
	{
		//assert(0);
	}
#endif
}

#define LOG_INFO(...) MyTraceImplTmp(LogType::EVerbos,__LINE__, __FILE__, __VA_ARGS__)
#define WARNING_INFO(...) MyTraceImplTmp(LogType::EWarning,__LINE__, __FILE__, __VA_ARGS__)
#define ERROR_INFO(...) MyTraceImplTmp(LogType::EError,__LINE__, __FILE__, __VA_ARGS__)