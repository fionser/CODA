//
// Created by riku on 2016/08/09.
//
#include "../../include/core/protocol-contingency.hpp"
#include "../../include/core/contingency_table.hpp"
#include "../../include/core/global.hpp"
#include <fstream>
namespace contingency_table {
#ifdef CT_ON_DEMAND
class ProtocolImp {
public:
    ProtocolImp(core::Attribute P, core::Attribute Q) : _P(P), _Q(Q) {}
    ~ProtocolImp() {}
    bool encrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk);

    bool decrypt(const std::string &inputFilePath,
                 const std::string &outputDirPath,
                 core::pk_ptr pk,
                 core::sk_ptr sk);

    bool evaluate(const std::vector<std::string> &inputDirs,
                  const std::string &outputDir,
                  core::pk_ptr pk,
                  core::context_ptr context);
private:
    std::vector<core::Attribute> parseHeader(std::istream &in);
    const core::Attribute _P, _Q;
};

bool ProtocolImp::encrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath, core::pk_ptr pk)
{
    std::ifstream fin(inputFilePath, std::ios::binary);
    if (!fin.is_open()) {
        L_DEBUG(global::_console, "Can not open file {0}", inputFilePath);
        return false;
    }
    std::vector<core::Attribute> attributes = parseHeader(fin);

    fin.close();
    return false;
}

bool ProtocolImp::decrypt(const std::string &inputFilePath,
                          const std::string &outputDirPath, core::pk_ptr pk,
                          core::sk_ptr sk)
{
    return false;
}

bool ProtocolImp::evaluate(const std::vector <std::string> &inputDirs,
                           const std::string &outputDir, core::pk_ptr pk,
                           core::context_ptr context)
{
    return false;
}
#endif // CT_ON_DEMAND

} // namespace contingency_table

ContingencyTableProtocol::ContingencyTableProtocol(core::Attribute P,
                                                   core::Attribute Q)
        : Protocol(std::string("Contingency Table"))
{
    imp = std::make_shared<contingency_table::ProtocolImp>(P, Q);
}

bool ContingencyTableProtocol::encrypt(const std::string &inputFilePath,
                                       const std::string &outputDirPath,
                                       core::pk_ptr pk)
{
    return imp->encrypt(inputFilePath, outputDirPath, pk);
}

bool ContingencyTableProtocol::decrypt(const std::string &inputFilePath,
                                       const std::string &outputDirPath,
                                       core::pk_ptr pk, core::sk_ptr sk)
{
    return imp->decrypt(inputFilePath, outputDirPath, pk, sk);
}

bool ContingencyTableProtocol::
evaluate(const std::vector <std::string> &inputDirs,
         const std::string &outputDir, core::pk_ptr pk,
         core::context_ptr context)
{
    return imp->evaluate(inputDirs, outputDir, pk, context);
}
