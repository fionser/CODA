//
// Created by riku on 2016/08/22.
//
#include <iostream>
#include <map>
#include "docopt.cpp/docopt.h"
static const char USAGE[] =
        R"(Test docopt

   Usage:
     docopt gen <meta file path>
     docopt encrypt [-l type] <input file path> <output file path> <meta file path>
     docopt eva -p PROTOCOL [<data>...]

   Arguments:
     data Input arguments for protocol

   Options:
     -h --help  Show Help.
     -l --local type Set local computation
     -p --protocol Set protocol
     --version  Show Version.
)";

bool testGen() {
    const char *argv[] = {"docopt", "gen", "AA"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (!args["gen"].asBool()) return false;
    if (args["<meta file path>"].asString().compare("AA") != 0) return false;
    return true;
}

bool testOption() {
    const char *argv[] = {"docopt", "encrypt", "AA", "BB", "CC"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (!args["encrypt"].asBool()) return false;
    if (args["<input file path>"].asString().compare("AA") != 0) return false;
    if (args["<output file path>"].asString().compare("BB") != 0) return false;
    if (args["<meta file path>"].asString().compare("CC") != 0) return false;
    return true;
}

bool testLocalOption() {
    const char *argv[] = {"docopt", "encrypt", "AA", "BB", "CC", "--local", "ct"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (!args["encrypt"].asBool()) return false;
    if (args["<input file path>"].asString().compare("AA") != 0) return false;
    if (args["<output file path>"].asString().compare("BB") != 0) return false;
    if (args["<meta file path>"].asString().compare("CC") != 0) return false;
    if (args["--local"].asString().compare("ct") != 0) return false;
    return true;
}

bool testVariables() {
    const char *argv[] = {"docopt", "eva", "-p", "PROT_CON", "1", "2"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (args["--protocol"].asString().compare("PROT_CON") != 0) return false;
    auto sList = args["<data>"].asStringList();
    for (auto s : sList)
        std::cout << s << "\n";
    return true;
}

int main(int argc, char *argv[]) {
    if (!testGen()) std::cerr << "testGen failed\n";
    if (!testOption()) std::cerr << "testOption failed\n";
    if (!testLocalOption()) std::cerr << "testLocalOption failed\n";
    if (!testVariables()) std::cerr << "testVariables failed\n";
    return 0;
}
