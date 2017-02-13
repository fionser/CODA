#ifndef CODA_CLION_PROTOCOL_HYBRID_CONTINGENCY_HPP
#define CODA_CLION_PROTOCOL_HYBRID_CONTINGENCY_HPP
#include "protocol.hpp"

namespace contingency_table {
class HybridProtocolImp;
}

class HybridContingencyTable : public Protocol {
public:
    HybridContingencyTable();

    ~HybridContingencyTable() {}

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
    std::shared_ptr<contingency_table::HybridProtocolImp> imp;
};
#endif // CODA_CLION_PROTOCOL_HYBRID_CONTINGENCY_HPP
