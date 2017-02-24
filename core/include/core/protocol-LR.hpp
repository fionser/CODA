#ifndef CORE_PROTOCOL_LR_HPP
#define CORE_PROTOCOL_LR_HPP
#include "protocol.hpp"

#include <memory>
#include <string>
#include <vector>
typedef std::vector<std::string> StringList;
class LRProtocol : public Protocol {
public:
    LRProtocol();

    ~LRProtocol() {}

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

    bool evaluate(const StringList &inputDirs,
                  const std::string &outputDir,
                  const StringList &params,
                  const core::PubKeyWrapper &pk,
                  const core::ContextWrapper &context) override;

    bool genKeypair() const;

protected:
    core::FHEArg parameters() const override;

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
};

#endif // CORE_PROTOCOL_LR_HPP
