#include <spdlog/spdlog.h>
#include <core/global.hpp>
#include <core/protocol.hpp>
#include <fstream>
#include <core/literal.hpp>
#include <core/file_util.hpp>

namespace protocol {
namespace contingency {
const core::FHEArg _fheArgs = {64, 1031, 2, 5};

struct ProtocolHeader {
    std::string protocol;
    std::vector<long> attrDomain;
};

static bool encrypt(std::fstream &fin,
                    const std::string &outputDirPath,
                    const ProtocolHeader &header,
                    core::pk_ptr pk) {
    return true;
}

static bool parseHeader(ProtocolHeader *header, std::fstream &fin) {
    std::string line;
    std::getline(fin, line);
    if (line.compare("#protocol PROT_CON") != 0)
        return false;
    header->protocol = "PROT_CON";

    std::getline(fin, line);
    size_t pos = line.find("#meta ");
    if (pos != 0UL)
        return false;

    line = line.substr(5);
    auto fields = util::splitBySpace(line);
    for (auto &f : fields) {
        header->attrDomain.push_back(literal::stol(f, &pos, 10));
        if (pos != f.size()) {
            L_WARN(global::_console, "Invalid meta field in header");
            return false;
        }
    }
    return true;
}

bool encrypt(const std::string &inputFilePath,
             const std::string &outputDirPath,
             core::pk_ptr pk) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
        L_WARN(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }

    ProtocolHeader header;
    if (!parseHeader(&header, fin)) {
        L_ERROR(global::_console, "Error happened in parsing {0}", inputFilePath);
        return false;
    }

    return encrypt(fin, outputDirPath, header, pk);
}

bool evaluate(const std::vector<std::string> &inputDirs,
              const std::string &outputDir,
              core::pk_ptr pk) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
}

bool decrypt(const std::string &inputFilePath,
             const std::string &outputFilePath,
             core::pk_ptr pk,
             core::sk_ptr sk) {
    L_INFO(global::_console, "Not implemented yet");
    return false;
}
} // namespace contingency
} // namespace protocol
