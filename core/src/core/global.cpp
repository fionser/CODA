#include "core/global.hpp"
std::shared_ptr<spdlog::logger> _console = spdlog::stdout_logger_mt("console");
std::string _doneFileName = std::string(".done.lock");
