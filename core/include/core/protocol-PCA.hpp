#ifndef CORE_PROTOCOL_PCA_HPP
#define CORE_PROTOCOL_PCA_HPP
#include "protocol.hpp"

#include <memory>
#include <string>
#include <vector>
typedef std::vector<std::string> StringList;
class PCAProtocol : public Protocol {
public:
    PCAProtocol();

    ~PCAProtocol() {}

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

    bool evaluate(const StringList &inputDirs,
                  const std::string &outputDir,
                  const StringList &params,
                  core::pk_ptr pk,
                  core::context_ptr context) override;
protected:
    core::FHEArg parameters() const override;

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
}; // PercentileProtocol

#endif // CORE_PROTOCOL_PCA_HPP
