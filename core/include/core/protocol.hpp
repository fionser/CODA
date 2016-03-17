#ifndef core_PROTOCOL_PROTOCOL_H
#define core_PROTOCOL_PROTOCOL_H
#include "coda.hpp"
namespace protocol {
bool genKeypair(core::Protocol protocol,
		std::fstream &skStream,
		std::fstream &ctxtStream,
		std::fstream &pkStream);
namespace chi2 {
const core::FHEArg fheArgs = {64, 1031, 2, 5};
bool encrypt(const std::string &inputFilePath,
	     const std::string &outputFilePath,
	     core::pk_ptr pk);
} // namespace chi2
namespace contingency {
const core::FHEArg fheArgs = {64, 1031, 2, 5};
bool encrypt(const std::string &inputFilePath,
	     const std::string &outputFilePath,
	     core::pk_ptr pk);
} // contingency
} // namespace protocol
#endif // core_PROTOCOL_PROTOCOL_H
