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
     core enc <input file path> <output dir path> <meta file path> [-l type]
     core dec <input file path> <output dir path> <meta file path>
     core eva <data dir path> <output dir path> <meta file path> -p <protocol> [<data> ...]

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

int main(int argc, char *argv[]) {
    std::map<std::string, docopt::value> args =
            docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "coda core 1.0");
    if (args["gen"].asBool()) {
        core::setProtocol(args["<meta file path>"].asString());
        if (!core::genKeypair()) {
            L_ERROR(_console, "Something went wrong in the key-generation");
            return -1;
        }
    } else if (args["enc"].asBool()) {
        auto inputFilePath = args["<input file path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();
        core::setProtocol(metaFilePath);
        bool local_compute = false;
        if (args["--local"]) local_compute = true;

        if (!core::encrypt(inputFilePath, outputFilePath, metaFilePath, local_compute)) {
            L_ERROR(_console, "Something went wrong in the encryption");
            return -1;
        }
    } else if (args["dec"].asBool()) {
        auto inputFilePath = args["<input file path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();
        core::setProtocol(metaFilePath);
        if (!core::decrypt(inputFilePath, outputFilePath, metaFilePath)) {
            L_ERROR(_console, "Something went wrong in the decryption");
            return -1;
        }
    } else if (args["eva"].asBool()) {
        auto dataDirPath = args["<data dir path>"].asString();
        auto outputFilePath = args["<output dir path>"].asString();
        auto metaFilePath = args["<meta file path>"].asString();
        core::setProtocol(metaFilePath);

        std::vector<std::string> params;
        if (args["<data>"])
           params = args["<data>"].asStringList();

        if (!core::evaluate(dataDirPath, outputFilePath, metaFilePath, params)) {
            L_ERROR(_console, "Somthing went wrong in the evaluataion");
            return -1;
        }
    }
    return 0;
}
