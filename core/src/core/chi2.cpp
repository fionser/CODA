#include <fstream>
#include <algorithm>

#include "core/protocol.hpp"
#include "core/file_util.hpp"
#include "core/literal.hpp"
#include "HElib/Ctxt.h"
#include "HElib/FHE.h"
#include "NTL/ZZX.h"
namespace protocol {
namespace chi2 {
static bool encryptPhenotype(std::fstream &fin,
			     const std::string &outputFilePath,
			     core::pk_ptr pk) {
    const long n = fheArgs.m >> 1;
    std::vector<int> coeff(n, 0);
    size_t pos;
    for (std::string line; std::getline(fin, line); ) {
	long id = coda::stol(line, &pos, 10);
	long ph = coda::stol(line.substr(pos), &pos, 10);
	if (id > n | id <= 0) {
	    std::cerr << "WARN! With ID " << id << " > m(" << fheArgs.m << ")\n";
	    continue;
	}
	if (ph != 0 && ph != 1) {
	    std::cerr << "WARN Invalid line in phenotype file: " << line << "\n";
	    continue;
	}
	coeff.at(id - 1) = ph;
    }

    NTL::ZZX poly;
    poly.SetLength(n);
    NTL::SetCoeff(poly, 0, coeff.front());

    auto itr = coeff.begin();
    std::advance(itr, 1);
    std::reverse(itr, coeff.end());
    for (long i = 1; i < n; i++) {
	NTL::SetCoeff(poly, i, -coeff.at(i));
    }

    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputFilePath, "File_1"),
	 	      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();
    return true;
}

static bool encryptGenotype(std::fstream &fin,
			    const std::string &outputFilePath,
			    core::pk_ptr pk) {
    const long n = fheArgs.m >> 1;
    std::vector<int> coeff(n);
    size_t pos;
    for (std::string line; std::getline(fin, line); ) {
	long id = coda::stol(line, &pos, 10);
	long gh = coda::stol(line.substr(pos), &pos, 10);
	if (id > n || id <= 0) {
	    std::cerr << "WARN! With ID " << id << " > m(" << fheArgs.m << ")\n";
	    continue;
	}
	if (!(gh >= 0 && gh <= 2)) {
	    std::cerr << "WARN Invalid line in genotype file: " << line << "\n";
	    continue;
	}
	coeff.at(id - 1) = gh;
    }

    NTL::ZZX poly;
    poly.SetLength(n);
    for (size_t i = 0; i < n; i++) {
	NTL::SetCoeff(poly, i, coeff.at(i));
    }
    Ctxt cipher(*pk);
    pk->Encrypt(cipher, poly);
    std::fstream fout(util::concatenate(outputFilePath, "File_1"),
		      std::ios::binary | std::ios::out);
    fout << cipher;
    fout.close();
    return true;
}

bool encrypt(const std::string &inputFilePath,
	     const std::string &outputFilePath,
	     core::pk_ptr pk) {
    std::fstream fin(inputFilePath);
    if (!fin.is_open()) {
	std::cerr << "WARN! Can not open file " << inputFilePath << "\n";
	return false;
    }

    std::string line;
    std::getline(fin, line);
    if (line.find("#protocol PROT_") == std::string::npos) {
	std::cerr << "WARN! invalid inputFilePath format! " << inputFilePath << "\n";
	return false;
    }

    std::getline(fin, line);
    if (line.find("#type phenotype") != std::string::npos)
	return encryptPhenotype(fin, outputFilePath, pk);
    else if (line.find("#type genotype") != std::string::npos)
	return encryptGenotype(fin, outputFilePath, pk);
    return false;
}

} // namespace chi2
} // namespace protocol
