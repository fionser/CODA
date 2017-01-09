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
                 core::pk_ptr pk,
                 core::context_ptr context) override;

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context) override;

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  const std::vector<std::string> &params,
                  core::pk_ptr pk,
                  core::context_ptr context) override;
protected:
    core::FHEArg parameters() const override;

private:
    std::shared_ptr<contingency_table::HybridProtocolImp> imp;
};
#endif // CODA_CLION_PROTOCOL_HYBRID_CONTINGENCY_HPP
