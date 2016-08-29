#include "../include/core/protocol-contingency.hpp"
#include "../include/core/coda.hpp"
#include "../include/HElib/FHE.h"
#include "../include/HElib/timing.h"
int main () {
    core::context_ptr context = std::make_shared<FHEcontext>(5227, 67499, 1);
    buildModChain(*context, 10);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);

    ContingencyTableProtocol ct(1, 2);
    FHE_NTIMER_START(ENCRYPT);
    ct.encrypt("test-ct-1", "test-dir-1/", false, pk, context);
    ct.encrypt("test-ct-2", "test-dir-2/", false, pk, context);
    FHE_NTIMER_STOP(ENCRYPT);
    printNamedTimer(std::cout, "ENCRYPT");

    std::vector<std::string> inputDirs;
    inputDirs.push_back("./test-dir-1");
    inputDirs.push_back("./test-dir-2");
    FHE_NTIMER_START(EVA);
    ct.evaluate(inputDirs, "./test-out", pk, context);
    FHE_NTIMER_STOP(EVA);
    printNamedTimer(std::cout, "EVA");

    FHE_NTIMER_START(DEC);
    ct.decrypt("./test-out/File_result", "./", pk, sk, context);
    FHE_NTIMER_STOP(DEC);
    printNamedTimer(std::cout, "DEC");
    return 0;
}
