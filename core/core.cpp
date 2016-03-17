#include <map>
#include <iostream>

#include "core/coda.hpp"
#include "core/core.hpp"
#include "core/file_util.hpp"
#include "core/global.hpp"
#include "docopt.cpp/docopt.h"
#include "spdlog/spdlog.h"
static const char USAGE[] =
R"(Core of CODA.

   Usage:
     core gen <meta file path>
     core encrypt <input file path> <output dir path> <meta file path>
     core decrypt <input file path> <output dir path> <meta file path>

   Options:
     -h --help  Show Help.
     --version  Show Version.
)";

extern std::shared_ptr<spdlog::logger> _console;

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
	return core::genKeypair(protocol, metaFilePath);
    case core::Protocol::PROT_CON:
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
        if (!genKeypair(metaFilePath))
	    return -1;
    } else if (args["encrypt"].asBool()) {
	auto inputFilePath = args["<input file path>"].asString();
	auto outputFilePath = args["<output dir path>"].asString();
	auto metaFilePath = args["<meta file path>"].asString();
	if (!core::encrypt(inputFilePath, outputFilePath, metaFilePath)) {
	    L_ERROR(_console, "Something went wrong in the encryption\n");
	    return -1;
	}
    } else if (args["decrypt"].asBool()) {
	auto inputFilePath = args["<input file path>"].asString();
	auto outputFilePath = args["<output dir path>"].asString();
	auto metaFilePath = args["<meta file path>"].asString();
	if (!core::decrypt(inputFilePath, outputFilePath, metaFilePath)) {
	    L_ERROR(_console, "Something went wrong in the decryption\n");
	    return -1;
	}
    }
    return 0;
}
