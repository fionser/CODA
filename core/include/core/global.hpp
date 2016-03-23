#ifndef CORE_GLOBAL_HPP
#define CORE_GLOBAL_HPP
#include <memory>
#include <string>

#include "spdlog/spdlog.h"
namespace global {
extern std::shared_ptr<spdlog::logger> _console;
extern std::string _doneFileName;
} // namespace global
#endif
