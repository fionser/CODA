//
// Created by riku on 2016/08/09.
//

#ifndef CODA_CLION_PROTOCOL_CONTINGENCY_HPP
#define CODA_CLION_PROTOCOL_CONTINGENCY_HPP
#include "protocol.hpp"
#include "contingency_table.hpp"

#include <string>
#include <memory>

#define CT_ON_DEMAND 1
namespace contingency_table {
class ProtocolImp;
}

class ContingencyTableProtocol : public Protocol {
public:
    ContingencyTableProtocol(int p = 0, int q = 0, long threshold = 2);

    ~ContingencyTableProtocol() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 const core::PubKeyWrapper &pk,
                 const core::ContextWrapper &context) override;

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 const core::PubKeyWrapper &pk,
                 const core::SecKeyWrapper &sk,
                 const core::ContextWrapper &context) override;

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  const std::vector<std::string> &params,
                  const core::PubKeyWrapper &pk,
                  const core::ContextWrapper &context) override;
protected:
    core::FHEArg parameters() const override;
private:
    std::shared_ptr<contingency_table::ProtocolImp> imp;
};
#endif //CODA_CLION_PROTOCOL_CONTINGENCY_HPP
