#include "core/literal.hpp"
#include "core/global.hpp"
namespace literal {
#ifdef OS_WIN
#include <stack>
#include <locale>
long stol(const std::string &str, std::size_t *pos, int base) {
	size_t len = str.size();
	const char *cstr = str.c_str();
	std::stack<char> st;
	for (size_t i = 0; i < len && !std::issapce(cstr[i]); i++)
	    st.push(cstr[i]);

	if (pos)
	    *pos = st.size();
	long ret = 0;
	while (!st.empty()) {
	    ret = ret * base + (long)(st.top() - '0');
	    st.pop();
	}

	return ret;
}
std::string separator = "\\";
#else
long stol(const std::string &str, std::size_t *pos, int base) {
    std::size_t _pos = 0;
    if (pos == nullptr) {
        long ret = std::stol(str, &_pos, base);
        if (str.empty() || _pos != str.size())
            L_WARN(global::_console, "Invalid string {0} to convert", str);
        return ret;
    } else {
        return std::stol(str, pos, base);
    }
}

std::string separator = "/";
#endif // OS_WIN
} // namespace literal
