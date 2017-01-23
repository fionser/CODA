#ifndef CODA_ENCODER_HPP
#define CODA_ENCODER_HPP
#include <vector>
namespace core {
struct Attribute;
};

namespace encoder {
class staircase {
public:
    static bool encode(std::vector<long> &ret,
                       const core::Attribute &attr,
                       const long value);
private:
    staircase () {}
    ~staircase() {}
    staircase(const staircase &oth) = delete;
    staircase(staircase &&oth) = delete;
    staircase & operator=(const staircase &oth) = delete;
};

} // namespace encoder
#endif // CODA_ENCODER_HPP
