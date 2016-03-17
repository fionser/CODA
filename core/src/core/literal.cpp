#include "core/literal.hpp"
namespace coda {
#ifdef OS_WIN
long stol(const std::string &str, std::size_t *pos, int base) {
	long ret = 0;
	size_t len = str.size();
	for (size_t i = 0; i < len; i++) {
		ret = ret * 10 + long(str.c_str()[len - i] - '0');
	}
	return ret;
}
#else
long stol(const std::string &str, std::size_t *pos, int base) {
    return std::stol(str, pos, base);
}
#endif
} // namespace coda
