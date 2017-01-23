#include "core/encoder.hpp"
#include "core/contingency_table.hpp" // Attribute
namespace encoder {
bool staircase::encode(std::vector<long> &ret,
                       const core::Attribute &attr,
                       const long value) {
    if (value > attr.size)
        return false;
    ret.resize(attr.size + 1);
    for (long i = value; i <= attr.size; i++)
        ret.at(i) = 1;
    return true;
}
}
