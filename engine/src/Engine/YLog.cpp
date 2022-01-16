#include "Engine/YLog.h"
#include <functional>
std::string g_verbo_log;
std::string g_warning_log;
std::string g_error_log;
std::vector<std::function<void(const std::string& str)>> g_verbo_log_funcs_;
std::vector<std::function<void(const std::string& str)>> g_warning_log_funcs_;
std::vector<std::function<void(const std::string& str)>> g_error_log_funcs_;