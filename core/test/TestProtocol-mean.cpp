#include "../include/core/protocol-mean.hpp"
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

    MeanProtocol mean;
    FHE_NTIMER_START(ENCRYPT);
    mean.encrypt("test-ct-1", "test-dir-1/", false, pk, context);
    FHE_NTIMER_STOP(ENCRYPT);
    printNamedTimer(std::cout, "ENCRYPT");

    FHE_NTIMER_START(EVALUATE);
    std::vector<std::string> dirs;
    dirs.push_back("test-dir-1");
    mean.evaluate(dirs, "evaluated", {""}, pk, context);
    FHE_NTIMER_STOP(EVALUATE);
    printNamedTimer(std::cout, "EVALUATE");

    mean.decrypt("evaluated/FILE_1", ".", pk, sk, context);
    return 0;
}
