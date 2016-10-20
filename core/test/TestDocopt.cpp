//
// Created by riku on 2016/08/22.
//
#include <iostream>
#include <map>
#include <sstream>
#include "docopt.cpp/docopt.h"
static const char USAGE[] =
        R"(Test docopt

   Usage:
     docopt gen <meta file path>
     docopt encrypt <input file path> <output file path> <meta file path> [-l type]
     docopt eva -p PROTOCOL [<data>...]

   Arguments:
     data Input arguments for protocol

   Options:
     -h --help  Show Help.
     -l --local type Set local computation
     -p --protocol Set protocol
     --version  Show Version.
)";

void printCmd(const char *argv[], int size) {
    std::stringstream cmd;
    cmd << "cmd[ ";
    for (int i = 0; i < size; i++)
        cmd << std::string(argv[i]) << " ";
    std::cout << cmd.str() << "] passed" << "\n";
}

bool testGen() {
    const char *argv[] = {"docopt", "gen", "AA"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (!args["gen"].asBool()) return false;
    if (args["<meta file path>"].asString().compare("AA") != 0) return false;
    printCmd(argv, sizeof(argv)/ sizeof(argv[0]));
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
    printCmd(argv, sizeof(argv)/ sizeof(argv[0]));
    return true;
}

bool testLocalOption() {
    const char *argv[] = {"docopt", "encrypt", "AA", "BB", "CC", "-l", "ct"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (!args["encrypt"].asBool()) return false;
    if (args["<input file path>"].asString().compare("AA") != 0) return false;
    if (args["<output file path>"].asString().compare("BB") != 0) return false;
    if (args["<meta file path>"].asString().compare("CC") != 0) return false;
    if (args["--local"].asString().compare("ct") != 0) return false;
    printCmd(argv, sizeof(argv)/ sizeof(argv[0]));
    return true;
}

bool testVariables() {
    const char *argv[] = {"docopt", "eva", "-p", "PROT_CON", "1", "2"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    auto args = docopt::docopt(USAGE, {argv + 1, argv + argc}, false, "docopt 1.0");
    if (args["--protocol"].asString().compare("PROT_CON") != 0) return false;
    auto sList = args["<data>"].asStringList();
    if (sList[0].compare("1") != 0) return false;
    if (sList[1].compare("2") != 0) return false;
    return true;
}

int main(int argc, char *argv[]) {
    if (!testGen()) std::cerr << "testGen failed\n";
    if (!testOption()) std::cerr << "testOption failed\n";
    if (!testLocalOption()) std::cerr << "testLocalOption failed\n";
    if (!testVariables()) std::cerr << "testVariables failed\n";
    return 0;
}
