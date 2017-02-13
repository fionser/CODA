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
    class ProtocolImp;
    std::shared_ptr<ProtocolImp> imp;
};
#endif // CORE_PROTOCOL_MEAN_HPP
