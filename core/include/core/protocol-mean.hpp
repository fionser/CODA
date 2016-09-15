#ifndef CORE_PROTOCOL_MEAN_HPP
#define CORE_PROTOCOL_MEAN_HPP
#include "protocol.hpp"

#include <string>
#include <memory>

class MeanProtocol : public Protocol {
public:
    MeanProtocol();
    ~MeanProtocol() {}

    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 bool local_compute,
                 core::pk_ptr pk,
                 core::context_ptr context) const override;

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk,
                 core::context_ptr context) const override;

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  core::pk_ptr pk,
                  core::context_ptr context) const override;

protected:
    core::FHEArg parameters() const override;

private:
    class ProtocolImp;
    std::shared_ptr<ProtocolImp> imp;
};
#endif // CORE_PROTOCOL_MEAN_HPP
