#include "core/protocol-hybrid-contingency.hpp"
#include "core/coda.hpp"
#include "HElib/FHE.h"
#include "HElib/timing.h"
int main () {
    FHE_NTIMER_START(SETUP);
    core::context_ptr context = std::make_shared<FHEcontext>(16384, 8191, 3);
    buildModChain(*context, 5);
    core::sk_ptr sk = std::make_shared<FHESecKey>(*context);
    sk->GenSecKey(64);
    // addSome1DMatrices(*sk);
    core::pk_ptr pk = std::make_shared<FHEPubKey>(*sk);
    FHE_NTIMER_STOP(SETUP);
    printNamedTimer(std::cout, "SETUP");

    FHE_NTIMER_START(ENCRYPT);
    HybridContingencyTable ct;
    ct.encrypt("test-ct-1", "test-dir-1/", true, pk, context);
    ct.encrypt("test-ct-2", "test-dir-2/", true, pk, context);
    FHE_NTIMER_STOP(ENCRYPT);
    printNamedTimer(std::cout, "ENCRYPT");

    std::vector<std::string> inputDirs;
    inputDirs.push_back("./test-dir-1");
    inputDirs.push_back("./test-dir-2");
    FHE_NTIMER_START(EVA);
    ct.evaluate(inputDirs, "./test-out", {"1", "2"}, pk, context);
    FHE_NTIMER_STOP(EVA);
    printNamedTimer(std::cout, "EVA");

    FHE_NTIMER_START(DEC);
    ct.decrypt("./test-out/FILE_1", "./", pk, sk, context);
    FHE_NTIMER_STOP(DEC);
    printNamedTimer(std::cout, "DEC");
    return 0;
}
