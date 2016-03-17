#ifndef CORE_LITERAL_HPP
#define CORE_LITERAL_HPP
#if defined _WIN32 || defined __MINGW32__ || defined __CYGWIN32__
#define OS_WIN 1
#include <windows.h>
#endif
#include <string>
namespace coda {
long stol(const std::string &str, std::size_t *pos = 0, int base = 0);
} // namespace coda
#endif
