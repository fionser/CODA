#ifndef CODA_PERCENTILE_HPP
#define CODA_PERCENTILE_HPP
#include "protocol.hpp"

#include <memory>
#include <string>
#include <vector>
typedef std::vector<std::string> StringList;
class PercentileProtocol : public Protocol {
public:
    PercentileProtocol();

    ~PercentileProtocol() {}

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
protected:
    core::FHEArg parameters() const override;

private:
    class Imp;
    std::shared_ptr<Imp> imp_;
}; // PercentileProtocol
#endif // CODA_PERCENTILE_HPP
