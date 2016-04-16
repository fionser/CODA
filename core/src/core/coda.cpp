#include <sstream>
#include <HElib/NumbTh.h>

#include "core/coda.hpp"
namespace core {
std::ostream& operator<<(std::ostream &out, const FHEArg &args)
{
    out << "[";
    out << args.m << " ";
    out << args.p << " ";
    out << args.r << " ";
    out << args.L << "]\n";
    return out;
}

std::istream& operator>>(std::istream &in, FHEArg &args)
{
    seekPastChar(in, '[');
    in >> args.m;
    in >> args.p;
    in >> args.r;
    in >> args.L;
    seekPastChar(in, ']');
    return in;
}

std::string toString(Protocol t) {
    switch (t) {
    case Protocol::PROT_CI2:
        return "PROT_CI2";
    case Protocol::PROT_CON:
        return "PROT_CON";
    default:
        return "PROT_UKN";
    }
}

Protocol getProtocol(std::string description) {
    if (description.compare("PROT_CI2") == 0)
        return Protocol::PROT_CI2;

    if (description.compare("PROT_CON") == 0)
        return Protocol::PROT_CON;

    if (description.compare("PROT_MEAN") == 0)
        return Protocol::PROT_MEAN;

    return Protocol::PROT_UKN;
}
} // namespace core
