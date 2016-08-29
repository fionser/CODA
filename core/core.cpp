#include <map>
#include <iostream>

#include "core/coda.hpp"
#include "core/core.hpp"
#include "core/file_util.hpp"
#include "core/global.hpp"
#include "docopt.cpp/docopt.h"

static const char USAGE[] =
        R"(Core of CODA.

   Usage:
     core gen <meta file path>
     core enc [-l type] <input file path> <output dir path> <meta file path>
     core dec <input file path> <output dir path> <meta file path>
     core eva <session dir path> <output dir path> <meta file path> -p <protocol> [<data> ...]

   Options:
     -h --help  Show Help.
     -l --local Set Local Computation
     -p --protocol Set Protocol
     --version  Show Version.
)";

namespace global {
extern std::shared_ptr<spdlog::logger> _console;
}
using namespace global;

static bool genKeypair(const std::string &metaFilePath) {
    util::Meta meta;
    bool ok;
    std::tie(meta, ok) = util::readMetaFile(metaFilePath);
    if (!ok) {
        std::cerr << "Warn! the can't not open the meta file " + metaFilePath << "\n";
        return false;
    }

    core::Protocol protocol;
    if (meta.find("protocol") == meta.end()) {
        L_ERROR(_console, "No protocol type is set! Please check file \"{0}\"", metaFilePath);
        return false;
    } else {
        auto protocols = meta["protocol"];
        if (protocols.empty()) {
            L_ERROR(_console, "No protocol type is set! Please check file \"{0}\"", metaFilePath);
            return false;
        }
        protocol = core::getProtocol(protocols.front());
    }

    switch (protocol) {
    case core::Protocol::PROT_CI2:
    case core::Protocol::PROT_CON:
    case core::Protocol::PROT_MEAN:
        return core::genKeypair(protocol, metaFilePath);
    default:
        L_ERROR(_console, "No protocol type is set! Please check file \"{0}\"", metaFilePath);
        return false;
    }
}

int main(int argc, char *argv[]) {
    std::map<std::string, docopt::value> args =
            docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "coda core 1.0");

    if (args["gen"].asBool()) {
        auto metaFilePath = args["<meta file path>"].asString();
        if (!genKeypair(metaFilePath)) {
            L_ERROR(_console, "Something went wrong in the key-generation");
            return -1;
        }
    } else if (args["enc"].asBool()) {
        auto inputFilePath = args["<input file path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();
        bool local_compute = false;
        if (args["-local"]) local_compute = true;

        if (!core::encrypt(inputFilePath, outputFilePath, metaFilePath, local_compute)) {
            L_ERROR(_console, "Something went wrong in the encryption");
            return -1;
        }
    } else if (args["dec"].asBool()) {
        auto inputFilePath = args["<input file path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();
        if (!core::decrypt(inputFilePath, outputFilePath, metaFilePath)) {
            L_ERROR(_console, "Something went wrong in the decryption");
            return -1;
        }
    } else if (args["eva"].asBool()) {
        auto sessionDirPath = args["<session dir path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();

        std::vector<std::string> params;
        if (args["<data>"])
           params = args["<data>"].asStringList();

        if (!core::evaluate(sessionDirPath, outputFilePath, metaFilePath, params)) {
            L_ERROR(_console, "Somthing went wrong in the evaluataion");
            return -1;
        }
    }
    return 0;
}
